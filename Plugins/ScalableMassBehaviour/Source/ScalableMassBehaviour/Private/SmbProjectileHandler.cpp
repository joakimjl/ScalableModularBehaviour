// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbProjectileHandler.h"

#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "SmbSubsystem.h"
#include "Engine/World.h"

// Sets default values
ASmbProjectileHandler::ASmbProjectileHandler()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ASmbProjectileHandler::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	
}

// Called every frame
void ASmbProjectileHandler::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//TODO fix so array doesn't have to be so large...
bool ASmbProjectileHandler::AddPhysicsParticles(TArray<FVector> SpawnVectors, TArray<FVector> TargetVectors, int32 SpawnCount, float Speed, TArray<FVector> &OutVel)
{
	bool bSuccess = true;
	OutVel.Empty();
	//OutPosArray.Empty();
	//OutVelArray.Empty();
	for (int i = 0; i < SpawnCount; ++i)
	{
		FVector OutLaunchVelocity = FVector::ZeroVector;
		FVector ProjectileLocation = SpawnVectors[i];
		FVector ProjectileEndLocation = TargetVectors[i];
		
		UGameplayStatics::FSuggestProjectileVelocityParameters Params = UGameplayStatics::FSuggestProjectileVelocityParameters(
			this,
			ProjectileLocation,
			ProjectileEndLocation,
			Speed);
		Params.ResponseParam = FCollisionResponseParams::DefaultResponseParam;
		//Ignore all collisions on calculations
		Params.ResponseParam.CollisionResponse.SetAllChannels(ECR_Ignore);
		if (!UGameplayStatics::SuggestProjectileVelocity(Params,OutLaunchVelocity))
		{
			bSuccess = false;
		} else //Add projectile to the NiagaraSystem with predicted velocity.
		{
			//UE_LOG(LogTemp, Display, TEXT("Velocity: %s"), *OutLaunchVelocity.ToString());
			OutVelArray.Add(OutLaunchVelocity);
			OutPosArray.Add(ProjectileLocation);
			PrevSpawnCount += 1;
		}
	}
	if (bSuccess)
	{
		OutVel = OutVelArray;
		PostParticle();
	}
	/*
	{
		FName PositionArray = "SpawnLocations";
		FName Integer = "InTotalSpawned";

		if (NiagaraComponent && NiagaraComponent->IsRegistered())
		{
			NiagaraComponent->SetVariableInt(Integer,SpawnCount);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
				NiagaraComponent,
				PositionArray,
				SpawnVectors
			);
		}
	} */
	return bSuccess;
}

TArray<FVector> ASmbProjectileHandler::GetPositions(FName Name)
{
	TArray<FVector> PositionArray = TArray<FVector>();
	if (NiagaraComponent && NiagaraComponent->IsRegistered())
	{
		PositionArray = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayPosition(NiagaraComponent,Name);
	}

	return PositionArray;
}

void ASmbProjectileHandler::SetPositions(TArray<FVector> Positions)
{
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NiagaraComponent, PositionName, Positions);
	OutPosArray = Positions;
}


