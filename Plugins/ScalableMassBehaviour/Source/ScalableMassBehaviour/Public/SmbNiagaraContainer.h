// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SmbNiagaraContainer.generated.h"

class UNiagaraComponent;

UCLASS()
class SCALABLEMASSBEHAVIOUR_API ASmbNiagaraContainer : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASmbNiagaraContainer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "01_Smb")
	TObjectPtr<USceneComponent> SceneComponent = TObjectPtr<USceneComponent>();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "01_Smb")
	TObjectPtr<UNiagaraComponent> NiagaraComponent = TObjectPtr<UNiagaraComponent>();

	UFUNCTION()
	void OnNiagaraFinished(UNiagaraComponent* FinishedComponent);
};
