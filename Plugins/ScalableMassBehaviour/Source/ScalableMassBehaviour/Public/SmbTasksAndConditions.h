// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassNavigationTypes.h"
#include "MassStateTreeTypes.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassSignalSubsystem.h"
#include "StateTreeConditionBase.h"
#include "ScalableMassBehaviour.h"
#include "SmbAbilityData.h"
#include "SmbSubsystem.h"

#include "SmbTasksAndConditions.generated.h"


class UNavigationSystemV1;
struct FMassRepresentationFragment;
class UMassCrowdRepresentationSubsystem;
struct FResourceFragment;
class UMassSignalSubsystem;
class USmbSubsystem;

/*
 * Possible options for Instance Data:
 * UENUM()
 * enum class EStateTreePropertyUsage : uint8
 * {
 * 	Invalid,
 * 	Context,
 * 	Input,
 * 	Parameter,
 * 	Output,
 * };
 */

struct FMassMoveTargetFragment;

USTRUCT()
struct FGetRandomLocationInRangeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Input)
	FVector InLocation = FVector::DownVector;
	UPROPERTY(EditAnywhere, Category = Input)
	float Radius = 0.f;
	UPROPERTY(EditAnywhere, Category = Input)
	float AcceptableWalkRadius = 0.f;
	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation TargetLocation;
};

USTRUCT(meta = (DisplayName = "SMB Get Random Location Near InVector"))
struct FGetRandomLocationInRange : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGetRandomLocationInRangeInstanceData;

	FGetRandomLocationInRange();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FGetRandomLocationInRangeInstanceData::StaticStruct(); };
	
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
	TStateTreeExternalDataHandle<FDeathPhysicsSharedFragment> DeathFragmentHandle;
	TStateTreeExternalDataHandle<FLocationDataFragment> LocationDataFragmentHandle;
	TStateTreeExternalDataHandle<FTransformFragment> TransformFragmentHandle;
	//TStateTreeExternalDataHandle<UNavigationSystemV1> NavigationSystemHandle;
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
};


USTRUCT()
struct FGetProcessableLocationInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = Input)
	EProcessable ResourceType = EProcessable::Grass;
	UPROPERTY(EditAnywhere, Category = Input)
	float Radius = 50.f;
	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation TargetLocation = FMassTargetLocation();
};

USTRUCT(meta = (DisplayName = "SMB Get Processable of Type"))
struct FGetProcessableLocation : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGetProcessableLocationInstanceData;

	FGetProcessableLocation();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FGetProcessableLocationInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
};


UENUM(BlueprintType)
enum class ETaskType : uint8
{
	Pickup,
	Drop,
	Process
};

USTRUCT()
struct FProcessResourceInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = Input)
	EProcessable ResourceType = EProcessable::Grass;
	UPROPERTY(EditAnywhere, Category = Parameter)
	float ProcessDuration = 1.f;
	UPROPERTY(EditAnywhere, Category = Parameter)
	ETaskType TaskType = ETaskType::Pickup;
	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation TargetLocation = FMassTargetLocation();
};

USTRUCT(meta = (DisplayName = "SMB Process Resource Task"))
struct FProcessResource : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FProcessResourceInstanceData;

	FProcessResource();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FProcessResourceInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FMassVelocityFragment> VelocityFragmentHandle;
	TStateTreeExternalDataHandle<FMassDesiredMovementFragment> DesiredFragmentHandle;
	TStateTreeExternalDataHandle<FResourceFragment> ResourceFragmentHandle;
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
};


USTRUCT()
struct FNavDoneConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Output)
	bool bIsDone = false;
};

USTRUCT(meta = (DisplayName = "SMB Is Nav Done"))
struct FNavDoneCondition : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FNavDoneConditionInstanceData;

	UPROPERTY(EditAnywhere, Category = Condition)
	bool bInvert = false;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	//virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};


USTRUCT()
struct FIsCloseEnoughConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Input)
	FVector InLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = Input)
	float AcceptableDistance = 50.f;

	UPROPERTY(EditAnywhere, Category = Output)
	bool bConditionResult = false;
};

USTRUCT(meta = (DisplayName = "SMB Close Enough to Target Location"))
struct FIsCloseEnoughCondition : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FIsCloseEnoughConditionInstanceData;

	UPROPERTY(EditAnywhere, Category = Condition)
	bool bInvertResult = false;
	UPROPERTY(EditAnywhere, Category = Condition)
	bool bUseInternalWalkTarget = true;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
	virtual bool Link(FStateTreeLinker& Linker) override;
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FLocationDataFragment> LocationDataFragmentHandle;
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
};

USTRUCT()
struct FChangeStateInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Input)
	EAggressionState NewState = EAggressionState::Pacifist;
};

USTRUCT(meta = (DisplayName = "SMB Set Entity State"))
struct FChangeState : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FChangeStateInstanceData;

	FChangeState();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FChangeStateInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	//virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;

	TStateTreeExternalDataHandle<FStateFragment> StateFragmentHandle;
};

USTRUCT()
struct FGetStateInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Output)
	EAggressionState CurrentState = EAggressionState::Pacifist;
};

USTRUCT(meta = (DisplayName = "SMB Get Entity State"))
struct FGetState : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FGetStateInstanceData;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FGetStateInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	//virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;

	TStateTreeExternalDataHandle<FStateFragment> StateFragmentHandle;
};


USTRUCT()
struct FEnemyDistanceConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Input)
	FSmbEntityData InEnemy = FSmbEntityData();

	UPROPERTY(EditAnywhere, Category = Input)
	float AcceptableDistance = 50.f;

	UPROPERTY(EditAnywhere, Category = Output)
	bool bConditionResult = false;
};

USTRUCT(meta = (DisplayName = "SMB Close Enough To Ability Target"))
struct FEnemyDistanceCondition : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyDistanceConditionInstanceData;

	//UPROPERTY(EditAnywhere, Category = Condition)
	//bool bInvert = false;
	//UPROPERTY(EditAnywhere, Category = Condition)
	//bool bUseMassTargetLocation = false;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
	virtual bool Link(FStateTreeLinker& Linker) override;
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FNearEnemiesFragment> NearEnemiesFragHandle;
};


USTRUCT()
struct FFindClosestEnemyInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Input)
	float MaxDistance = 1000.f;
	
	UPROPERTY(EditAnywhere, Category = Output)
	FSmbEntityData EntityTarget = FSmbEntityData();
	UPROPERTY(EditAnywhere, Category = Output)
	bool bFoundEntityTarget = false;
};

USTRUCT(meta = (DisplayName = "SMB Find Closest Enemy"))
struct FFindClosestEnemy : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FFindClosestEnemyInstanceData;

	FFindClosestEnemy();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FFindClosestEnemyInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;

	UPROPERTY(EditAnywhere, Category = "Smb")
	bool bTickingFind = false;
	UPROPERTY(EditAnywhere, Category = "Smb")
	float TickDelay = 1.f;
	
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FAttackFragment> AttackFragmentHandle;
	TStateTreeExternalDataHandle<FTeamFragment> TeamFragmentHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
};
/*
USTRUCT()
struct FProcessAttackInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = Input)
	FSmbEntityData EntityTarget;
};

USTRUCT(meta = (DisplayName = "SMB Attack Enemy"))
struct FProcessAttack : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FProcessAttackInstanceData;

	FProcessAttack();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FProcessAttackInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FAttackFragment> AttackFragmentHandle;
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
	TStateTreeExternalDataHandle<FMassDesiredMovementFragment> DesiredMovementHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
	TStateTreeExternalDataHandle<FMassMovementParameters> MovementParamHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
}; */



USTRUCT()
struct FWalkToEntityInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = Input)
	FSmbEntityData EntityTarget;

	UPROPERTY(EditAnywhere, Category = Input)
	float DistanceAway = 50.f;

	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation LocationTarget;
};

USTRUCT(meta = (DisplayName = "SMB Get Entity Location"))
struct FWalkToEntityLocation : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWalkToEntityInstanceData;

	FWalkToEntityLocation();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FWalkToEntityInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FAttackFragment> AttackFragmentHandle;
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
	TStateTreeExternalDataHandle<FNearEnemiesFragment> NearEnemiesFragHandle;
	TStateTreeExternalDataHandle<FLocationDataFragment> LocationDataFragHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
};




USTRUCT()
struct FCheckOwnHealthInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Output)
	float EntityHealth = -1.f;
};

USTRUCT(meta = (DisplayName = "SMB Check own health"))
struct FCheckOwnHealth : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCheckOwnHealthInstanceData;

	FCheckOwnHealth();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FCheckOwnHealthInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FDefenceFragment> DefenceFragmentHandle;
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
	TStateTreeExternalDataHandle<FDeathPhysicsSharedFragment> DeathPhysicsHandle;
	TStateTreeExternalDataHandle<FMassRepresentationFragment> MassRepFragmentHandle;
	TStateTreeExternalDataHandle<UMassCrowdRepresentationSubsystem> MassCrowdRepHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
};


USTRUCT()
struct FAttackWithSkillInstanceData
{
	GENERATED_BODY()

	/* Skill to use on target */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<USmbAbilityData> SkillData;

	/* Target for attack (Less priority than Location) */
	UPROPERTY(EditAnywhere, Category = Input)
	FSmbEntityData EntityToAttack;

	/* Specific Location for Attack in World Space (Priority over Entity target) */
	UPROPERTY(EditAnywhere, Category = Input)
	FVector LocationToAttack = FVector::DownVector*9999.f;

	UPROPERTY(EditAnywhere, Category = "Smb")
	float TimeInAttack = 0.f;

	UPROPERTY(EditAnywhere, Category = "Smb")
	int8 TimeSpawned = 0;
};

USTRUCT(meta = (DisplayName = "SMB Attack With Skill"))
struct FAttackWithSkill : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAttackWithSkillInstanceData;

	FAttackWithSkill();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FAttackWithSkillInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
	TStateTreeExternalDataHandle<FMassRepresentationFragment> MassRepFragmentHandle;
	TStateTreeExternalDataHandle<FTeamFragment> TeamFragmentHandle;
	TStateTreeExternalDataHandle<FLocationDataFragment> LocationDataHandle;
	
	TStateTreeExternalDataHandle<FNearEnemiesFragment> NearEnemiesFragHandle;
	
	TStateTreeExternalDataHandle<UMassCrowdRepresentationSubsystem> MassCrowdRepHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
};




USTRUCT()
struct FListenEnemyInstanceData
{
	GENERATED_BODY()

	/* Signal to listen for (Default is FoundEnemy) */
	UPROPERTY(EditAnywhere, Category = Input)
	FName Signal = Smb::Signals::FoundEnemy;

	/* Found Enemy */
	UPROPERTY(EditAnywhere, Category = Output)
	FSmbEntityData Enemy;
};

USTRUCT(meta = (DisplayName = "SMB Locate Enemy Observer"))
struct FListenEnemy : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FListenEnemyInstanceData;

	FListenEnemy();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FListenEnemyInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
	TStateTreeExternalDataHandle<FNearEnemiesFragment> NearEnemiesFragHandle;
};


USTRUCT()
struct FListenerTaskInstanceData
{
	GENERATED_BODY()

	/* Signal to listen for */
	UPROPERTY(EditAnywhere, Category = Input)
	FName Signal = Smb::Signals::MoveTargetChanged;
	/* New Data */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bNewData = false;
};

USTRUCT(meta = (DisplayName = "SMB Reset on Signal task"))
struct FListenerTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FListenerTaskInstanceData;

	FListenerTask();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FListenerTaskInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
};




USTRUCT()
struct FCheckHealthListenerInstanceData
{
	GENERATED_BODY()

	/* Current Health */
	UPROPERTY(EditAnywhere, Category = Output)
	float CurHealth = 1.f;
};

USTRUCT(meta = (DisplayName = "SMB Health Change Listener"))
struct FCheckHealthListener : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCheckHealthListenerInstanceData;

	FCheckHealthListener();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FCheckHealthListenerInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
	TStateTreeExternalDataHandle<FDefenceFragment> DefenceHandle;
	TStateTreeExternalDataHandle<FTransformFragment> EntityTransformHandle;
	TStateTreeExternalDataHandle<FAnimationFragment> AnimationFragmentHandle;
	TStateTreeExternalDataHandle<FDeathPhysicsSharedFragment> DeathPhysicsHandle;
	TStateTreeExternalDataHandle<FMassRepresentationFragment> MassRepFragmentHandle;
	TStateTreeExternalDataHandle<UMassCrowdRepresentationSubsystem> MassCrowdRepHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
};

USTRUCT()
struct FNewNavTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Input)
	float RadiusNear = 150.f;

	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation WalkToTarget;
};

USTRUCT(meta = (DisplayName = "SMB Get Valid Location Near"))
struct FNewWalkTarget : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FNewNavTargetInstanceData;

	FNewWalkTarget();

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FNewNavTargetInstanceData::StaticStruct(); };
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	//virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	//TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
	TStateTreeExternalDataHandle<FLocationDataFragment> LocationDataFragHandle;
	TStateTreeExternalDataHandle<FTransformFragment> TransformFragmentHandle;
	TStateTreeExternalDataHandle<USmbSubsystem> SmbSubsystemHandle;
};


