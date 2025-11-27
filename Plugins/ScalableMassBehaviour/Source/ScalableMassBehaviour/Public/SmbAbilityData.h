// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScalableMassBehaviour.h"
#include "Sound/SoundBase.h"
#include "SmbAbilityData.generated.h"

class UNiagaraSystem;
/**
 * 
 */
UCLASS()
class SCALABLEMASSBEHAVIOUR_API USmbAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/* Damage with Ability **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	float Damage = 10.f;

	/* Range of ability in cm **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	float Range = 100.f;

	/* Area of effect radius in CM **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AreaOfEffect = 100.f;

	/* Cooldown of ability in seconds, 0 is none. **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	float Cooldown = 1.f;

	/* Attack rate of ability, times per second.
	 * This will modify animation play speed **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	float AttackRate = 1.f;

	/* The animation to be used for ability **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	EAnimationState AnimationState = EAnimationState::Attacking;

	/* How long to animate before attack trigger, based on base animation speed **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	float TimeUntilHit = 0.3f;

	/* How long to recover after, also based on base animation speed **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	float RecoveryTime = 0.2f;

	/* To be implemented */
	UPROPERTY(EditAnywhere, Category = "Smb")
	float ManaCost = 0.f;

	/* Vfx to spawn on hit */
	UPROPERTY(EditAnywhere, Category = "Smb")
	TSoftObjectPtr<UNiagaraSystem> AbilityVfx = TSoftObjectPtr<UNiagaraSystem>();

	/* Sound effect to play */
	UPROPERTY(EditAnywhere, Category = "Smb")
	TSoftObjectPtr<USoundBase> AbilitySound = TSoftObjectPtr<USoundBase>();

	/* Attack is a projectile (Note Ranged not fully finished) */
	UPROPERTY(EditAnywhere, Category = "Smb")
	bool bIsProjectile = false;

	/* If ability uses a projectile, what speed should it have */
	UPROPERTY(EditAnywhere, Category = "Smb", meta=(EditCondition = "bIsProjectile"))
	float Speed = 0.f;
	
	/* Projectile Mesh (not used yet) */
	UPROPERTY(EditAnywhere, Category = "Smb", meta=(EditCondition = "bIsProjectile"))
	TObjectPtr<class UStaticMesh> ProjectileMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	FGameplayTagContainer AbilityTagContainer;
};
