// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "SmbPhysicsManager.generated.h"

UCLASS()
class ASmbPhysicsManager : public AActor
{
	GENERATED_BODY()

public:
	ASmbPhysicsManager();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	bool bRegisteredSelf = false;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Smb")
	TSubclassOf<ASmbPhysicsManager> BP_PhysicsManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Smb")
	UNiagaraComponent* NiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	TSoftObjectPtr<UStaticMesh> StaticMeshParticle = TSoftObjectPtr<UStaticMesh>();

	UPROPERTY(BlueprintReadWrite, Category = "Smb")
	int32 TotalDead = 0;

	UFUNCTION(BlueprintCallable, Category = "Smb")
	void AddPhysicsParticles(TArray<FVector> InVectors, int32 SpawnCount);

	UPROPERTY(BlueprintReadWrite, Category = "Smb")
	float InternalDelay = 0.5f;
};