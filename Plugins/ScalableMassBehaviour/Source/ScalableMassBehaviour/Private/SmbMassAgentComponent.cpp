// Copyright © 2025 Land Chaunax, All rights reserved.


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
	if (!EntityManager) return -1.f;
	if (!EntityManager->IsEntityValid(AgentHandle)) return -1.f;
	if (!EntityManager->IsEntityBuilt(AgentHandle)) return -1.f;
	if (FDefenceFragment* DefenceFrag = EntityManager->GetFragmentDataPtr<FDefenceFragment>(AgentHandle))
	{
		return DefenceFrag->HP;
	}
	return -1.f;
}

bool USmbMassAgentComponent::SetTeam(int32 NewTeam)
{
	if (!EntityManager) return false;
	if (!EntityManager->IsEntityBuilt(AgentHandle)) return false;
	if (!EntityManager->IsEntityValid(AgentHandle)) return false;
	FTeamFragment* TeamFragment = EntityManager->GetFragmentDataPtr<FTeamFragment>(AgentHandle);
	if (!TeamFragment) return false;
	TeamFragment->TeamID = NewTeam;
	return true;
}


