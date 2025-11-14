// Copyright 2025 Land Chaunax


#include "SmbMassAgentComponent.h"

#include "SmbFragments.h"


// Sets default values for this component's properties
USmbMassAgentComponent::USmbMassAgentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USmbMassAgentComponent::BeginPlay()
{
	Super::BeginPlay();
	
	EntityManager = UE::Mass::Utils::GetEntityManager(GetWorld());
	// ...
	
}


// Called every frame
void USmbMassAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

float USmbMassAgentComponent::GetHealth() const
{
	if (!EntityManager) return -2.f;
	if (FDefenceFragment* DefenceFrag = EntityManager->GetFragmentDataPtr<FDefenceFragment>(AgentHandle))
	{
		return DefenceFrag->HP;
	}
	return -1.f;
}

