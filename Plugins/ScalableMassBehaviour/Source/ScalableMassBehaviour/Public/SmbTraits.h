// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScalableMassBehaviour.h"
#include "MassEntityTraitBase.h"
#include "MassEntityElementTypes.h"
#include "SmbFragments.h"

#include "SmbTraits.generated.h"



UCLASS()
class USmbCarryResourceTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
	
	UPROPERTY(EditAnywhere, Category = "Smb")
	FResourceFragment InResourceFragment;
};


UCLASS()
class USmbAnimTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

	UPROPERTY(EditAnywhere, Category = "Smb")
	FVertexAnimations InVertexFrag;
	
protected:
	UPROPERTY(EditAnywhere, Category = "Smb")
	FAnimationFragment InAnimationFragment;
	
};

UCLASS()
class USmbDefenceTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
	
	UPROPERTY(EditAnywhere, Category = "Smb")
	FDeathPhysicsSharedFragment InSharedDeathFragment;
	
protected:
	
	UPROPERTY(EditAnywhere, Category = "Smb")
	FDefenceFragment InDefenceFragment;
};


UCLASS()
class USmbStandardAttackTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Smb")
	FAttackFragment InAttackFragment;

	UPROPERTY(EditAnywhere, Category = "Smb")
	FNearEnemiesFragment InNearEnemiesFragment;
};

UCLASS()
class USmbExistingTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

	UPROPERTY(EditAnywhere, Category = "Smb")
	FTeamFragment InTeamFragment;

protected:
	UPROPERTY(EditAnywhere, Category = "Smb")
	FLocationDataFragment InDataFragment;

	UPROPERTY(EditAnywhere, Category = "Smb");
	FCollisionDataFragment InCollisionFrag;

	UPROPERTY(EditAnywhere, Category = "Smb");
	FHeightFragment InHeightFrag;
};

UCLASS()
class USmbAbilitiesTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
public:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Smb")
	FAbilityDataFragment InAbilityDataFragment;
};


