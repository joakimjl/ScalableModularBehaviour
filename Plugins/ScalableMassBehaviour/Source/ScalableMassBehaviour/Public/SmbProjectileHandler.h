// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmbProjectileHandler.generated.h"

class UNiagaraComponent;

UCLASS()
class SCALABLEMASSBEHAVIOUR_API ASmbProjectileHandler : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASmbProjectileHandler();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Smb")
	TArray<FVector> OutVelArray = TArray<FVector>();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Smb")
	TArray<FVector> OutPosArray = TArray<FVector>();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Smb")
	int32 PrevSpawnCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Smb")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	TSoftObjectPtr<UStaticMesh> StaticMeshParticle = TSoftObjectPtr<UStaticMesh>();

	UFUNCTION(BlueprintCallable, Category = "Smb")
	virtual bool AddPhysicsParticles(TArray<FVector> SpawnVectors, TArray<FVector> TargetVectors, int32 SpawnCount, float Speed, TArray<FVector> &OutVel);

	UFUNCTION(BlueprintCallable, Category = "Smb")
	TArray<FVector> GetPositions(FName Name = "Particles.Position Array");

	UFUNCTION(BlueprintCallable, Category = "Smb")
	void SetPositions(TArray<FVector> Positions);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Blueprint implementable Post Particle event
	UFUNCTION(BlueprintImplementableEvent, Category = "Smb")
	void PostParticle();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smb")
	FName PositionName = FName("Projectile Locations");

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
