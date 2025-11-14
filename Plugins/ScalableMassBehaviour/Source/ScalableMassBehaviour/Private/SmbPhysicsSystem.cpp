// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbPhysicsSystem.h"

#include "SmbSubsystem.h"


// Sets default values
ASmbPhysicsSystem::ASmbPhysicsSystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASmbPhysicsSystem::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASmbPhysicsSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

