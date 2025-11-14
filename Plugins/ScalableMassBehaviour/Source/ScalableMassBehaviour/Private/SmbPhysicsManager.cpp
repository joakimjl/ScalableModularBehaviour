// Copyright © 2025 Land Chaunax, All rights reserved.

#include "SmbPhysicsManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "SmbSubsystem.h"
#include "Engine/World.h"

ASmbPhysicsManager::ASmbPhysicsManager()
{
	PrimaryActorTick.bCanEverTick = true;

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(RootComponent);
}

void ASmbPhysicsManager::BeginPlay()
{
	Super::BeginPlay();

	NiagaraComponent->Activate(true);

	this->NiagaraComponent->SetCastShadow(true);
}

void ASmbPhysicsManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bRegisteredSelf) {
		InternalDelay-=DeltaTime;
		if (InternalDelay >= 0.f) return; 
		if (GetWorld()->HasSubsystem<USmbSubsystem>()) {
			TObjectPtr<USmbSubsystem> ScaleSubsystem = GetWorld()->GetSubsystem<USmbSubsystem>();
			if (StaticMeshParticle.Get()) {
				FString NameOfMesh = StaticMeshParticle.Get()->GetName();
				ScaleSubsystem->RegisterPhysicsManager(this,NameOfMesh);
				bRegisteredSelf = true;
				UE_LOG(LogTemp, Display, TEXT("Registered Physics Manager %s"), *NameOfMesh);
				NiagaraComponent->SetVariableStaticMesh("UserStaticMesh",StaticMeshParticle.Get());
			}
		}
	}
}

void ASmbPhysicsManager::AddPhysicsParticles(TArray<FVector> InVectors, int32 SpawnCount)
{
	FName PositionArray = "SpawnLocations";
	FName Integer = "InTotalSpawned";

	TotalDead = SpawnCount;

	if (NiagaraComponent && NiagaraComponent->IsRegistered())
	{
		NiagaraComponent->SetVariableInt(Integer,SpawnCount);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
			NiagaraComponent,
			PositionArray,
			InVectors
		);
	}
}