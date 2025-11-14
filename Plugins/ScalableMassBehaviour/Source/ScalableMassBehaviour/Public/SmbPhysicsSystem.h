// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "SmbPhysicsSystem.generated.h"


UCLASS()
class SCALABLEMASSBEHAVIOUR_API ASmbPhysicsSystem : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASmbPhysicsSystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "01_Smb")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem = TSoftObjectPtr<UNiagaraSystem>();

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "01_Smb")
	//TSoftObjectPtr<UStaticMesh> StaticMeshParticle = TSoftObjectPtr<UStaticMesh>();
};
