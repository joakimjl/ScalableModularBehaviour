// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbBakeUnit.h"
#include "MassSpawner.h"
#include "MassEntityTemplateRegistry.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "UObject/SavePackage.h"
#include "MassEntityConfigAsset.h"
#include "EditorUtilityActorComponent.h"
#include "MassCrowdVisualizationTrait.h"
#endif

#include "MassCrowdRepresentationSubsystem.h"
#include "ScalableMassBehaviour.h"
#include "SmbSpawner.h"
#include "SmbTraits.h"
#include "Animation/AnimSequence.h"
#include "Misc/DefinePrivateMemberPtr.h"


DEFINE_LOG_CATEGORY_STATIC(LogBlueprintFuncLibrary, Log, All);

USmbBlueprintLibrary::USmbBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
UMaterialInstanceConstant* USmbBlueprintLibrary::CreateMaterialInstance(FString AssetPath, FString AssetName, UMaterial* ParentMaterial)
{
	if (!ParentMaterial) return nullptr;

	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = ParentMaterial;
	
	FString PackageName = AssetPath + "/" + AssetName;

	//Delete old if there is one
    UPackage* ExistingPackage = FindPackage(nullptr, *PackageName);
    if (ExistingPackage)
    {
    	ExistingPackage->SetFlags(RF_Transient);
    	ExistingPackage->ClearFlags(RF_Standalone);
    	ExistingPackage = nullptr;
    }
	
	FString UniqueAssetName;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(
		AssetName,
		AssetPath,
		UMaterialInstanceConstant::StaticClass(),
		Factory
	);
	
	if (NewAsset)
	{
		NewAsset->SetFlags(RF_Standalone | RF_Public);

		UPackage* Package = NewAsset->GetPackage();
		Package->MarkPackageDirty();

		FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone | RF_Public;
		SaveArgs.SaveFlags = SAVE_NoError;
		
		const bool bSucceeded = UPackage::SavePackage(Package, NewAsset, *FilePath, SaveArgs);
		UE_LOG(LogTemp, Log, TEXT("Created Material Instance: %s, save: %i"), *NewAsset->GetName(), bSucceeded);
		return static_cast<UMaterialInstanceConstant*>(NewAsset);
	}
	return nullptr;
}

bool USmbBlueprintLibrary::SetAnimationsInConfig(UMassEntityConfigAsset* MassEntityConfigAsset, TMap<EAnimationState, UAnimSequence*> AnimationMap, UStaticMesh* StaticMesh)
{
	FMassEntityConfig& Config = MassEntityConfigAsset->GetMutableConfig();
	TConstArrayView<UMassEntityTraitBase*> Traits = Config.GetTraits();

	USmbAnimTrait* AnimTraitCast = nullptr;
	USmbDefenceTrait* DefenceTraitCast = nullptr;
	UMassVisualizationTrait* CrowdVisTraitCast = nullptr;
	for (int i = 0; i < Traits.Num(); ++i)
	{
		if (!AnimTraitCast)
		{
			AnimTraitCast = Cast<USmbAnimTrait>(Traits[i]);
		}
		if (!DefenceTraitCast)
		{
			DefenceTraitCast = Cast<USmbDefenceTrait>(Traits[i]);
		}
		if (!CrowdVisTraitCast)
		{
			CrowdVisTraitCast = Cast<UMassVisualizationTrait>(Traits[i]);
		}
		//UE_LOG(LogTemp, Log, TEXT("%s"), *Traits[i]->GetName());
	}
	
	if (StaticMesh)
	{
		if (DefenceTraitCast)
		{
			DefenceTraitCast->InSharedDeathFragment.StaticMesh = StaticMesh;
			UE_LOG(LogTemp, Display, TEXT("Set static mesh in config"))
		}
		if (CrowdVisTraitCast)
		{
			// "Soft deprecated" class still used will likely need change in future updates beyond 5.7
			CrowdVisTraitCast->StaticMeshInstanceDesc.Meshes[0].Mesh = StaticMesh;
		}
	}

	if (AnimTraitCast)
	{
		AnimTraitCast->InVertexFrag.AnimSequences = AnimationMap;
		UE_LOG(LogTemp, Display, TEXT("Set animation sequences in config"))
		return true;
	}

	//if (AnimTraitCast)
	//{
	//	AnimTraitCast->InVertexFrag.AnimSequences = AnimationMap;
	//	UE_LOG(LogTemp, Warning, TEXT("Set animation sequences in config"))
	//	return true;
	//}
	
	return false;
}

#endif

bool USmbBlueprintLibrary::SetTeamAtSpawn(ASmbSpawner* SmbSpawner, int32 NewTeam)
{
	if (!SmbSpawner) return false;
	TArray<FMassEntityHandle>& Handles = SmbSpawner->GetLatestSpawned();
	FMassEntityManager* EntityManager = EntityManager = UE::Mass::Utils::GetEntityManager(SmbSpawner->GetWorld());
	if (!EntityManager) return false;
	if (Handles.Num() <= 0) return false;
	for (FMassEntityHandle Handle : Handles)
	{
		if (!EntityManager->IsEntityBuilt(Handle)) continue;
		if (!EntityManager->IsEntityValid(Handle)) continue;
		FTeamFragment* TeamFragment = EntityManager->GetFragmentDataPtr<FTeamFragment>(Handle);
		if (!TeamFragment) continue;
		TeamFragment->TeamID = NewTeam;
	}
	return true;
}

float USmbBlueprintLibrary::GetAnimationFramerate(UAnimSequence* AnimSequence)
{
	if (!AnimSequence) return 0.f;
	return AnimSequence->GetSamplingFrameRate().AsDecimal();
}
/*
void USmbBlueprintLibrary::ChangeTeam(UMassEntityConfigAsset* MassEntityConfigAsset, int32 NewTeam)
{
	FMassEntityConfig& Config = MassEntityConfigAsset->GetMutableConfig();
	TConstArrayView<UMassEntityTraitBase*> Traits = Config.GetTraits();

	USmbExistingTrait* ExistingTrait = nullptr;
	
	for (int i = 0; i < Traits.Num(); ++i)
	{
		if (!ExistingTrait)
		{
			ExistingTrait = Cast<USmbExistingTrait>(Traits[i]);
			if (ExistingTrait)
			{
				ExistingTrait->InTeamFragment.TeamID = NewTeam;
			}
		}
	}
}
*/

