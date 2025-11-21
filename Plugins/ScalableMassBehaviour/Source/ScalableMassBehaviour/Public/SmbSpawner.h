// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassSpawner.h"
#include "SmbSpawner.generated.h"

UCLASS()
class SCALABLEMASSBEHAVIOUR_API ASmbSpawner : public AMassSpawner
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASmbSpawner();

	TArray<FMassEntityHandle>& GetLatestSpawned();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
