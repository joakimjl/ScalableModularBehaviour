// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityConfigAsset.h"
#include "Engine/AssetManager.h"
#include "SmbAssetManager.generated.h"

UCLASS()
class SCALABLEMASSBEHAVIOUR_API USmbAssetManager : public UAssetManager
{
	GENERATED_BODY()

	public:
	UFUNCTION(BlueprintCallable, Category = "Smb")
	void Load(UMassEntityConfigAsset* EntityConfig);

	TObjectPtr<USmbGameData> GameData;

	static USmbAssetManager* Get();

	UFUNCTION(BlueprintPure, Category="Smb", meta=(WorldContext="WorldContextObject"))
	static USmbAssetManager* GetSmbAssetManager();

	protected:
	virtual void PostInitialAssetScan() override;

	template<class DataAssetType>
	TObjectPtr<DataAssetType> LoadAssetSync(const FPrimaryAssetType& PrimaryAsset)
	{
		TSharedPtr<FStreamableHandle> DataHandle = LoadPrimaryAssetsWithType(PrimaryAsset);
		if (DataHandle.IsValid())
		{
			DataHandle->WaitUntilComplete(0.f, false);
			return GetPrimaryAssetObjectClass<DataAssetType>(DataHandle->GetLoadedAsset()->GetPrimaryAssetId()).GetDefaultObject();
		}
		return nullptr;
	}

	private:
	inline static FPrimaryAssetType GameDataType{ "SmbGameData" };
};

UCLASS(BlueprintType, Blueprintable)
class SCALABLEMASSBEHAVIOUR_API USmbGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Smb")
	TObjectPtr<UMassEntityConfigAsset> UnitEntityConfig;

	// Override GetPrimaryAssetId to return the correct asset ID
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("SmbGameData", GetFName());
	}
};
