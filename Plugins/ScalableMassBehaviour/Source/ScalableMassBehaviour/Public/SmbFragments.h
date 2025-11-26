// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScalableMassBehaviour.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationAsset.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=7 
#include "MassEntityTypes.h"
#endif

#include "SmbFragments.generated.h"

class UNiagaraSystem;
struct FMassEntityHandle;
struct FTraceHandle;

USTRUCT()
struct FResourceFragment : public FMassFragment
{
	GENERATED_BODY()

	FResourceFragment() = default;

	FResourceFragment GetValidated() const
	{
		FResourceFragment Copy = *this;
		Copy.TimeSinceGatherStart = FMath::Max(Copy.TimeSinceGatherStart, 0);	
		return Copy;
	}

	/* If this entity should receive bonus resources
	 * (Note resource gathering will be implemented to abilities soon instead) */
	UPROPERTY(EditAnywhere, Category = "Smb")
	TMap<EProcessable, float> BonusMap = TMap<EProcessable, float>();

	UPROPERTY()
	TMap<EProcessable, int32> Carrying = TMap<EProcessable, int32>();

	UPROPERTY()
	float TimeSinceGatherStart = 0.f;
};

#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=7 
template<>
struct TMassFragmentTraits<FResourceFragment>
{
	enum { AuthorAcceptsItsNotTriviallyCopyable = true };
};
#endif

USTRUCT()
struct FAnimationFragment : public FMassFragment
{
	GENERATED_BODY()

	FAnimationFragment() = default;

	FAnimationFragment GetValidated() const
	{
		FAnimationFragment Copy = *this;
		Copy.AnimationUnitScale = FMath::Max(Copy.AnimationUnitScale, KINDA_SMALL_NUMBER);
		Copy.AnimOffsetTime = FMath::Max(Copy.AnimOffsetTime, KINDA_SMALL_NUMBER);;
		Copy.AnimationSpeed = FMath::Max(Copy.AnimationSpeed, KINDA_SMALL_NUMBER);

		return Copy;
	}
	/* Current State as given from StateTree Tasks (Change this for new Task Started) */
	UPROPERTY()
	EAnimationState CurrentState = EAnimationState::Idle;

	/* Previous State that is being transitioned from (Only to be changed from Animation Processor) */
	UPROPERTY()
	EAnimationState PreviousState = EAnimationState::Idle;
	
	/* To scale Vertex Animation. Should be the same scale as unit Mesh. */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AnimationUnitScale = 1.f;
	/* Offset Timing i.e., changes timing so ISM material instance starts on the correct time */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AnimOffsetTime = 0.f;
	
	UPROPERTY()
	float TimeInCurrentAnimation = 0.f;

	UPROPERTY()
	float LerpAlpha = 0.f;

	/* How quickly the vertex animation should blend between animations */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float BlendSpeed = 5.f;
	
	UPROPERTY()
	FName AnimationName = FName("None");

	/* Blueprint actor offset to keep correct synced locations */ 
	UPROPERTY(EditAnywhere, Category = "Smb")
	FVector ActorOffset = FVector::ZeroVector;

	UPROPERTY()
	float CurrentAnimationFrame = 0.f;

	UPROPERTY()
	float PreviousAnimationFrame = 0.f;

	UPROPERTY()
	float PrevStart = 0.f;

	UPROPERTY()
	float PrevEnd = 0.f;

	/* How quick to play the vertex animation, 1 is 100% speed*/
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AnimationSpeed = 1.f;
};


USTRUCT()
struct FVertexAnimations : public FMassSharedFragment
{
	GENERATED_BODY()

	FVertexAnimations() = default;

	FVertexAnimations GetValidated() const
	{
		FVertexAnimations Copy = *this;

		return Copy;
	}

	UPROPERTY(EditAnywhere, Category = "Smb")
	TMap<EAnimationState,UAnimSequence*> AnimSequences = TMap<EAnimationState,UAnimSequence*>();
};

#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=7 
template<>
struct TMassFragmentTraits<FVertexAnimations>
{
	enum { AuthorAcceptsItsNotTriviallyCopyable = true };
};
#endif

/** Types of Armor */
UENUM(BlueprintType)
enum class EArmorType : uint8
{
	HeavyArmor UMETA(ToolTip = "Unit is using Heavy Armor, weak to piercing"),
	LightArmor UMETA(ToolTip = "Unit is using Light Armor, weak to slashing"),
	MediumArmor UMETA(ToolTip = "Unit is using Medium Armor, weak to blunt"),
	None UMETA(ToolTip = "Unit is using Normal Armor")
};

USTRUCT()
struct FStateFragment : public FMassFragment
{
	GENERATED_BODY()

	void SetAggressionState(EAggressionState NewAggressionState)
	{
		PreviousAggressionState = CurrentAggressionState;
		CurrentAggressionState = NewAggressionState;
	}

	EAggressionState GetCurrentAggressionState()
	{
		return CurrentAggressionState;
	}

	EAggressionState GetPreviousAggressionState()
	{
		return PreviousAggressionState;
	}

protected:

	UPROPERTY(EditAnywhere, Category = "Smb")
	EAggressionState CurrentAggressionState = EAggressionState::Passive;

	UPROPERTY(EditAnywhere, Category = "Smb")
	EAggressionState PreviousAggressionState = EAggressionState::Passive;
};


USTRUCT()
struct FDefenceFragment : public FMassFragment
{
	GENERATED_BODY()

	FDefenceFragment() = default;

	FDefenceFragment GetValidated() const
	{
		FDefenceFragment Copy = *this;
		Copy.MaxHP = FMath::Max(Copy.MaxHP, KINDA_SMALL_NUMBER);
		Copy.HP = FMath::Max(Copy.HP, KINDA_SMALL_NUMBER);

		return Copy;
	}

	/* Max Health */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float MaxHP = 20.f;

	/* Current Health */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float HP = 20.f;

	/* Armor Type */
	UPROPERTY(EditAnywhere, Category = "Smb")
	EArmorType UnitArmor = EArmorType::MediumArmor;
};

USTRUCT()
struct FTeamFragment : public FMassFragment
{
	GENERATED_BODY()

	FTeamFragment() = default;

	FTeamFragment GetValidated() const
	{
		FTeamFragment Copy = *this;
		Copy.TeamID = FMath::Max(Copy.TeamID, -1);

		return Copy;
	}

	/* Team ID */
	UPROPERTY(EditAnywhere, Category = "Smb")
	int32 TeamID = 0;

	/* For additional visuals, functionality TBD */
	UPROPERTY(EditAnywhere, Category = "Smb")
	bool bIsSelected = false;
};


UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Normal,
	Piercing,
	Slashing,
	Blunt
};


/* Note this is a legacy fragment and not used in tasks anymore */
USTRUCT()
struct FAttackFragment : public FMassFragment
{
	GENERATED_BODY()

	FAttackFragment() = default;

	FAttackFragment GetValidated() const
	{
		FAttackFragment Copy = *this;
		Copy.AttackDamage = FMath::Max(Copy.AttackDamage, KINDA_SMALL_NUMBER);
		Copy.AttackRate = FMath::Max(Copy.AttackRate, KINDA_SMALL_NUMBER);
		Copy.AttackRange = FMath::Max(Copy.AttackRange, KINDA_SMALL_NUMBER);
		Copy.AnimationDelayUntilDamage = FMath::Max(Copy.AnimationDelayUntilDamage, KINDA_SMALL_NUMBER);
		Copy.AttackRecoveryTime = FMath::Max(Copy.AttackRecoveryTime, KINDA_SMALL_NUMBER);

		return Copy;
	}

	/* Attack damage from unit standard attack */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AttackDamage = 20.f;

	/* How long in seconds until it deals damage */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AnimationDelayUntilDamage = 0.2f;

	/* Attack Speed, in attacks per second */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AttackRate = 1.f;

	/* How long after hit does animation last (Attack Speed timer starts after this) */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AttackRecoveryTime = 0.5f;

	/* Will allow more than one damage instance (Currently WIP) */
	UPROPERTY(EditAnywhere, Category = "Smb")
	int32 DamageInstances = 1;

	UPROPERTY()
	int32 CurDamageInstance = 0;

	/* How long until it can attack again currently */
	UPROPERTY()
	float TimeLeftToAttack = 0.f;

	UPROPERTY()
	float CurrentTimeIntoTheAttack = 0.f;

	/* Attack range, as radius from location */ 
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AttackRange = 30.f;

	/* Damage Type */
	UPROPERTY(EditAnywhere, Category = "Smb")
	EDamageType DamageType = EDamageType::Normal;
};


USTRUCT()
struct FProjectileFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY()
	FVector Target = FVector::ZeroVector;
};


USTRUCT()
struct FProjectileParams : public FMassConstSharedFragment
{
	GENERATED_BODY();

	FProjectileParams GetValidated() const
	{
		FProjectileParams Copy = *this;
		Copy.InitialSpeed = FMath::Max(0.f,Copy.InitialSpeed);
		Copy.InitialDirection = Copy.InitialDirection.GetSafeNormal();
		return Copy;
	}

	UPROPERTY(EditAnywhere, Category = "Projectile")
	float InitialSpeed = 0;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	FVector InitialDirection = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, Category = "Projectile")
	EDamageType DamageType = EDamageType::Normal;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	float AreaOfEffectRadius = 10.f;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	int32 TeamId = -1;
};

USTRUCT()
struct FProjectileVis : public FMassSharedFragment
{
	GENERATED_BODY();

	/* Projectile Instanced Static Mesh */
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TObjectPtr<UInstancedStaticMeshComponent> ProjectileMeshComponent = TObjectPtr<UInstancedStaticMeshComponent>();

	//Mesh SoftPointer
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;
};

USTRUCT()
struct FProjectileTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAliveTag : public FMassTag
{
	GENERATED_BODY()
};


USTRUCT()
struct FLocationDataFragment : public FMassFragment
{
	GENERATED_BODY()

	FLocationDataFragment() = default;

	FLocationDataFragment GetValidated() const
	{
		FLocationDataFragment Copy = *this;
		Copy.BaseRefresh = FMath::Max(Copy.BaseRefresh, 0.f);
		Copy.TimeSince = FMath::Max(FMath::FRandRange(0.f,Copy.BaseRefresh), 0.f);

		return Copy;
	}

	/* Old Entity Location In Grid */
	UPROPERTY()
	FVector OldLocation = FVector::ZeroVector;

	/* Location to walk towards (Changes during runtime) */
	UPROPERTY()
	FVector WalkToLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector PrevWalkToLocationBeforeAttack = FVector::ZeroVector;

	UPROPERTY()
	bool bIsAttackLocation = false;

	/* Time Since Last */
	UPROPERTY()
	float TimeSince = 9999.f;

	/* How frequently should entity update position in the grid (Base only larger if lower LOD) */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float BaseRefresh = 1.f;

	/* How frequently should entity update position in the grid (Base only larger if lower LOD) */
	UPROPERTY()
	bool bTraceResultIn = false;

	/* How trace checks distance and time reduction multipliers */
	UPROPERTY()
	float TraceDistMulti = 1.f;

	FTraceHandle HitResultAsync;
	
	UPROPERTY()
	bool bNewLocation = false;

	/* If the entity should keep rechecking the NavMesh to interpolate height (more heavy on CPU gamethread) */ 
	UPROPERTY(EditAnywhere, Category = "Smb")
	bool bShouldRecheckNav = false;

	UPROPERTY()
	float ExponentialMove = 0.f;

	UPROPERTY()
	int32 DidNotMoveStreak = 0;
};

USTRUCT()
struct FHeightFragment : public FMassFragment
{
	GENERATED_BODY()

	FHeightFragment() = default;

	FHeightFragment GetValidated() const
	{
		FHeightFragment Copy = *this;
		Copy.HeightInterpolationSpeed = FMath::Max(Copy.HeightInterpolationSpeed, 10.f);
		Copy.BaseRefreshPeriod = FMath::Max(Copy.BaseRefreshPeriod, 0.01f);
		Copy.TimeSinceRefresh = FMath::Max(FMath::FRandRange(0,Copy.BaseRefreshPeriod), 0.f);
		Copy.DistanceAwayToCheckNavMulti = FMath::Max(Copy.DistanceAwayToCheckNavMulti, 0.5f);
		
		return Copy;
	}
	
	UPROPERTY()
	float TargetHeight = -999999999.f;

	UPROPERTY()
	float CurrentHeight = -999999999999.f;

	UPROPERTY()
	float TimeSinceRefresh = 0.f;

	/* How quickly the unit moves up or down when walking on a slope */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float HeightInterpolationSpeed = 100.f;

	/* How often it should check the ground to interpolate height */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float BaseRefreshPeriod = 0.02f;

	/* Multiplier for how far away it should check to find NavMesh to be able to walk, 1x is 200x,200y,400z (min 0.5x)
	 * If there are a lot of NavMesh not found warnings, this should be increased.
	 */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float DistanceAwayToCheckNavMulti = 3.f;
};

USTRUCT()
struct FCollisionDataFragment : public FMassFragment
{
	GENERATED_BODY()

	FCollisionDataFragment() = default;

	FCollisionDataFragment GetValidated() const
	{
		FCollisionDataFragment Copy = *this;
		Copy.CheckDelay = FMath::Max(Copy.CheckDelay, 0.f);
		Copy.TimeSinceLastCheck = FMath::Max(Copy.TimeSinceLastCheck, 0.f);
		Copy.MaxEntitiesToCheck = FMath::Max(Copy.MaxEntitiesToCheck, 1);

		return Copy;
	}

	UPROPERTY(EditAnywhere, Category = "Smb")
	float CheckDelay = 1.f;
	UPROPERTY()
	float TimeSinceLastCheck = 0.f;

	/* CURRENTLY NOT FUNCTIONAL */
	UPROPERTY(EditAnywhere, Category = "Smb")
	int32 MaxEntitiesToCheck = 5;

	TStaticArray<FMassEntityHandle, COLLISION_ARR_SIZE> ClosestEntities;
	//UPROPERTY()
	//FMassEntityHandle ClosestEntity;

	/* Weight to collide with */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float CollisionMass = 10.f;
};

USTRUCT()
struct FDeathPhysicsSharedFragment : public FMassSharedFragment
{
	GENERATED_BODY()

	FDeathPhysicsSharedFragment() = default;

	FDeathPhysicsSharedFragment GetValidated() const
	{
		FDeathPhysicsSharedFragment Copy = *this;
		Copy.TotalDeaths = FMath::Max(Copy.TotalDeaths, 0);
		return Copy;
	}

	UPROPERTY()
	TArray<FVector> DeathLocations = TArray<FVector>();

	/* Total Deaths */
	UPROPERTY()
	int32 TotalDeaths = 0;

	/* What static mesh to use in death physics Niagara System */
	UPROPERTY(EditAnywhere, Category = "Smb")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY()
	float DelayTime = 0.f;
};

#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=7 
template<>
struct TMassFragmentTraits<FDeathPhysicsSharedFragment>
{
	enum { AuthorAcceptsItsNotTriviallyCopyable = true };
};
#endif


UENUM(BlueprintType)
enum class EAbilityType : uint8
{
	MeleeSingle,
	MeleeAoe,
	RangedSingle,
	RangedAoe,
	CastSingle,
	CastAoe,
	None
};

USTRUCT()
struct FAbilityDataFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	FAbilityDataFragment() = default;

	FAbilityDataFragment GetValidated() const
	{
		FAbilityDataFragment Copy = *this;
		Copy.Cooldown = FMath::Max(Copy.Cooldown, 0);

		return Copy;
	}

	UPROPERTY(EditAnywhere, Category = "Smb")
	float Cooldown = 1.f;

	UPROPERTY(EditAnywhere, Category = "Smb")
	float AbilityRange = 100.f;

	UPROPERTY(EditAnywhere, Category = "Smb")
	float EffectRadius = 100.f;

	UPROPERTY(EditAnywhere, Category = "Smb")
	float EffectStrength = 10.f;

	UPROPERTY(EditAnywhere, Category = "Smb")
	EAbilityType AbilityType = EAbilityType::MeleeAoe;

	UPROPERTY(EditAnywhere, Category = "Smb")
	TSoftObjectPtr<UNiagaraSystem> AbilityVfx = TSoftObjectPtr<UNiagaraSystem>();

	UPROPERTY(EditAnywhere, Category = "Smb")
	EAnimationState AbilityAnimation = EAnimationState::Attacking;
	
	UPROPERTY(EditAnywhere, Category = "Smb")
	float EffectTriggerTime = 0.2f; 
};

USTRUCT()
struct FNearEnemiesFragment : public FMassFragment
{
	GENERATED_BODY()

public:

	FNearEnemiesFragment() = default;

	FNearEnemiesFragment GetValidated() const
	{
		FNearEnemiesFragment Copy = *this;
		Copy.CheckPeriod = FMath::Max(Copy.CheckPeriod, 0.02f);
		Copy.TimeSinceLastCheck = rand()*(1.f/CheckPeriod);
		Copy.AmountOfEnemies = FMath::Max(Copy.AmountOfEnemies, 1);
		
		return Copy;
	}

	/* How often to check in seconds min(0.02f) (time multiplied for lower LODS) */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float CheckPeriod = 1.5f;

	/* How far to check in cm radius */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float CheckRadius = 500.f;

	/* Array with information about close enemies */
	UPROPERTY()
	TArray<FMassEntityHandle> ClosestEnemies = TArray<FMassEntityHandle>();

	/* Timer */
	UPROPERTY()
	float TimeSinceLastCheck = 0.5f;

	/* How many enemies to be aware of max */
	UPROPERTY(EditAnywhere, Category = "Smb")
	int8 AmountOfEnemies = 5;
};

#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=7 
template<>
struct TMassFragmentTraits<FNearEnemiesFragment>
{
	enum { AuthorAcceptsItsNotTriviallyCopyable = true };
};
#endif