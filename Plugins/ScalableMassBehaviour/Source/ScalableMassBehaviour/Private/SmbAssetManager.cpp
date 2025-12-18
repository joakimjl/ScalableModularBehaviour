// Copyright © 2025 Land Chaunax, All rights reserved.

#include "SmbAssetManager.h"

#include "MassEntityConfigAsset.h"

void USmbAssetManager::Load(UMassEntityConfigAsset* EntityConfig)
{
	UE_LOG(LogTemp, Warning, TEXT("Loading EntityConfig %s"), *EntityConfig->GetName());
	EntityConfig->GetOrCreateEntityTemplate(*GetWorld());
}

USmbAssetManager* USmbAssetManager::Get()
{
	return &static_cast<USmbAssetManager&>(UAssetManager::Get());
}

USmbAssetManager* USmbAssetManager::GetSmbAssetManager()
{
	return Get();
}

void USmbAssetManager::PostInitialAssetScan()
{
	Super::PostInitialAssetScan();

	//GameData = LoadAssetSync<USmbGameData>(GameDataType);
	//ensure(GameData); UNUSED
}