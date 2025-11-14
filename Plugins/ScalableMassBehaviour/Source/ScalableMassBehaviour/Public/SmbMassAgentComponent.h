// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassAgentComponent.h"
#include "SmbMassAgentComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SCALABLEMASSBEHAVIOUR_API USmbMassAgentComponent : public UMassAgentComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USmbMassAgentComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Smb")
	float GetHealth() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	FMassEntityManager* EntityManager = nullptr;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
