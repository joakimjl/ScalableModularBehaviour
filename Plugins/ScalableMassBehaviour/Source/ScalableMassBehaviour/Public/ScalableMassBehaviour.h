// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "Modules/ModuleManager.h"

class FScalableMassBehaviourModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scalable_Mass_Behaviour);

#define COLLISION_ARR_SIZE 4

UENUM(BlueprintType)
enum class EProcessable : uint8
{
	Gold,
	Food,
	Grass,
	Wood,
	Stone,
	Water,
	Storage,
	None
};

/** Animation States */
UENUM(BlueprintType)
enum class EAnimationState : uint8
{
	Idle UMETA(ToolTip = "Unit is currently Standing"),
	Attacking,
	Running,
	Dead,
	ProcessingResource,
	AttackingExtra1,
	AttackingExtra2
};

UENUM(BlueprintType)
enum class EAggressionState : uint8
{
	Passive UMETA(Tooltip = "Unit will not attack unless hit"),
	Pacifist UMETA(Tooltip = "Unit will never attack unless told"),
	Guard UMETA(Tooltip = "Unit will attack enemies within range"),
	Aggressive UMETA(ToolTip = "Unit will attack enemies close and chase enemies for a decent distance"),
	//SmartPassive UMETA(Tooltip = "(WIP) Unit will not attack unless hit and will try to disengage if at disadvantage"),
	//SmartGuard UMETA(Tooltip = "(WIP) Unit will Defend a location safely, attempt to stay behind walls, separate behaviours depending on melee or ranged"),
	//SmartAggressive UMETA(ToolTip = "(WIP) Unit will attack enemies close and chase enemies for a decent distance, will only engage if similar strength levels"),
	//SmartPacifist UMETA(Tooltip = "(WIP) Unit will never attack unless told and run to the base when hit")
};


/* NOTE WHEN A NEW SIGNAL IS ADDED, MAKE SURE THEY ARE IN SCALE SIGNAL PROCESSOR */
namespace Smb::Signals
{
	const FName AttackFinished = FName("AttackFinished");
	const FName ReceivedDamage = FName("ReceivedDamage");
	const FName FoundEnemy = FName("FoundEnemy");
	const FName MoveTargetChanged = FName("MoveTargetChanged");
}
