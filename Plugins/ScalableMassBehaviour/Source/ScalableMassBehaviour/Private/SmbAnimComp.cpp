// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbAnimComp.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AnimationCore.h"
#include "SmbBakeUnit.h"
#include "SmbSubsystem.h"
//#include "ViewportInteractionTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

// Sets default values for this component's properties
USmbAnimComp::USmbAnimComp()
{
	PrimaryComponentTick.bCanEverTick = true;

	PerformAnimationChange(0.001f);
}


// Called when the game starts
void USmbAnimComp::BeginPlay()
{
	Super::BeginPlay();
	
	PerformAnimationChange(GetWorld()->GetDeltaSeconds());
	
	// ...
	
}


// Called every frame
void USmbAnimComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	PerformAnimationChange(DeltaTime);
}

void USmbAnimComp::PerformAnimationChange(float DeltaTime)
{
	if (!bControlledFromMass) return;
	if (CurrentHealth <= 0.f) return;
	/** Sets animation to correct time if it is a mass actor **/
	if (CurrentFrame < 0.f)
	{
		if (!GetWorld()) return;
		USmbSubsystem* SmbSubsystem = GetWorld()->GetSubsystem<USmbSubsystem>();
		if (!SmbSubsystem) return;
		float Time = SmbSubsystem->FindEntityAnimationTime(this);
		//UE_LOG(LogTemp, Display, TEXT("Pre Time: %f"),Time);
		if (Time < 0.f) return;
		CurrentFrame = Time;
		//UE_LOG(LogTemp, Display, TEXT("Time"));
	}
	//UE_LOG(LogTemp, Display, TEXT("Is now %i and %i "), GetEntityHandle().SerialNumber, GetEntityHandle().Index);
	//GetOwner()->SetActorLocation(Location);
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character) return;
	if (!Character->GetMesh()) return;
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (Character->GetMesh()->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		//UE_LOG(LogTemp, Display, TEXT("Changed Mesh"));
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Character->GetCharacterMovement()->DisableMovement();
		//Mesh->SetAllBodiesBelowSimulatePhysics(Bone,true,true);
	}
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(Bone,CurBlendWeight);
	if (!Animations.Contains(AnimationType)) return;
	//if (CurBlendWeight >= 0.f) return;
	UAnimationAsset* Animation = Animations[AnimationType].Get();
	Mesh->SetAnimation(Animation);
	Mesh->SetPosition((CurrentFrame-1.f)/30.f); //TODO Get framerate to here
	Character->SetActorRotation(Rotation);
	CurBlendWeight = FMath::Min(CurBlendWeight+DeltaTime,1.f);
	//UE_LOG(LogTemp, Display, TEXT("Blend weight: %f"), CurBlendWeight);
}