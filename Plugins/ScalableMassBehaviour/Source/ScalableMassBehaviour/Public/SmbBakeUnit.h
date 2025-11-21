// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/ActorComponent.h"

#if WITH_EDITOR
#include "EditorUtilityActorComponent.h"
//#include "AnimToTextureBPLibrary.h"
#endif

#include "SmbBakeUnit.generated.h"


class ASmbSpawner;
class UObject;
class UMaterialInstanceConstant;
class UMaterial;
class UAnimSequence;
class UMassEntityConfigAsset;

/**
 *
 */
UCLASS(Blueprintable, Abstract, meta = (ShowWorldContextPin))
class SCALABLEMASSBEHAVIOUR_API USmbBlueprintLibrary: public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = "Smb")
	static UMaterialInstanceConstant* CreateMaterialInstance(FString AssetPath, FString AssetName, UMaterial* ParentMaterial);

	UFUNCTION(BlueprintCallable, Category = "Smb")
	static bool SetAnimationsInConfig(UMassEntityConfigAsset* MassEntityConfigAsset,
		TMap<EAnimationState, UAnimSequence*> AnimationMap, UStaticMesh* StaticMesh = nullptr);
#endif

	/* NOTE: Only use this function after the OnSpawnFinished event from MassSpawners. */
	UFUNCTION(BlueprintCallable, Category = "Smb")
	static bool SetTeamAtSpawn(ASmbSpawner* MassSpawner, int32 NewTeam);

	UFUNCTION(BlueprintCallable, Category = "Smb")
	static float GetAnimationFramerate(UAnimSequence* AnimSequence);
	
	//UFUNCTION(BlueprintCallable, Category = "Smb")
	//static void ChangeTeam(UMassEntityConfigAsset* MassEntityConfigAsset, int32 NewTeam);
	
};

