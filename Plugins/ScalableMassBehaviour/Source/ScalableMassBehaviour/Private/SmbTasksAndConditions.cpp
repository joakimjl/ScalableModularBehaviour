// Copyright © 2025 Land Chaunax, All rights reserved.

#include "SmbTasksAndConditions.h"
#include "Engine/World.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "SmbSubsystem.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeDependency.h"
#include "MassStateTreeTypes.h"
#include "MassNavigationFragments.h"
#include "SmbFragments.h"
#include "MassRepresentationSubsystem.h"
#include "MassRepresentationProcessor.h"
#include "MassCrowdRepresentationSubsystem.h"
#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "SmbNiagaraContainer.h"
#include "Compression/lz4.h"
#include "Kismet/GameplayStatics.h"

//Own Location boilerplate


FGetOwnLocation::FGetOwnLocation()
{
}

bool FGetOwnLocation::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityTransformHandle);
	return true;
}

void FGetOwnLocation::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FTransformFragment>();
}

EStateTreeRunStatus FGetOwnLocation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FTransformFragment& EntityTransform = Context.GetExternalData(EntityTransformHandle);
	
	InstanceData.OwnLocation = EntityTransform.GetTransform().GetLocation();

	FMassTargetLocation& OutLocation = InstanceData.TargetLocation;
	OutLocation.EndOfPathPosition = InstanceData.OwnLocation;
	OutLocation.EndOfPathIntent = EMassMovementAction::Stand;
	
	return EStateTreeRunStatus::Succeeded;
}



FGetRandomLocationInRange::FGetRandomLocationInRange()
{
	bShouldCallTick = false;
}

bool FGetRandomLocationInRange::Link(FStateTreeLinker& Linker)
{
	//Linker.LinkExternalData(MassSignalSubsystemHandle);
	//Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(AnimationFragmentHandle);
	Linker.LinkExternalData(DeathFragmentHandle);
	Linker.LinkExternalData(LocationDataFragmentHandle);
	Linker.LinkExternalData(TransformFragmentHandle);
	//Linker.LinkExternalData(NavigationSystemHandle);
	return true;
}

void FGetRandomLocationInRange::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(AnimationFragmentHandle);
	Builder.AddReadWrite(DeathFragmentHandle);
	Builder.AddReadWrite(LocationDataFragmentHandle);
	Builder.AddReadOnly(TransformFragmentHandle);
	//Builder.AddReadWrite(NavigationSystemHandle);
}

EStateTreeRunStatus FGetRandomLocationInRange::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	FVector RandomLocation;
	if (InstanceData.RandomRadiusRange <= 0.f)
	{
		RandomLocation = InstanceData.InLocation;
	} else {
		FVector RandomOffset = InstanceData.InLocation;
		RandomOffset.X += FMath::FRandRange(-InstanceData.RandomRadiusRange, InstanceData.RandomRadiusRange);
		RandomOffset.Y += FMath::FRandRange(-InstanceData.RandomRadiusRange, InstanceData.RandomRadiusRange);
		RandomLocation = RandomOffset;
	}
	FTransformFragment& TransformFragment = Context.GetExternalData(TransformFragmentHandle);
	FVector Location = TransformFragment.GetTransform().GetLocation();
	FLocationDataFragment LocationDataFragment = Context.GetExternalData(LocationDataFragmentHandle);

	
	if (LocationDataFragment.WalkToLocation != FVector::DownVector && !InstanceData.bNewLocation)
	{
		FVector RandomOffset = FVector::ZeroVector;
		RandomOffset.X += FMath::FRandRange(-InstanceData.RandomRadiusRange, InstanceData.RandomRadiusRange);
		RandomOffset.Y += FMath::FRandRange(-InstanceData.RandomRadiusRange, InstanceData.RandomRadiusRange);
		RandomLocation = LocationDataFragment.WalkToLocation + RandomOffset;
		
		if ((Location-RandomLocation).Size() <= InstanceData.AcceptableWalkRadius)
		{
			FAnimationFragment& AnimFrag = Context.GetExternalData(AnimationFragmentHandle);
			AnimFrag.CurrentState = EAnimationState::Idle;
			//UE_LOG(LogTemp, Display, TEXT("Close enough to target"));
			return EStateTreeRunStatus::Succeeded;
		}
	}
	
	//The way they did it in MassNavMeshTask
	UNavigationSystemV1* NavMeshSubsystem = Cast<UNavigationSystemV1>(Context.GetWorld()->GetNavigationSystem());
	FNavLocation NavLocation;
	bool bFoundLocation = NavMeshSubsystem->ProjectPointToNavigation(RandomLocation,NavLocation, FVector(300,300,300));
	if (!bFoundLocation) return EStateTreeRunStatus::Failed;

	FMassTargetLocation OutLocation = FMassTargetLocation();
	OutLocation.EndOfPathPosition = NavLocation.Location;
	OutLocation.EndOfPathIntent = EMassMovementAction::Stand;

	InstanceData.TargetLocation = OutLocation;

	FAnimationFragment& AnimFrag = Context.GetExternalData(AnimationFragmentHandle);
	AnimFrag.CurrentState = EAnimationState::Running;
	//UE_LOG(LogTemp, Display, TEXT("Walking to location"));
	return EStateTreeRunStatus::Running;
}


FGetProcessableLocation::FGetProcessableLocation()
{
	bShouldCallTick = false;
}

bool FGetProcessableLocation::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(SmbSubsystemHandle);
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(AnimationFragmentHandle);
	return true;
}

void FGetProcessableLocation::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(SmbSubsystemHandle);
	Builder.AddReadOnly(EntityTransformHandle);
	Builder.AddReadWrite(AnimationFragmentHandle);
}

EStateTreeRunStatus FGetProcessableLocation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	//const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	USmbSubsystem& SmbSubsystem = Context.GetExternalData(SmbSubsystemHandle);
	const FTransformFragment& TransformFragment = Context.GetExternalData(EntityTransformHandle);
	FAnimationFragment& AnimFrag = Context.GetExternalData(AnimationFragmentHandle);
	FVector EntityLocation = TransformFragment.GetTransform().GetLocation();
	FMassEntityHandle ClosestHandle = SmbSubsystem.GetClosestResource(InstanceData.ResourceType, EntityLocation);
	FVector WalkLocation;
	if (ClosestHandle.IsValid() == false) {
		TArray<FVector> ResourceLocations = SmbSubsystem.GetResources(InstanceData.ResourceType);
		WalkLocation = ResourceLocations[0];
	} else
	{
		WalkLocation = SmbSubsystem.GetEntityLocation(ClosestHandle);
	}

	FMassTargetLocation OutLocation = FMassTargetLocation();
	OutLocation.EndOfPathPosition = WalkLocation;
	OutLocation.EndOfPathIntent = EMassMovementAction::Stand;
	AnimFrag.CurrentState = EAnimationState::Running;

	InstanceData.TargetLocation = OutLocation;
	
	return EStateTreeRunStatus::Running;
}


bool FNavDoneCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(MassStateTreeContext.GetWorld());
	return (NavSys != nullptr)^bInvert;
}


bool FIsCloseEnoughCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(LocationDataFragmentHandle);
	Linker.LinkExternalData(AnimationFragmentHandle);

	return true;
}

bool FIsCloseEnoughCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	const FTransformFragment& TransformFragment = Context.GetExternalData(EntityTransformHandle);
	const FVector EntityLocation = TransformFragment.GetTransform().GetLocation();
	const FLocationDataFragment LocationDataFragment = Context.GetExternalData(LocationDataFragmentHandle);
	FAnimationFragment& AnimationFragment = Context.GetExternalData(AnimationFragmentHandle);
	
	float Distance;
	if (bUseInternalWalkTarget)
	{
		Distance = (LocationDataFragment.WalkToLocation - EntityLocation).Size();
		AnimationFragment.CurrentState = EAnimationState::Idle;
	} else
	{
		Distance = (InstanceData.InLocation - EntityLocation).Size();
	}
	bool bIsCloseEnough = Distance < InstanceData.AcceptableDistance;

	return bIsCloseEnough^bInvertResult;
}



bool FEnemyDistanceCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(NearEnemiesFragHandle);
	return true;
}

bool FEnemyDistanceCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	const FTransformFragment& TransformFragment = Context.GetExternalData(EntityTransformHandle);
	FNearEnemiesFragment& NearEnemiesFragment = Context.GetExternalData(NearEnemiesFragHandle);
	FVector EntityLocation = TransformFragment.GetTransform().GetLocation();
	if (NearEnemiesFragment.ClosestEnemies.Num() == 0) return false;
	
	FMassEntityHandle ClosestHandle(NearEnemiesFragment.ClosestEnemies[0].Index, NearEnemiesFragment.ClosestEnemies[0].SerialNumber);
	if (!MassStateTreeContext.GetEntityManager().IsEntityValid(ClosestHandle)) return false;
	
	USmbSubsystem* SmbSubsystem = MassStateTreeContext.GetWorld()->GetSubsystem<USmbSubsystem>();
	if (!SmbSubsystem) return false;
	FMassEntityManager& EntityManager = MassStateTreeContext.GetEntityManager();

	FAgentRadiusFragment* AgentRadiusFragment = EntityManager.GetFragmentDataPtr<FAgentRadiusFragment>(ClosestHandle);
	if (!AgentRadiusFragment) return false;
	FTransformFragment* OtherTransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(ClosestHandle);
	if (!OtherTransformFrag) return false;
	FVector OtherLocation = OtherTransformFrag->GetTransform().GetLocation();
	FVector OwnLocation = TransformFragment.GetTransform().GetLocation();
	FVector TargetLocation = OtherLocation+(OwnLocation-OtherLocation).GetSafeNormal()*(AgentRadiusFragment->Radius*0.85f);
	
	float Distance = (EntityLocation - TargetLocation).Size();
	bool bIsCloseEnough = Distance < InstanceData.AcceptableDistance;
	InstanceData.bConditionResult = bIsCloseEnough;
	return bIsCloseEnough;
}

FProcessResource::FProcessResource()
{
	bShouldCallTick = true;
}

bool FProcessResource::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(ResourceFragmentHandle);
	Linker.LinkExternalData(VelocityFragmentHandle);
	Linker.LinkExternalData(DesiredFragmentHandle);
	Linker.LinkExternalData(SmbSubsystemHandle);
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	Linker.LinkExternalData(AnimationFragmentHandle);;
	return true;
}

void FProcessResource::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(EntityTransformHandle);
	Builder.AddReadWrite(ResourceFragmentHandle);
	Builder.AddReadWrite(VelocityFragmentHandle);
	Builder.AddReadWrite(DesiredFragmentHandle);
	Builder.AddReadWrite(SmbSubsystemHandle);
	Builder.AddReadWrite(MassSignalSubsystemHandle);
	Builder.AddReadWrite(AnimationFragmentHandle);;
}

EStateTreeRunStatus FProcessResource::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	USmbSubsystem& SmbSubsystem = Context.GetExternalData(SmbSubsystemHandle);
	FMassVelocityFragment& VelocityFragment = Context.GetExternalData(VelocityFragmentHandle);
	VelocityFragment.Value = FVector::ZeroVector;
	FMassDesiredMovementFragment& DesiredFragment = Context.GetExternalData(DesiredFragmentHandle);
	DesiredFragment.DesiredVelocity = FVector::ZeroVector;

	FAnimationFragment& AnimFrag = Context.GetExternalData(AnimationFragmentHandle);
	AnimFrag.CurrentState = EAnimationState::ProcessingResource;

	UMassSignalSubsystem& MassSignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);
	MassSignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
		UE::Mass::Signals::StandTaskFinished, MassStateTreeContext.GetEntity(), InstanceData.ProcessDuration);


	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FProcessResource::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	USmbSubsystem& SmbSubsystem = Context.GetExternalData(SmbSubsystemHandle);
	FResourceFragment& ResourceFragment = Context.GetExternalData(ResourceFragmentHandle);
	FTransformFragment& TransformFragment = Context.GetExternalData(EntityTransformHandle);

	ResourceFragment.TimeSinceGatherStart += DeltaTime;
	
	if (ResourceFragment.TimeSinceGatherStart >= InstanceData.ProcessDuration)
	{
		ResourceFragment.TimeSinceGatherStart = 0.f;
		const EProcessable Resource = InstanceData.ResourceType;

		if (InstanceData.TaskType == ETaskType::Process)
		{
			if (SmbSubsystem.ReqMap.ReqMap.Contains(Resource))
			{
				FProcessableReqArr ProcessableArr = SmbSubsystem.ReqMap.ReqMap[Resource];
				for (int i = 0; i < SmbSubsystem.ReqMap.ReqMap[Resource].TypeArr.Num(); ++i)
				{
					if (ResourceFragment.Carrying[ProcessableArr.TypeArr[i]] < ProcessableArr.AmountArr[i])
						return EStateTreeRunStatus::Failed;
				}
			}
		}

		if (InstanceData.TaskType == ETaskType::Pickup)
		{
			if (!ResourceFragment.BonusMap.Contains(Resource))
				ResourceFragment.BonusMap.Add(Resource, 1.f);
			if (!ResourceFragment.Carrying.Contains(Resource))
			{
				ResourceFragment.Carrying.Add(Resource, 1*ResourceFragment.BonusMap[Resource]);
			} else
			{
				ResourceFragment.Carrying[Resource] += 1*ResourceFragment.BonusMap[Resource];
			}
			return EStateTreeRunStatus::Succeeded;
		}

		if (InstanceData.TaskType == ETaskType::Drop)
		{
			FVector EntityLocation = TransformFragment.GetTransform().GetLocation();
			FMassEntityHandle Target = SmbSubsystem.GetClosestResource(Resource, EntityLocation);
			//UE_LOG(LogTemp, Display, TEXT("Closest entity is valid? %i"), SmbSubsystem.IsEntityValidManager(Target));
			if (SmbSubsystem.IsEntityValidManager(Target))
			{
				TArray<EProcessable> Keys = TArray<EProcessable>();
				ResourceFragment.Carrying.GetKeys(Keys);
				for (int i = 0; i < Keys.Num(); ++i)
				{
					const EProcessable CurResource = Keys[i];
					if (CurResource == EProcessable::Storage) continue;
					const int32 CarryingAmount = ResourceFragment.Carrying[CurResource];
					ResourceFragment.Carrying[CurResource] = 0;
					bool Res = SmbSubsystem.AddToEntity(Target, CurResource, CarryingAmount);
				}
				return EStateTreeRunStatus::Succeeded;
			}
			//UE_LOG(LogTemp, Display, TEXT("Failed to drop resource"));
			return EStateTreeRunStatus::Failed;
		}
	}
	
	return EStateTreeRunStatus::Running;
}




FFindClosestEnemy::FFindClosestEnemy()
{
	bShouldCallTick = true;
}

bool FFindClosestEnemy::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(SmbSubsystemHandle);
	Linker.LinkExternalData(TeamFragmentHandle);
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	return true;
}

void FFindClosestEnemy::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(EntityTransformHandle);
	Builder.AddReadWrite(SmbSubsystemHandle);
	Builder.AddReadWrite(TeamFragmentHandle);
	Builder.AddReadWrite(MassSignalSubsystemHandle);
}

EStateTreeRunStatus FFindClosestEnemy::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	USmbSubsystem& SmbSubsystem = Context.GetExternalData(SmbSubsystemHandle);
	FTransformFragment& TransformFragment = Context.GetExternalData(EntityTransformHandle);
	FTeamFragment& TeamFragment = Context.GetExternalData(TeamFragmentHandle);

	InstanceData.bFoundEntityTarget = true;
	FSmbEntityData EntityData = SmbSubsystem.GetClosestEnemy(TransformFragment.GetTransform().GetLocation(), TeamFragment.TeamID, InstanceData.MaxDistance);
	if (EntityData.SerialNumber == -1 && EntityData.Index == -1) InstanceData.bFoundEntityTarget = false;
	//UE_LOG(LogTemp, Display, TEXT("Found entity was (Enter): %i %i which means: %i"), EntityData.Index, EntityData.SerialNumber, InstanceData.bFoundEntityTarget);

	UMassSignalSubsystem& MassSignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);

	if (EntityData.SerialNumber == -1 && EntityData.Index == -1)
	{
		MassSignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
			UE::Mass::Signals::StandTaskFinished, MassStateTreeContext.GetEntity(), FMath::RandRange(0.01,0.1));
	}

	if (bTickingFind)
		MassSignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
			UE::Mass::Signals::StandTaskFinished, MassStateTreeContext.GetEntity(), TickDelay + FMath::RandRange(0.01,0.1));

	InstanceData.EntityTarget = EntityData;
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FFindClosestEnemy::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	USmbSubsystem& SmbSubsystem = Context.GetExternalData(SmbSubsystemHandle);
	FTransformFragment& TransformFragment = Context.GetExternalData(EntityTransformHandle);
	FTeamFragment& TeamFragment = Context.GetExternalData(TeamFragmentHandle);

	InstanceData.bFoundEntityTarget = true;
	FSmbEntityData EntityData = SmbSubsystem.GetClosestEnemy(TransformFragment.GetTransform().GetLocation(), TeamFragment.TeamID, InstanceData.MaxDistance);
	if (EntityData.SerialNumber == -1 && EntityData.Index == -1) InstanceData.bFoundEntityTarget = false;
	//UE_LOG(LogTemp, Display, TEXT("Found entity was (Tick): %i %i which means: %i"), EntityData.Index, EntityData.SerialNumber, InstanceData.bFoundEntityTarget);

	UMassSignalSubsystem& MassSignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);

	if (bTickingFind)
		MassSignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
			UE::Mass::Signals::StandTaskFinished, MassStateTreeContext.GetEntity(), TickDelay + FMath::RandRange(0.01,0.1));
	
	InstanceData.EntityTarget = EntityData;
	
	return EStateTreeRunStatus::Running;
}



FWalkToEntityLocation::FWalkToEntityLocation()
{
	bShouldCallTick = false;
}

bool FWalkToEntityLocation::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(SmbSubsystemHandle);
	Linker.LinkExternalData(AnimationFragmentHandle);
	Linker.LinkExternalData(NearEnemiesFragHandle);
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	Linker.LinkExternalData(LocationDataFragHandle);
	return true;
}

void FWalkToEntityLocation::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(EntityTransformHandle);
	Builder.AddReadWrite(SmbSubsystemHandle);
	Builder.AddReadWrite(AnimationFragmentHandle);
	Builder.AddReadWrite(NearEnemiesFragHandle);
	Builder.AddReadWrite(MassSignalSubsystemHandle);
	Builder.AddReadWrite(LocationDataFragHandle);
}

EStateTreeRunStatus FWalkToEntityLocation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	USmbSubsystem& SmbSubsystem = Context.GetExternalData(SmbSubsystemHandle);
	FAnimationFragment& AnimationFragment = Context.GetExternalData(AnimationFragmentHandle);
	FTransform& Transform = Context.GetExternalData(EntityTransformHandle).GetMutableTransform();
	FLocationDataFragment& LocationDataFragment = Context.GetExternalData(LocationDataFragHandle);

	AnimationFragment.CurrentState = EAnimationState::Running;

	FMassTargetLocation OutLocation = FMassTargetLocation();
	FNearEnemiesFragment& NearEnemies = Context.GetExternalData(NearEnemiesFragHandle);
	if (NearEnemies.ClosestEnemies.Num() <= 0)
	{
		return EStateTreeRunStatus::Failed;
	}
	InstanceData.EntityTarget = NearEnemies.ClosestEnemies[0];
	
	if (SmbSubsystem.IsEntityValidManager(NearEnemies.ClosestEnemies[0])){
		const FVector EntityLoc = SmbSubsystem.GetEntityLocation(NearEnemies.ClosestEnemies[0]);
		UNavigationSystemV1* NavMeshSubsystem = Cast<UNavigationSystemV1>(Context.GetWorld()->GetNavigationSystem());
		FNavLocation NavLocation;
		bool FoundLocation = NavMeshSubsystem->ProjectPointToNavigation((EntityLoc-Transform.GetLocation())/1.5f+Transform.GetLocation(),NavLocation, FVector(InstanceData.DistanceAway,InstanceData.DistanceAway,400.f));
		if (!FoundLocation) return EStateTreeRunStatus::Failed;
		if (!LocationDataFragment.bIsAttackLocation)
		{
			LocationDataFragment.PrevWalkToLocationBeforeAttack = FVector(LocationDataFragment.WalkToLocation);
			LocationDataFragment.bIsAttackLocation = true;
		}
		LocationDataFragment.WalkToLocation = NavLocation.Location;
		OutLocation.EndOfPathPosition = NavLocation.Location;
	}

	OutLocation.EndOfPathIntent = EMassMovementAction::Stand;

	InstanceData.LocationTarget = OutLocation;

	return EStateTreeRunStatus::Succeeded;
}




FCheckOwnHealth::FCheckOwnHealth()
{
	bShouldCallTick = true;
}

bool FCheckOwnHealth::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(SmbSubsystemHandle);
	Linker.LinkExternalData(AnimationFragmentHandle);
	Linker.LinkExternalData(DefenceFragmentHandle);
	Linker.LinkExternalData(DeathPhysicsHandle);
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	Linker.LinkExternalData(MassCrowdRepHandle);
	Linker.LinkExternalData(MassRepFragmentHandle);
	return true;
}

void FCheckOwnHealth::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(EntityTransformHandle);
	Builder.AddReadWrite(SmbSubsystemHandle);
	Builder.AddReadWrite(AnimationFragmentHandle);
	Builder.AddReadWrite(DefenceFragmentHandle);
	Builder.AddReadWrite(DeathPhysicsHandle);
	Builder.AddReadWrite(MassCrowdRepHandle);
	Builder.AddReadWrite(MassRepFragmentHandle);
	Builder.AddReadWrite(MassSignalSubsystemHandle);
}

EStateTreeRunStatus FCheckOwnHealth::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);
	
	return EStateTreeRunStatus::Running;
	
}

EStateTreeRunStatus FCheckOwnHealth::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	USmbSubsystem& SmbSubsystem = Context.GetExternalData(SmbSubsystemHandle);
	FAnimationFragment& AnimationFragment = Context.GetExternalData(AnimationFragmentHandle);
	FDefenceFragment& DefenceFragment = Context.GetExternalData(DefenceFragmentHandle);
	
	
	InstanceData.EntityHealth = DefenceFragment.HP;

	//UE_LOG(LogTemp, Display, TEXT("Defence Fragment HP: %f"), DefenceFragment.HP);

	if (DefenceFragment.HP <= 0.f)
	{
		//UE_LOG(LogTemp, Display, TEXT("FCheckOwnHealth::Tick, Dead"));
		if (AnimationFragment.CurrentState == EAnimationState::Dead) return EStateTreeRunStatus::Running;
		AnimationFragment.CurrentState = EAnimationState::Dead;
		FDeathPhysicsSharedFragment& DeathFragment = Context.GetExternalData(DeathPhysicsHandle);
		FTransformFragment& Transform = Context.GetExternalData(EntityTransformHandle);
		
		DeathFragment.TotalDeaths += 1;
		DeathFragment.DeathLocations.Add(Transform.GetTransform().GetLocation());
		
		UMassCrowdRepresentationSubsystem& CrowdRepSubsystem = Context.GetExternalData(MassCrowdRepHandle);
		FMassRepresentationFragment& MassRepresentationFragment = Context.GetExternalData(MassRepFragmentHandle);
		
		if (MassRepresentationFragment.StaticMeshDescHandle.IsValid())
		{
			FMassInstancedStaticMeshInfoArrayView ISMInfosView = CrowdRepSubsystem.GetMutableInstancedStaticMeshInfos();
			FMassInstancedStaticMeshInfo ISMInfo = ISMInfosView[MassRepresentationFragment.StaticMeshDescHandle.ToIndex()];
			if (MassRepresentationFragment.CurrentRepresentation == EMassRepresentationType::HighResSpawnedActor)
			{
				SmbSubsystem.DetatchActorSetHealth(MassStateTreeContext.GetEntity());
			} else
			{
				SmbSubsystem.NewDeath(ISMInfo.GetDesc().Meshes[0].Mesh->GetName()
					,DeathFragment.TotalDeaths
					,DeathFragment.DeathLocations);
			}
			//Transform.GetMutableTransform().SetScale3D(FVector(0.001f,0.001f,0.001f));
			//Transform.GetMutableTransform().SetLocation(FVector(0,0,-9999));
			
			SmbSubsystem.DestroyDelayed(MassStateTreeContext.GetEntity(), 3.8f);
		}
	}

	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);
	//SignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
	//	UE::Mass::Signals::StandTaskFinished, MassStateTreeContext.GetEntity(), 99.f);

	return EStateTreeRunStatus::Running;
}




FAttackWithSkill::FAttackWithSkill()
{
	bShouldCallTick = true;
}

bool FAttackWithSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(SmbSubsystemHandle);
	Linker.LinkExternalData(AnimationFragmentHandle);
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	Linker.LinkExternalData(MassCrowdRepHandle);
	Linker.LinkExternalData(TeamFragmentHandle);
	Linker.LinkExternalData(NearEnemiesFragHandle);;
	Linker.LinkExternalData(MassRepFragmentHandle);
	Linker.LinkExternalData(LocationDataHandle);
	Linker.LinkExternalData(AbilityDataHandle);
	return true;
}

void FAttackWithSkill::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(EntityTransformHandle);
	Builder.AddReadWrite(SmbSubsystemHandle);
	Builder.AddReadWrite(AnimationFragmentHandle);
	Builder.AddReadWrite(MassCrowdRepHandle);
	Builder.AddReadWrite(MassRepFragmentHandle);
	Builder.AddReadWrite(TeamFragmentHandle);
	Builder.AddReadWrite(NearEnemiesFragHandle);
	Builder.AddReadWrite(MassSignalSubsystemHandle);
	Builder.AddReadWrite(LocationDataHandle);
	Builder.AddReadWrite(AbilityDataHandle);
}

EStateTreeRunStatus FAttackWithSkill::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	FAbilityDataFragment& AbilityDataFragment = Context.GetExternalData(AbilityDataHandle);
	if (!IsValid(InstanceData.SkillData))
	{
		return EStateTreeRunStatus::Failed;
	}
	if (AbilityDataFragment.IsAttacking) return EStateTreeRunStatus::Running;
	AbilityDataFragment.CurrentAbility = InstanceData.SkillData.Get();
	AbilityDataFragment.IsAttacking = true;
	AbilityDataFragment.TimeInAttack = 0.f;
	AbilityDataFragment.CurrentCooldown = 0.f;
	AbilityDataFragment.TimesHit = 0;

	FAnimationFragment& AnimationFragment = Context.GetExternalData(AnimationFragmentHandle);
	AnimationFragment.CurrentState = InstanceData.SkillData->AnimationState;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAttackWithSkill::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	FAbilityDataFragment& AbilityDataFragment = Context.GetExternalData(AbilityDataHandle);
	if (AbilityDataFragment.IsAttacking)
	{
		return EStateTreeRunStatus::Running;
	}
	AbilityDataFragment.TimeInAttack = 0.f;
	AbilityDataFragment.IsAttacking = false;
	USmbAbilityData* Ability = AbilityDataFragment.CurrentAbility;
	if (AbilityDataFragment.TimeInAttack >= Ability->TimeUntilHit+Ability->RecoveryTime)
	{
		AbilityDataFragment.IsAttacking = false;
		AbilityDataFragment.TimeInAttack = 0.f;
		AbilityDataFragment.TimesHit = 0;
	}
	return EStateTreeRunStatus::Succeeded;
}



FListenEnemy::FListenEnemy()
{
	bShouldCallTick = true;
}

bool FListenEnemy::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	Linker.LinkExternalData(NearEnemiesFragHandle);
	return true;
}

void FListenEnemy::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(MassSignalSubsystemHandle);
	Builder.AddReadWrite(NearEnemiesFragHandle);
}

EStateTreeRunStatus FListenEnemy::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);

	FNearEnemiesFragment& NearEnemiesFragment = Context.GetExternalData(NearEnemiesFragHandle);
	
	FListenEnemyInstanceData& InstanceData = Context.GetInstanceData(*this);
	
	if (InstanceData.Signal == Smb::Signals::FoundEnemy)
	{
		if (NearEnemiesFragment.ClosestEnemies.Num() <= 0)
		{
			InstanceData.Enemy = FSmbEntityData(-1,-1);
			return EStateTreeRunStatus::Running;
		}
		InstanceData.Enemy = FSmbEntityData(NearEnemiesFragment.ClosestEnemies[0]);
	}
	
	SignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
		InstanceData.Signal, MassStateTreeContext.GetEntity(), 0.0f);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FListenEnemy::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);
	FNearEnemiesFragment& NearEnemiesFragment = Context.GetExternalData(NearEnemiesFragHandle);
	
	FListenEnemyInstanceData& InstanceData = Context.GetInstanceData(*this);
	
	if (InstanceData.Signal == Smb::Signals::FoundEnemy)
	{
		if (NearEnemiesFragment.ClosestEnemies.Num() <= 0)
		{
			InstanceData.Enemy = FSmbEntityData(-1,-1);
			return EStateTreeRunStatus::Running;
		}
		InstanceData.Enemy = FSmbEntityData(NearEnemiesFragment.ClosestEnemies[0]);
	}

	//UE_LOG(LogTemp, Display, TEXT("FListenEnemy::Tick, found : %i, %i"),InstanceData.Enemy.Index,InstanceData.Enemy.SerialNumber);
	
	return EStateTreeRunStatus::Running;
}




FListenerTask::FListenerTask()
{
	bShouldCallTick = true;
}

bool FListenerTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	return true;
}

void FListenerTask::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(MassSignalSubsystemHandle);
}

EStateTreeRunStatus FListenerTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);
	
	FListenerTaskInstanceData& InstanceData  = Context.GetInstanceData(*this);
	
	SignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
		InstanceData.Signal, MassStateTreeContext.GetEntity(), 99999.0f);
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FListenerTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);
	
	FListenerTaskInstanceData& InstanceData = Context.GetInstanceData(*this);

	//UE_LOG(LogTemp, Display, TEXT("FListenerTask::Tick, Tick ongoing"));
	if (InstanceData.Signal == Smb::Signals::MoveTargetChanged)
	{
		//UE_LOG(LogTemp, Display, TEXT("FListenerTask::Tick, MoveTargetChanged"));
		FLocationDataFragment* DataFragment = MassStateTreeContext.GetEntityManager().GetFragmentDataPtr<FLocationDataFragment>(MassStateTreeContext.GetEntity());
		if (DataFragment)
		{
			if (DataFragment->bNewLocation)
			{
				//UE_LOG(LogTemp, Display, TEXT("FListenerTask::Tick, new location"));
				DataFragment->bNewLocation = false;
				InstanceData.bNewData = true;
			}
		}
	}

	SignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
		InstanceData.Signal, MassStateTreeContext.GetEntity(), 999999.0f);
	
	return EStateTreeRunStatus::Running;
}



FCheckHealthListener::FCheckHealthListener()
{
	bShouldCallTick = true;
}

bool FCheckHealthListener::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	Linker.LinkExternalData(DefenceHandle);
	Linker.LinkExternalData(EntityTransformHandle);
	Linker.LinkExternalData(AnimationFragmentHandle);
	Linker.LinkExternalData(DeathPhysicsHandle);
	Linker.LinkExternalData(MassRepFragmentHandle);
	Linker.LinkExternalData(MassCrowdRepHandle);
	Linker.LinkExternalData(SmbSubsystemHandle);
	return true;
}

void FCheckHealthListener::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(MassSignalSubsystemHandle);
	Builder.AddReadWrite(DefenceHandle);
	Builder.AddReadWrite(EntityTransformHandle);
	Builder.AddReadWrite(AnimationFragmentHandle);
	Builder.AddReadWrite(DeathPhysicsHandle);
	Builder.AddReadWrite(MassRepFragmentHandle);
	Builder.AddReadWrite(MassCrowdRepHandle);
	Builder.AddReadWrite(SmbSubsystemHandle);
}

EStateTreeRunStatus FCheckHealthListener::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);
	FDefenceFragment& DefenceFragment = Context.GetExternalData(DefenceHandle);
	
	FCheckHealthListenerInstanceData& InstanceData = Context.GetInstanceData(*this);

	InstanceData.CurHealth = DefenceFragment.HP;
	
	SignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
		Smb::Signals::ReceivedDamage, MassStateTreeContext.GetEntity(), 999999.0f);
	//UE_LOG(LogTemp, Display, TEXT("FCheckHealthListener::EnterState"));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FCheckHealthListener::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FMassStateTreeExecutionContext& MassStateTreeContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	UMassSignalSubsystem& SignalSubsystem = Context.GetExternalData(MassSignalSubsystemHandle);
	FDefenceFragment& DefenceFragment = Context.GetExternalData(DefenceHandle);

	//FGameplayTagContainer EventTag = FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Skill.Type.Melee")));
	//FStateTreeEvent TreeEvent(EventTag.First());
	//FStateTreeSharedEvent SharedTreeEvent = FStateTreeSharedEvent(TreeEvent);
	//Context.ConsumeEvent(SharedTreeEvent);

	FCheckHealthListenerInstanceData& InstanceData = Context.GetInstanceData(*this);

	//UE_LOG(LogTemp, Display, TEXT("FCheckHealthListener::Tick, Alive?"));
	FTransformFragment& Transform = Context.GetExternalData(EntityTransformHandle);
	InstanceData.CurHealth = DefenceFragment.HP;
	if (DefenceFragment.HP <= 0.f && Transform.GetTransform().GetLocation() != FVector(0,0,-9999))
	{
		if (!DefenceFragment.IsAlive) return EStateTreeRunStatus::Running;
		DefenceFragment.IsAlive = false;
		//UE_LOG(LogTemp, Display, TEXT("FCheckHealthListener::Tick, Dead"));
		FAnimationFragment& AnimationFragment = Context.GetExternalData(AnimationFragmentHandle);
		USmbSubsystem& SmbSubsystem = Context.GetExternalData(SmbSubsystemHandle);
		if (AnimationFragment.CurrentState == EAnimationState::Dead) return EStateTreeRunStatus::Running;
		AnimationFragment.CurrentState = EAnimationState::Dead;
		FDeathPhysicsSharedFragment& DeathFragment = Context.GetExternalData(DeathPhysicsHandle);
		
	
		DeathFragment.TotalDeaths += 1;
		DeathFragment.DeathLocations.Add(Transform.GetTransform().GetLocation());
		
		
		UMassCrowdRepresentationSubsystem& CrowdRepSubsystem = Context.GetExternalData(MassCrowdRepHandle);
		FMassRepresentationFragment& MassRepresentationFragment = Context.GetExternalData(MassRepFragmentHandle);
		
		if (MassRepresentationFragment.StaticMeshDescHandle.IsValid())
		{
			FMassInstancedStaticMeshInfoArrayView ISMInfosView = CrowdRepSubsystem.GetMutableInstancedStaticMeshInfos();
			FMassInstancedStaticMeshInfo ISMInfo = ISMInfosView[MassRepresentationFragment.StaticMeshDescHandle.ToIndex()];
			if (MassRepresentationFragment.CurrentRepresentation == EMassRepresentationType::HighResSpawnedActor ||
				MassRepresentationFragment.CurrentRepresentation == EMassRepresentationType::LowResSpawnedActor)
			{
				SmbSubsystem.DetatchActorSetHealth(MassStateTreeContext.GetEntity());
			} else
			{
				SmbSubsystem.NewDeath(ISMInfo.GetDesc().Meshes[0].Mesh->GetName()
					,DeathFragment.TotalDeaths
					,DeathFragment.DeathLocations);
			}
			//Transform.GetMutableTransform().SetScale3D(FVector(0.001f,0.001f,0.001f));
			//Transform.GetMutableTransform().SetLocation(FVector(0,0,-9999));
			
			SmbSubsystem.DestroyDelayed(MassStateTreeContext.GetEntity(),0.11f);
			return EStateTreeRunStatus::Running;
		}
	}

	SignalSubsystem.DelaySignalEntityDeferred(MassStateTreeContext.GetMassEntityExecutionContext(),
		Smb::Signals::ReceivedDamage, MassStateTreeContext.GetEntity(), 99999.0f);
	
	return EStateTreeRunStatus::Running;
}

FChangeState::FChangeState()
{
	bShouldCallTick = false;
}


bool FChangeState::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(StateFragmentHandle);
	return true;
}

void FChangeState::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(StateFragmentHandle);
}

EStateTreeRunStatus FChangeState::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FStateFragment& StateFragment = Context.GetExternalData(StateFragmentHandle);

	FChangeStateInstanceData InstanceData = Context.GetInstanceData(*this); 
	StateFragment.SetAggressionState(InstanceData.NewState);

	return EStateTreeRunStatus::Succeeded;
}


bool FGetState::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(StateFragmentHandle);
	return true;
}

void FGetState::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(StateFragmentHandle);
}

EStateTreeRunStatus FGetState::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FStateFragment& StateFragment = Context.GetExternalData(StateFragmentHandle);

	FGetStateInstanceData& InstanceData = Context.GetInstanceData(*this); 
	EAggressionState CurrentState =  StateFragment.GetCurrentAggressionState();

	InstanceData.CurrentState = CurrentState;

	return EStateTreeRunStatus::Succeeded;
}


FNewWalkTarget::FNewWalkTarget()
{
	bShouldCallTick = false;
}

bool FNewWalkTarget::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(LocationDataFragHandle);
	Linker.LinkExternalData(TransformFragmentHandle);
	Linker.LinkExternalData(SmbSubsystemHandle);
	return true;
}


void FNewWalkTarget::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite(LocationDataFragHandle);
	Builder.AddReadWrite(TransformFragmentHandle);
	Builder.AddReadOnly(SmbSubsystemHandle);
}

EStateTreeRunStatus FNewWalkTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FLocationDataFragment& LocationDataFragment = Context.GetExternalData(LocationDataFragHandle);
	FVector NewLocation = LocationDataFragment.WalkToLocation;

	FNewNavTargetInstanceData InstanceData = Context.GetInstanceData(*this);
	

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Context.GetWorld()))
	{
		FNavLocation NavLocation;
		if (NavSys->ProjectPointToNavigation(NewLocation, NavLocation, FVector(InstanceData.RadiusNear, InstanceData.RadiusNear, 2500.0f)))
		{
			LocationDataFragment.WalkToLocation = NavLocation.Location;
		}
		else // Failed fallback
		{
			if (LocationDataFragment.OldLocation.IsNearlyZero())
			{
				FTransformFragment& TransformFragment = Context.GetExternalData(TransformFragmentHandle);
				LocationDataFragment.WalkToLocation = TransformFragment.GetTransform().GetLocation();
			} else
			{
				LocationDataFragment.WalkToLocation = LocationDataFragment.OldLocation;	
			}
		}
	}
	else
	{
		LocationDataFragment.WalkToLocation = LocationDataFragment.OldLocation;
	}

	FMassTargetLocation WalkToTarget = FMassTargetLocation();
	WalkToTarget.EndOfPathIntent = EMassMovementAction::Stand;
	WalkToTarget.EndOfPathPosition = LocationDataFragment.WalkToLocation;
	InstanceData.WalkToTarget = WalkToTarget;

	return EStateTreeRunStatus::Succeeded;
}




