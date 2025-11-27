// Copyright © 2025 Land Chaunax, All rights reserved.

#include "SmbTraits.h"

#include "MassCommonFragments.h"
#include "MassActorSubsystem.h"
#include "MassEntityTemplateRegistry.h"
#include "Engine/World.h"
#include "SmbSubsystem.h"


void USmbCarryResourceTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FAgentRadiusFragment>();

	FResourceFragment& ResourceFrag = BuildContext.AddFragment_GetRef<FResourceFragment>();
	ResourceFrag = InResourceFragment.GetValidated();
}

void USmbAnimTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& MassEntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FAnimationFragment& AnimRef = BuildContext.AddFragment_GetRef<FAnimationFragment>();
	AnimRef = InAnimationFragment.GetValidated();

	BuildContext.RequireFragment<FMassActorFragment>();
	const FVertexAnimations VertexFrag = InVertexFrag.GetValidated();
	const FSharedStruct& SharedVertexFrag = MassEntityManager.GetOrCreateSharedFragment<FVertexAnimations>(VertexFrag);
	BuildContext.AddSharedFragment(SharedVertexFrag);
}

void USmbDefenceTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& MassEntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FDefenceFragment& DefenceRef = BuildContext.AddFragment_GetRef<FDefenceFragment>();
	DefenceRef = InDefenceFragment.GetValidated();
	
	const FDeathPhysicsSharedFragment DeathPhysicsFragment = InSharedDeathFragment.GetValidated();
	const FSharedStruct& SharedDeathPhysicsFragment = MassEntityManager.GetOrCreateSharedFragment<FDeathPhysicsSharedFragment>(DeathPhysicsFragment);
	BuildContext.AddSharedFragment(SharedDeathPhysicsFragment);

	BuildContext.AddTag<FAliveTag>();
}

void USmbStandardAttackTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FAttackFragment& AttackRef = BuildContext.AddFragment_GetRef<FAttackFragment>();
	AttackRef = InAttackFragment.GetValidated();

	FNearEnemiesFragment& NearEnemiesFragment = BuildContext.AddFragment_GetRef<FNearEnemiesFragment>();
	NearEnemiesFragment = InNearEnemiesFragment.GetValidated();
}

void USmbExistingTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FTransformFragment>();
	FTeamFragment& TeamRef = BuildContext.AddFragment_GetRef<FTeamFragment>();
	TeamRef = InTeamFragment.GetValidated();

	BuildContext.AddFragment<FStateFragment>();

	FCollisionDataFragment& ColRef = BuildContext.AddFragment_GetRef<FCollisionDataFragment>();
	ColRef = InCollisionFrag.GetValidated();

	FLocationDataFragment& LocationRef = BuildContext.AddFragment_GetRef<FLocationDataFragment>();
	LocationRef = InDataFragment.GetValidated();

	FHeightFragment& HeightRef = BuildContext.AddFragment_GetRef<FHeightFragment>();
	HeightRef = InHeightFrag.GetValidated();
}

void USmbAbilitiesTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FTransformFragment>();

	FMassEntityManager& MassEntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	FAbilityDataFragment& AbilityDataFragment = BuildContext.AddFragment_GetRef<FAbilityDataFragment>();
	AbilityDataFragment = InAbilityDataFragment.GetValidated();
}