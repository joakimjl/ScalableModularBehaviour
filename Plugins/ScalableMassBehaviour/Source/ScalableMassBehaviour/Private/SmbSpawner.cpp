// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbSpawner.h"


// Sets default values
ASmbSpawner::ASmbSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASmbSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASmbSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TArray<FMassEntityHandle>& ASmbSpawner::GetLatestSpawned()
{
	TArray<FMassEntityHandle> EmptyArray = TArray<FMassEntityHandle>();
	if (AllSpawnedEntities.Num() <= 0) return EmptyArray;
	return AllSpawnedEntities.Last().Entities;
}


