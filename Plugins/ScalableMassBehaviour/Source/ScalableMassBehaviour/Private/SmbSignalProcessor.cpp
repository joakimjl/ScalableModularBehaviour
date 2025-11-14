// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbSignalProcessor.h"
#include "Engine/World.h"
#include "MassSignalSubsystem.h"
#include "ScalableMassBehaviour.h"
#include "MassEntityManager.h"
#include "SmbTasksAndConditions.h"


void USmbSignalProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::InitializeInternal(Owner, EntityManager);

	UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(Owner.GetWorld());

	SubscribeToSignal(*SignalSubsystem, Smb::Signals::AttackFinished);
	SubscribeToSignal(*SignalSubsystem, Smb::Signals::FoundEnemy);
	SubscribeToSignal(*SignalSubsystem, Smb::Signals::ReceivedDamage);
	SubscribeToSignal(*SignalSubsystem, Smb::Signals::MoveTargetChanged);

	UE_LOG(LogTemp, Display, TEXT("Signal Processor Initialized"));
}
