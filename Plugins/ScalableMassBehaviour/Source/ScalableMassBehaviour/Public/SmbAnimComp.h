// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassAgentComponent.h"
#include "SmbFragments.h"
#include "Components/ActorComponent.h"
#include "SmbAnimComp.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SCALABLEMASSBEHAVIOUR_API USmbAnimComp : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USmbAnimComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void PerformAnimationChange(float DeltaTime);

	UDELEGATE(BlueprintAuthorityOnly)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealthChange, float, CurrentHealth);
	UPROPERTY(BlueprintAssignable, Category = "Smb")
	FHealthChange OnHealthChange;
	
	/** Current Frame in Anim **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	float CurrentFrame = -1.f;

	/** Is the movement controlled from Mass, or Actor **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	bool bControlledFromMass = false;

	/** Current Anim Type **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	EAnimationState AnimationType = EAnimationState::Idle;

	/** Animations to play **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	TMap<EAnimationState, TSoftObjectPtr<UAnimationAsset>> Animations;

	/** Actor Intended Location from Mass **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	FVector Location = FVector::ZeroVector;

	/** Actor Intended Rotation from Mass **/
	UPROPERTY(EditAnywhere, Category = "Smb")
	FQuat Rotation = FQuat::Identity;

	/** Root bone for physics simulation toggling **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	FName Bone = "None";

	/** BlendWeight for physics sim **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	float CurBlendWeight = 0.f;

	/** BlendWeight for physics sim **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	float CurrentHealth = 10.f;
};
