// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbNiagaraContainer.h"
#include "NiagaraComponent.h"


// Sets default values
ASmbNiagaraContainer::ASmbNiagaraContainer()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	if (!SceneComponent) SceneComponent = CreateDefaultSubobject<USceneComponent>("Scene");
	SetRootComponent(SceneComponent.Get());
	if (!NiagaraComponent) NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("NiagaraComp");
}

// Called when the game starts or when spawned
void ASmbNiagaraContainer::BeginPlay()
{
	Super::BeginPlay();
	NiagaraComponent->OnSystemFinished.AddDynamic(this, &ASmbNiagaraContainer::OnNiagaraFinished);
}

// Called every frame
void ASmbNiagaraContainer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASmbNiagaraContainer::OnNiagaraFinished(UNiagaraComponent* FinishedComponent)
{
	Destroy();
}