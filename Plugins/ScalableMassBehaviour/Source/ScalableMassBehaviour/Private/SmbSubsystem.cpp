// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbSubsystem.h"
#include "MassEntityBuilder.h"
#include "MassAgentComponent.h"
#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassNavigationTypes.h"
#include "MassSignalSubsystem.h"
#include "SmbFragments.h"
#include "SmbPhysicsManager.h"
#include "SmbPhysicsSystem.h"
#include "SmbTasksAndConditions.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "EngineUtils.h"
#include "MassActorSubsystem.h"
#include "MassRepresentationFragments.h"
#include "MassSettings.h"
#include "MassBehaviorSettings.h"
#include "MassReplicationSubsystem.h"
#include "SmbSignalProcessor.h"
#include "OrientedBoxTypes.h"
#include "Engine/World.h"
#include "SmbAnimComp.h"
#include "SmbTraits.h"
#include "SmbProjectileHandler.h"
#include "AI/NavigationSystemBase.h"
#include "Kismet/GameplayStatics.h"
#include "SmbNiagaraContainer.h"
#include "NavigationSystem.h"
#include "SmbAssetManager.h"
#include "MassEntityConfigAsset.h"
#include "MassReplicationFragments.h"
#include "MassObserverNotificationTypes.h"
#include "MassEntityConfigAsset.h"
#include "Compression/lz4.h"
#include "Engine/AssetManager.h"
#include "ScalableMassBehaviour/Replication/Public/SmbMassClientBubbleInfo.h"


void USmbSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	auto* MassSubsystem = Collection.InitializeDependency<UMassEntitySubsystem>();
	check(MassSubsystem);
	Collection.InitializeDependency(UMassSignalSubsystem::StaticClass());
	EntityManagerPtr = MassSubsystem->GetMutableEntityManager().AsShared();
	UE::Mass::FEntityBuilder Builder(*EntityManagerPtr);
	EntityBuilder = &Builder;

	UMassSignalSubsystem* SignalSubsystem = GetWorld()->GetSubsystem<UMassSignalSubsystem>();
	//SignalSubsystem->GetSignalDelegateByName(UE::Mass::Signals::StandTaskFinished);
	
	Grid = NewObject<UGrid>();

	RegisteredResources = TMap<EProcessable, FProcessableArr>();

	AbilitySpawningDataArray = TArray<FAbilitySpawningData>();

#if WITH_EDITOR
	UMassSettings* Settings  = GetMutableDefault<UMassSettings>();
	UMassBehaviorSettings* BehaviorSettings = static_cast<UMassBehaviorSettings*>(Settings->ModuleSettings["Mass Behavior"]);
	if (BehaviorSettings->DynamicStateTreeProcessorClass.GetAssetName() == UMassStateTreeProcessor::StaticClass()->GetFName())
	{
		BehaviorSettings->DynamicStateTreeProcessorClass = USmbSignalProcessor::StaticClass();
	}
	Settings->SaveConfig();
#endif

	FProcessableReqArr ProcessReqArray = FProcessableReqArr();
}

void USmbSubsystem::PostInitialize()
{
	Super::PostInitialize();
	auto* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(GetWorld());
	check(ReplicationSubsystem);

	ReplicationSubsystem->RegisterBubbleInfoClass(ASmbUnitClientBubbleInfo::StaticClass());

	auto* SpawnerSubsystem = UWorld::GetSubsystem<UMassSpawnerSubsystem>(GetWorld());
	check(SpawnerSubsystem);

	/*
	if (USmbAssetManager::Get()->GameData)
	{
		UE_LOG(LogTemp, Display, TEXT("GameData loaded: %s"), *USmbAssetManager::Get()->GameData->GetName());
		if (USmbAssetManager::Get()->GameData->UnitEntityConfig)
		{
			UE_LOG(LogTemp, Display, TEXT("UnitEntityConfig: %s"), *USmbAssetManager::Get()->GameData->UnitEntityConfig->GetName());
			// Force template creation and registration
			const auto& EntityTemplate = USmbAssetManager::Get()->GameData->UnitEntityConfig->GetOrCreateEntityTemplate(*GetWorld());
			if (EntityTemplate.IsValid())
			{
				UE_LOG(LogTemp, Display, TEXT("Entity template registered successfully"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create entity template"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UnitEntityConfig is null in GameData"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GameData is null, check asset manager configuration"));
	}*/

}

bool USmbSubsystem::LoadEntityTemplate(const FPrimaryAssetId& PrimaryAssetId)
{
	LoadAssetSync<USmbGameData>(PrimaryAssetId);
	UE_LOG(LogTemp, Display, TEXT("GameData->UnitEntityConfig is %s"), *(USmbAssetManager::Get()->GameData->GetName()));
	const auto& EntityTemplate = USmbAssetManager::Get()->GameData->UnitEntityConfig->GetOrCreateEntityTemplate(*GetWorld());
	ensure(EntityTemplate.IsValid());
	return true;
}

bool USmbSubsystem::LoadEntityTemplateConfig(const UMassEntityConfigAsset* ConfigAsset)
{
	if (!ConfigAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("ConfigAsset is null"));
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("Loading entity template config %s"), *(ConfigAsset->GetName()));
	

	// Force template creation
	const auto& EntityTemplate = ConfigAsset->GetOrCreateEntityTemplate(*GetWorld());
	if (!EntityTemplate.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create entity template from config asset"));
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("Successfully loaded entity template"));
	return true;
}




void USmbSubsystem::Deinitialize()
{
	EntityBuilder = nullptr;
	EntityManagerPtr.Reset();

	RegisteredResources.Empty();
	Grid->EmptySelf();
	Grid->MarkAsGarbage();
	Grid = nullptr;
	ReqMap.EmptyMap();
	CarryingFree.Empty();
	PhysicsManagers.Empty();
	ToDestroy.Empty();
	AbilitySpawningDataArray.Empty();
	
	Super::Deinitialize();
}

void USmbSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DestroyStalledEntity(DeltaTime);
	
	for (int i = 0; i < AbilitySpawningDataArray.Num(); ++i)
	{
		FAbilitySpawningData& SpawningData = AbilitySpawningDataArray[i];
		SpawningData.Delay -= DeltaTime;
		if (SpawningData.Delay <= 0.f)
		{
			USmbAbilityData* Ability = SpawningData.AbilityData;
			TSoftObjectPtr<USoundBase> Sound = Ability->AbilitySound;
			if (Sound.IsValid())
			{
				UGameplayStatics::SpawnSoundAtLocation(GetWorld(),Sound.Get(),SpawningData.Transform.GetLocation());
			}
			TSoftObjectPtr<UNiagaraSystem> NiagaraSystem = Ability->AbilityVfx;
			if (NiagaraSystem.IsValid())
			{
				ASmbNiagaraContainer* NiagaraContainer = GetWorld()->SpawnActor<ASmbNiagaraContainer>();
				NiagaraContainer->NiagaraComponent.Get()->SetAsset(NiagaraSystem.Get());
				NiagaraContainer->NiagaraComponent->SetWorldLocation(SpawningData.Transform.GetLocation());
			}
			AbilitySpawningDataArray.RemoveAtSwap(i);
			i -= 1;
		}
	}
}

TStatId USmbSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UScaleSubsystem, STATGROUP_Tickables);
}

bool USmbSubsystem::RegisterResource(FVector Location, EProcessable Type, UMassAgentComponent* Component)
{
	if (!RegisteredResources.Contains(Type)) RegisteredResources.Add(Type, FProcessableArr());
	RegisteredResources[Type].Locations.Add(Location);
	//FMassEntityHandle New = EntityManagerPtr->ReserveEntity();
	Component->Enable();
	//Component->ClearEntityHandle();
	//Component->SetEntityHandle(New);
	RegisteredResources[Type].Handles.Add(Component->GetEntityHandle());
	if (Component->IsEntityPendingCreation()) return false;
	return true;
}

TArray<FVector> USmbSubsystem::GetResources(EProcessable Type)
{
	if (!RegisteredResources.Contains(Type)) return TArray<FVector>();
	return RegisteredResources[Type].Locations;
}

FMassEntityHandle USmbSubsystem::GetClosestResource(EProcessable Type, FVector Location)
{
	if (!RegisteredResources.Contains(Type)) return FMassEntityHandle();
	if (RegisteredResources[Type].Locations.Num() <= 0) return FMassEntityHandle();
	//UE_LOG(LogTemp, Display, TEXT("Closest Resource is %i"), static_cast<int32>(RegisteredResources[Type].Handles[0].AsNumber()));
	FMassEntityHandle ClosestHandle = RegisteredResources[Type].Handles[0];
	float MinDist = MAX_FLT;
	for (int i = 0; i < RegisteredResources[Type].Locations.Num(); ++i)
	{
		//UE_LOG(LogTemp, Display, TEXT("Closest Handle is testing... %i"), static_cast<int32>(RegisteredResources[Type].Handles[i].AsNumber()));
		FVector ResourceLocation = RegisteredResources[Type].Locations[i];
		if ( (Location-ResourceLocation).Size() >= MinDist ) continue;
		MinDist = (Location-ResourceLocation).Size();
		ClosestHandle = RegisteredResources[Type].Handles[i];
	}
	return ClosestHandle;
}

TMap<EProcessable, int32> USmbSubsystem::GetTotalStored()
{
	TMap<EProcessable, int32> TotalStored = TMap<EProcessable, int32>();
	TotalStored.Add(EProcessable::Gold,0);
	TotalStored.Add(EProcessable::Wood,0);
	TotalStored.Add(EProcessable::Stone,0);
	TotalStored.Add(EProcessable::Food,0);
	if (RegisteredResources.Num() <= 0) return TotalStored;
	for (auto StoredHandle : RegisteredResources[EProcessable::Storage].Handles)
	{
		if (!EntityManagerPtr->IsEntityValid(StoredHandle)) continue;
		if (FResourceFragment* StoredResourceFragment = EntityManagerPtr->GetFragmentDataPtr<FResourceFragment>(StoredHandle); StoredResourceFragment == nullptr)
			continue;
		for (auto& [Key, Value] : EntityManagerPtr->GetFragmentDataPtr<FResourceFragment>(StoredHandle)->Carrying)
		{
			if (!TotalStored.Contains(Key))
			{
				TotalStored.Add(Key, 0);
			}
			if (Value > 0)
				TotalStored.Add(Key,TotalStored[Key]+Value);
		}
	}
	
	return TotalStored;
}


bool USmbSubsystem::LowerResource(TMap<EProcessable, int32> CostResourceMap)
{
	TMap<EProcessable, int32> TotalStored = GetTotalStored();

	for (auto& [Key, Value] : CostResourceMap)
	{
		if (!TotalStored.Contains(Key)) return false;
		TotalStored[Key] -= Value;
		if (TotalStored[Key] < 0)
			return false;
	}
	
	for (auto StoredHandle : RegisteredResources[EProcessable::Storage].Handles)
	{
		if (!EntityManagerPtr->IsEntityValid(StoredHandle)) continue;
		if (FResourceFragment* StoredResourceFragment = EntityManagerPtr->GetFragmentDataPtr<FResourceFragment>(StoredHandle); StoredResourceFragment == nullptr)
			continue;
		for (auto& [Key, Value] : EntityManagerPtr->GetFragmentDataPtr<FResourceFragment>(StoredHandle)->Carrying)
		{
			if (!CostResourceMap.Contains(Key)) continue;
			int32 MaxLower = FMath::Min(Value,CostResourceMap[Key]);
			Value -= MaxLower;
			CostResourceMap[Key] -= MaxLower;
		}
	}
	
	return true;
}


bool USmbSubsystem::AddToEntity(FMassEntityHandle EntityHandle, EProcessable Type, int32 Amount) const
{
	//UE_LOG(LogTemp, Display, TEXT("Adding %i to %i"), Amount, static_cast<int32>(EntityHandle.AsNumber()));
	FResourceFragment* ResourceFragment = EntityManagerPtr->GetFragmentDataPtr<FResourceFragment>(EntityHandle);
	if (!ResourceFragment) return false;
	if (!ResourceFragment->Carrying.Contains(Type)) ResourceFragment->Carrying.Add(Type, 0); 
	ResourceFragment->Carrying.Add(Type,ResourceFragment->Carrying[Type]+Amount);
	
	//UE_LOG(LogTemp, Display, TEXT("Entity now has %i"), ResourceFragment->Carrying[Type]);
	
	return true;
}

bool USmbSubsystem::RemoveFromEntity(FMassEntityHandle EntityHandle, EProcessable Type, int32 Amount) const
{
	FResourceFragment* ResourceFragment = EntityManagerPtr->GetFragmentDataPtr<FResourceFragment>(EntityHandle);
	if (!ResourceFragment) return false;
	if (!ResourceFragment->Carrying.Contains(Type)) return false;
	if (ResourceFragment->Carrying[Type] < Amount) return false;
	ResourceFragment->Carrying[Type] -= Amount;

	return true;
}

int32 USmbSubsystem::GetEntityResources(EProcessable Type, FVector Location)
{
	FMassEntityHandle Handle = GetClosestResource(Type, Location);
	if (!EntityManagerPtr->IsEntityValid(Handle)) return 1;
	FResourceFragment& ResourceFragment = EntityManagerPtr->GetFragmentDataChecked<FResourceFragment>(Handle);
	//FResourceFragment* ResourceFragment = EntityManagerPtr->GetFragmentDataPtr<FResourceFragment>(Handle);
	//if (!ResourceFragment) return 0;
	int32 Total = 1;
	if (ResourceFragment.Carrying.Num() == 0) return 1;
	for (auto& [Key, Value] : ResourceFragment.Carrying)
	{
		Total += Value;
	}
	return Total;
} 

FVector USmbSubsystem::GetEntityLocation(FMassEntityHandle Handle)
{
	if (!EntityManagerPtr->IsEntityValid(Handle)) return FVector();
	return EntityManagerPtr->GetFragmentDataPtr<FTransformFragment>(Handle)->GetTransform().GetLocation();
}

FVector USmbSubsystem::GetEntityDataLocation(FSmbEntityData EntityData)
{
	FMassEntityHandle Handle = FMassEntityHandle(EntityData.Index, EntityData.SerialNumber);
	if (!EntityManagerPtr->IsEntityValid(Handle)) return FVector();
	return EntityManagerPtr->GetFragmentDataPtr<FTransformFragment>(Handle)->GetTransform().GetLocation();
}

TArray<FMassEntityHandle> USmbSubsystem::GetNumberClosestEntities(FVector Location, float Radius, int32 Amount, int32 Team)
{
	FVector2D Cell = VectorToCell(Location);
	int32 RadiusInCell = Radius/CellSize;
	int32 InsideRadius = 1+RadiusInCell;
	TArray<FMassEntityHandle> UnitArr = Grid->GetAround(Cell.X,Cell.Y,InsideRadius);
	TArray<FMassEntityHandle> ClosestArr = TArray<FMassEntityHandle>();

	TArray<TPair<float, FMassEntityHandle>> DistanceArr;
	for (auto Unit : UnitArr)
	{
		
		if (!EntityManagerPtr->IsEntityValid(Unit)) continue;

		//If team is not included all teams will be checked
		int32 OtherTeam = -2;
		if (FTeamFragment* TeamFragment = EntityManagerPtr->GetFragmentDataPtr<FTeamFragment>(Unit))
		{
			OtherTeam = TeamFragment->TeamID;
		}
		if (OtherTeam == Team) continue;
		
		FTransformFragment* TransformFragment = EntityManagerPtr->GetFragmentDataPtr<FTransformFragment>(Unit);
		if (!TransformFragment) continue;
		
		float Distance = (Location - TransformFragment->GetTransform().GetLocation()).Size();
		if (Distance <= Radius)
		{
			DistanceArr.Add(TPair<float, FMassEntityHandle>(Distance, Unit));
		}
	}
    
	DistanceArr.Sort([](const TPair<float, FMassEntityHandle>& A, const TPair<float, FMassEntityHandle>& B) {
		return A.Key < B.Key;
	});
    
	int32 ResultCount = FMath::Min(Amount, DistanceArr.Num());
	for (int32 i = 0; i < ResultCount; i++)
	{
		ClosestArr.Add(DistanceArr[i].Value);
	}
    
	return ClosestArr;
}

bool USmbSubsystem::RegisterPhysicsManager(ASmbPhysicsManager* InScalePhysicsManager, FString MeshName)
{
	FPhysicsManagerStruct PhysicsManager = FPhysicsManagerStruct();

	PhysicsManager.PhysicsManagerPtr = InScalePhysicsManager;
	PhysicsManager.MeshName = MeshName;

	UE_LOG(LogTemp, Display, TEXT("Registering physics manager"));
	
	UWorld* World = GetWorld();
	for (TActorIterator<ASmbPhysicsSystem> It(World); It; ++It)
	{
		ASmbPhysicsSystem* PhysicsSystem = *It;
		UE_LOG(LogTemp, Display, TEXT("Found physics system"));
		if (!PhysicsSystem->NiagaraSystem)
		{
			PhysicsSystem->NiagaraSystem.LoadSynchronous();
		}
		if (PhysicsSystem->NiagaraSystem)
		{
			auto NiagaraSoftPtr = PhysicsSystem->NiagaraSystem.Get();
			InScalePhysicsManager->NiagaraComponent->SetAsset(NiagaraSoftPtr);
		}
	}
	
	PhysicsManagers.Add(PhysicsManager);
	return true;
}

bool USmbSubsystem::RegisterProjectileManager(ASmbProjectileHandler* InProjectileManager, int32& OutId)
{
	if (InProjectileManager == nullptr) return false;
	if (ProjectileHandlerArray.Contains(InProjectileManager)) return false;
	OutId = ProjectileHandlerArray.Num();
	ProjectileHandlerArray.Add(InProjectileManager);
	return true;
}


void USmbSubsystem::NewDeath(FString MeshName, int32 TotalDeaths, TArray<FVector> Locations)
{
	//TArray<FVector> DeathLocations = TArray<FVector>();
	//DeathLocations.Add((Locations[Locations.Num()-1]+FVector::UpVector*10.f));
	for (auto PhysicsManagerStruct : PhysicsManagers)
	{
		if (PhysicsManagerStruct.MeshName == MeshName){
			PhysicsManagerStruct.PhysicsManagerPtr->AddPhysicsParticles(Locations, TotalDeaths);
			return;
		}
	}
}


FSmbEntityData USmbSubsystem::GetClosestEnemy(FVector Location, int32 TeamId, float Radius)
{
	FVector2D Cell = VectorToCell(Location);
	int32 RadiusInCell = Radius/CellSize;
	int32 InsideRadius = 1+RadiusInCell;
	TArray<FMassEntityHandle> UnitArr = Grid->GetAround(Cell.X,Cell.Y,InsideRadius);

	float MinDist = MAX_FLT;
	FSmbEntityData ClosestData = FSmbEntityData();

	//UE_LOG(LogTemp, Warning, TEXT("Checking %i units"), UnitArr.Num());

	for (auto Unit : UnitArr)
	{
		if (!EntityManagerPtr->IsEntityValid(Unit)) continue;
		FTransformFragment* TransformFragment = EntityManagerPtr->GetFragmentDataPtr<FTransformFragment>(Unit);
		if (!TransformFragment) continue;
		FTeamFragment* TeamFragment = EntityManagerPtr->GetFragmentDataPtr<FTeamFragment>(Unit);
		if (!TeamFragment) continue;
		if (TeamFragment->TeamID == TeamId) continue;
		FDefenceFragment* DefenceFrag = EntityManagerPtr->GetFragmentDataPtr<FDefenceFragment>(Unit);
		if (!DefenceFrag) continue;
		if (DefenceFrag->HP <= 0) continue;
		float Dist = (Location-TransformFragment->GetTransform().GetLocation()).Size();
		if (Dist >= MinDist) continue;
		MinDist = Dist;
		ClosestData.SerialNumber = Unit.SerialNumber;
		ClosestData.Index = Unit.Index;
	}
	return ClosestData;
}

void USmbSubsystem::SetProjectileLocations(TArray<FVector> Positions)
{
	ProjectileHandlerArray[0]->SetPositions(Positions);
}

bool USmbSubsystem::DealDamageToEnemy(FSmbEntityData TargetData, float DamageAmount, EDamageType DamageType)
{
	if (TargetData.SerialNumber == -1 && TargetData.Index == -1) return false;
	//UE_LOG(LogTemp, Warning, TEXT("Checking handle and is %i %i"), TargetData.Index, TargetData.SerialNumber)
	FMassEntityHandle EnemyHandle = FMassEntityHandle(TargetData.Index, TargetData.SerialNumber);
	if (!EntityManagerPtr->IsEntityValid(EnemyHandle)) return false;
	//UE_LOG(LogTemp, Warning, TEXT("Entity Valid and was %i %i"), TargetData.Index, TargetData.SerialNumber)
	FDefenceFragment* DefenceFragmentPtr = EntityManagerPtr->GetFragmentDataPtr<FDefenceFragment>(EnemyHandle);
	if (!DefenceFragmentPtr) return false;
	if (DamageType == EDamageType::Blunt && DefenceFragmentPtr->UnitArmor == EArmorType::HeavyArmor)
		DamageAmount *= 2;
	if (DamageType == EDamageType::Slashing && DefenceFragmentPtr->UnitArmor == EArmorType::LightArmor)
		DamageAmount *= 2;
	if (DamageType == EDamageType::Piercing && DefenceFragmentPtr->UnitArmor == EArmorType::MediumArmor)
		DamageAmount *= 2;
	DefenceFragmentPtr->HP -= DamageAmount;
	//UE_LOG(LogTemp, Warning, TEXT("Dealt damage"))
	if (DefenceFragmentPtr->HP <= 0)
	{
		DefenceFragmentPtr->HP = 0;
	}
	return true;
}

FVector USmbSubsystem::RegisterToGrid(FVector NewLocation, FMassEntityHandle Handle, FVector OldLocation)
{
	if (!EntityManagerPtr->IsEntityValid(Handle)) return FVector::DownVector;
	FVector2D NewCell = VectorToCell(NewLocation);
	FVector2D OldCell = VectorToCell(OldLocation);
	Grid->RemoveAt(OldCell.X,OldCell.Y,Handle);
	Grid->AddToGrid(NewCell.X,NewCell.Y,Handle);
	return NewLocation;
}

bool USmbSubsystem::AddPhysicsManagerToWorld(TSoftObjectPtr<UStaticMesh> StaticMesh)
{
	//If Manager for mesh already exists, exit
	//UE_LOG(LogTemp, Warning, TEXT("	Name: %s"), *StaticMesh->GetName());
	for (FPhysicsManagerStruct PhysicsManager : PhysicsManagers)
	{
		if (PhysicsManager.MeshName == StaticMesh->GetName())
		{
			return false;
		}
	}
	
	ASmbPhysicsManager* NewManager = GetWorld()->SpawnActor<ASmbPhysicsManager>();
	UWorld* World = GetWorld();
	for (TActorIterator<ASmbPhysicsSystem> It(World); It; ++It)
	{
		ASmbPhysicsSystem* PhysicsSystem = *It;
		if (PhysicsSystem->NiagaraSystem)
		{
			auto NiagaraSoftPtr = PhysicsSystem->NiagaraSystem.Get();
			NewManager->NiagaraComponent->SetAsset(NiagaraSoftPtr);
			UE_LOG(LogTemp, Display, TEXT("Found Niagara System"));
		}
	}
	NewManager->StaticMeshParticle = StaticMesh;
	FPhysicsManagerStruct NewPhysicsManager = FPhysicsManagerStruct();
	NewPhysicsManager.PhysicsManagerPtr = NewManager;
	NewPhysicsManager.MeshName = StaticMesh->GetName();
	PhysicsManagers.Add(NewPhysicsManager);

	UE_LOG(LogTemp, Display, TEXT("Added physics manager for %s"), *StaticMesh->GetName());
	//static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NiagaraSystemAsset(TEXT("/ScalableMassBehaviour/NS_Smb_PhysisManager.NS_Smb_PhysisManager"));
	//NewManager->NiagaraComponent->SetAsset(NiagaraSystemAsset.Object);
	return true;
}


bool USmbSubsystem::DealDamageAoe(FVector InLocation, float Radius, float DamageAmount, EDamageType DamageType, int32 OwnTeam, int32 &AmountKilled)
{
	FVector2D CellLocation = VectorToCell(InLocation);
	TArray<FMassEntityHandle> EnemyArray = Grid->GetAround(CellLocation.X,CellLocation.Y,Radius);
	UMassSignalSubsystem* SignalSubsystem = GetWorld()->GetSubsystem<UMassSignalSubsystem>();
	TArray<FMassEntityHandle> Signaled = TArray<FMassEntityHandle>();

	for (auto EnemyHandle : EnemyArray)
	{
		if (!EntityManagerPtr->IsEntityValid(EnemyHandle)) continue;
		FTransformFragment* TransformFragment = EntityManagerPtr->GetFragmentDataPtr<FTransformFragment>(EnemyHandle);
		if (!TransformFragment) continue;
		FVector EnemyLocation = TransformFragment->GetMutableTransform().GetLocation();
		FAgentRadiusFragment* AgentRadiusFrag = EntityManagerPtr->GetFragmentDataPtr<FAgentRadiusFragment>(EnemyHandle);
		if (!AgentRadiusFrag) continue;
		if ((EnemyLocation-InLocation).Size()>Radius+AgentRadiusFrag->Radius) continue;
		FDefenceFragment* DefenceFragment = EntityManagerPtr->GetFragmentDataPtr<FDefenceFragment>(EnemyHandle);
		if (!DefenceFragment) continue;
		if (DefenceFragment->HP <= 0) continue;
		FTeamFragment* TeamFragment = EntityManagerPtr->GetFragmentDataPtr<FTeamFragment>(EnemyHandle);
		if (!TeamFragment) continue;
		if (TeamFragment->TeamID == OwnTeam) continue;
		Signaled.Add(EnemyHandle);
		if (DamageType == EDamageType::Blunt && DefenceFragment->UnitArmor == EArmorType::HeavyArmor)
			DamageAmount *= 2;
		if (DamageType == EDamageType::Slashing && DefenceFragment->UnitArmor == EArmorType::LightArmor)
			DamageAmount *= 2;
		if (DamageType == EDamageType::Piercing && DefenceFragment->UnitArmor == EArmorType::MediumArmor)
			DamageAmount *= 2;
		DefenceFragment->HP -= DamageAmount;
		if (DefenceFragment->HP <= 0)
		{
			DefenceFragment->HP = 0;
			AmountKilled += 1;
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("Signaled %i units"), Signaled.Num());
	if (Signaled.Num() <= 0) return false;
	SignalSubsystem->DelaySignalEntities(Smb::Signals::ReceivedDamage,Signaled,0.001f);
	
	return true;
}

bool USmbSubsystem::AddProjectile(FVector SpawnVector, FVector TargetVector, USmbAbilityData *Data, int32 TeamId)
{
	if (ProjectileHandlerArray.Num() <= 0) return false;
	
	TArray<FVector> OutVel = TArray<FVector>();
	for (auto ProjectileHandler : ProjectileHandlerArray)
	{
		if (ProjectileHandler)
		{
			TArray<FVector> Locations = TArray<FVector>();
			Locations.Add(SpawnVector);
			TArray<FVector> Targets = TArray<FVector>();
			Targets.Add(TargetVector);
			
			bool bSpawnedProjectile = ProjectileHandler->AddPhysicsParticles(Locations,Targets,1,Data->Speed,OutVel);
			if (!bSpawnedProjectile)
				return false;
		}
	}
	if (OutVel.Num() <= 0) return false;
	UE::Mass::FEntityBuilder Builder(*EntityManagerPtr);
	FProjectileFragment& ProjectileFragment = Builder.Add_GetRef<FProjectileFragment>();
	ProjectileFragment.Velocity = OutVel[OutVel.Num()-1];
	ProjectileFragment.Target = TargetVector;
	FTransformFragment& TransformFragment = Builder.Add_GetRef<FTransformFragment>();
	TransformFragment.GetMutableTransform().SetLocation(SpawnVector);
	
	FProjectileParams& ProjectileParams = Builder.Add_GetRef<FProjectileParams>();
	ProjectileParams.Damage = Data->Damage;
	ProjectileParams.AreaOfEffectRadius = Data->AreaOfEffect;
	ProjectileParams.TeamId = TeamId;
	Builder.Add<FProjectileTag>();
	Builder.Commit();
	
	return true;
}




void USmbSubsystem::DestroyEntity(FMassEntityHandle Handle)
{
	if (!EntityManagerPtr->IsEntityValid(Handle)) return;
	FLocationDataFragment* DataFragment = EntityManagerPtr->GetFragmentDataPtr<FLocationDataFragment>(Handle);
	FVector2D OldCell = VectorToCell(DataFragment->OldLocation);
	Grid->RemoveAt(OldCell.X,OldCell.Y,Handle);
	EntityManagerPtr->Defer().DestroyEntity(Handle);
}

bool USmbSubsystem::IsEntityValidManager(FMassEntityHandle Handle) const
{
	return EntityManagerPtr->IsEntityValid(Handle);
}

void USmbSubsystem::DestroyDelayed(FMassEntityHandle Handle, float Delay)
{
	if (ToDestroy.Contains(Handle)) return;
	float& Val = ToDestroy.Add(Handle);
	Val = Delay;
}

void USmbSubsystem::DestroyStalledEntity(float DeltaTime)
{
	if (ToDestroy.Num() <= 0) return;
	for (auto& Element : ToDestroy)
	{
		//UE_LOG(LogTemp, Display, TEXT("Destroy delay is %f"), Element.Value);
		Element.Value -= DeltaTime;
		if (Element.Value > 0.0f) continue;
		const FMassEntityHandle Handle =  Element.Key;
		DestroyEntity(Handle);
	}
}

float USmbSubsystem::FindEntityAnimationTime(USmbAnimComp* SmbComponent)
{
	if (UMassAgentComponent* MassAgent = Cast<UMassAgentComponent>(SmbComponent->GetOwner()->GetComponentByClass(UMassAgentComponent::StaticClass())))
	{
		if (!EntityManagerPtr) return -1.f;
		//UE_LOG(LogTemp, Warning, TEXT("Found agent component with %i and %i "), MassAgent->GetEntityHandle().Index, MassAgent->GetEntityHandle().SerialNumber);
		if (!EntityManagerPtr->IsEntityValid(MassAgent->GetEntityHandle())) return 1.f;
		//FAnimationFragment& AnimationFragment = EntityManagerPtr->GetFragmentDataChecked<FAnimationFragment>(MassAgent->GetEntityHandle());
		FAnimationFragment* AnimationFragmentPtr = EntityManagerPtr->GetFragmentDataPtr<FAnimationFragment>(MassAgent->GetEntityHandle());
		if (!AnimationFragmentPtr)
		{
			AActor* Actor = SmbComponent->GetOwner();
			USmbSubsystem* SmbSubsystem = GetWorld()->GetSubsystem<USmbSubsystem>();
			FSmbEntityData Closest = SmbSubsystem->GetClosestEnemy(Actor->GetActorLocation(),-1,10.f);
			AnimationFragmentPtr = EntityManagerPtr->GetFragmentDataPtr<FAnimationFragment>(FMassEntityHandle(Closest.Index,Closest.SerialNumber));
			if (!AnimationFragmentPtr) return -1.f;
		}
		
		return AnimationFragmentPtr->CurrentAnimationFrame;
	}
	return -1.f;
}

bool USmbSubsystem::MoveEntities(TArray<FSmbEntityData> Units, FVector NewLocation, int32 Team)
{
	TArray<FMassEntityHandle> Handles;
	for (auto Unit : Units)
	{
		FMassEntityHandle Handle = FMassEntityHandle(Unit.Index, Unit.SerialNumber);
		if (!EntityManagerPtr->IsEntityValid(Handle)) continue;
		Handles.Add(Handle);
	}
	if (Handles.Num() <= 0) return false;

	for (auto Handle : Handles){
		FLocationDataFragment* DataFragment = EntityManagerPtr->GetFragmentDataPtr<FLocationDataFragment>(Handle);
		if (FTeamFragment* TeamFragment = EntityManagerPtr->GetFragmentDataPtr<FTeamFragment>(Handle))
		{
			if (TeamFragment->TeamID != Team) continue;
		}
		if (FDefenceFragment* DefenceFragment = EntityManagerPtr->GetFragmentDataPtr<FDefenceFragment>(Handle))
		{
			if (DefenceFragment->HP <= 0) continue;
		}
		FVector RandomisedTargetOffset = FVector(FMath::FRand()-0.5f,FMath::FRand()-0.5f,0);

		// Nav Trace Result
		//FNavResult NavLocation = FNavLocation();
		if (FAgentRadiusFragment* RadiusFragment = EntityManagerPtr->GetFragmentDataPtr<FAgentRadiusFragment>(Handle))
		{
			FVector LocationToCheck = NewLocation + RandomisedTargetOffset*FMath::Pow(Handles.Num(),0.71f)*RadiusFragment->Radius;
			
			if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation NavLocation;
				if (NavSys->ProjectPointToNavigation(LocationToCheck, NavLocation, FVector(25.0f, 25.0f, 2500.0f)))
				{
					DataFragment->WalkToLocation = NavLocation.Location;
				}
				else // Failed fallback
				{
					DataFragment->WalkToLocation = LocationToCheck;
				}
			}
			else
			{
				DataFragment->WalkToLocation = LocationToCheck;
			}
		}
		DataFragment->bNewLocation = true;
	}
	//UE_LOG(LogTemp, Warning, TEXT("Moved %i units"), Handles.Num());

	//Signal that entities have to refresh StateTree
	UMassSignalSubsystem* SignalSubsystem = GetWorld()->GetSubsystem<UMassSignalSubsystem>();
	SignalSubsystem->SignalEntities(Smb::Signals::MoveTargetChanged,Handles);
	return true;
}


//Gets entities at location in the grid, max X and Y and min X and Y, then it appends ones with the correct team 
TArray<FSmbEntityData> USmbSubsystem::SelectEntitiesInside(FVector TopLeftLocation, FVector BottomRightLocation, int32 Team, float YawRotation)
{
	FVector2D TopLeftCell = VectorToCell(TopLeftLocation);
	FVector2D BottomRightCell = VectorToCell(BottomRightLocation);
	float Radius = FMath::Abs(TopLeftCell.Y-BottomRightCell.Y)+FMath::Abs(TopLeftCell.X-BottomRightCell.X);
	TArray<FMassEntityHandle> Handles = Grid->GetAround((TopLeftCell.X+BottomRightCell.X)/2,(TopLeftCell.Y+BottomRightCell.Y)/2,Radius);
	TArray<FSmbEntityData> SelectedEntities = TArray<FSmbEntityData>();
	for (auto Handle : Handles)
	{
		if (!EntityManagerPtr->IsEntityValid(Handle)) continue;
		if (FTeamFragment* TeamFragment = EntityManagerPtr->GetFragmentDataPtr<FTeamFragment>(Handle))
		{
			if (TeamFragment->TeamID != Team && Team != -1) continue;
		}
		FTransformFragment* TransformFragment = EntityManagerPtr->GetFragmentDataPtr<FTransformFragment>(Handle);
		if (!TransformFragment) continue;
		FVector2D EnemyLocation = FVector2D(TransformFragment->GetTransform().GetLocation().X,TransformFragment->GetTransform().GetLocation().Y);
		FVector2D TopLeft2D = FVector2D(TopLeftLocation.X,TopLeftLocation.Y);
		FVector2D BottomRight2D = FVector2D(BottomRightLocation.X,BottomRightLocation.Y);
		FVector2D RectCenter = (TopLeft2D+BottomRight2D)/2;
		FVector2D Extents = FVector2D(FMath::Abs(TopLeft2D.X-BottomRight2D.X)/2,FMath::Abs(TopLeft2D.Y-BottomRight2D.Y)/2);
		float YawRad = FMath::DegreesToRadians(YawRotation);
		FVector2D UnitAxisX(FMath::Cos(YawRad), FMath::Sin(YawRad));
		
		//2D Rotated Selection Box
		UE::Geometry::FOrientedBox2d SelectionBox = UE::Geometry::FOrientedBox2d(RectCenter, UnitAxisX, Extents);
		bool bIsInside = SelectionBox.Contains(EnemyLocation);
		if (!bIsInside) continue;
		
		if (FDefenceFragment* DefenceFragment = EntityManagerPtr->GetFragmentDataPtr<FDefenceFragment>(Handle))
		{
			if (DefenceFragment->HP <= 0) continue;
		}
		SelectedEntities.Add(FSmbEntityData(Handle));
	}

	//UE_LOG(LogTemp, Warning, TEXT("Selected %i entities"), SelectedEntities.Num());

	return SelectedEntities;
}

void USmbSubsystem::GetEntitiesLocationsAndHealth(TArray<FSmbEntityData> SelectedEntities, TArray<FVector>& Locations, TArray<float>& HealthPercentage, FVector Offset, int32 OwnTeam)
{
	Locations.Empty();
	HealthPercentage.Empty();

	TArray<FMassEntityHandle> Signaled = TArray<FMassEntityHandle>();
	for (auto EntityData : SelectedEntities)
	{
		FMassEntityHandle Handle = FMassEntityHandle(EntityData.Index, EntityData.SerialNumber);
		if (!EntityManagerPtr->IsEntityValid(Handle)) continue;
		FTransformFragment* TransformFragment = EntityManagerPtr->GetFragmentDataPtr<FTransformFragment>(Handle);
		FDefenceFragment* DefenceFragment = EntityManagerPtr->GetFragmentDataPtr<FDefenceFragment>(Handle);
		if (TransformFragment && DefenceFragment)
		{
			Locations.Add(TransformFragment->GetTransform().GetLocation() + Offset);
			HealthPercentage.Add(DefenceFragment->HP/DefenceFragment->MaxHP);
		}
	}
}


void USmbSubsystem::DamageSelectedEntities(TArray<FSmbEntityData> SelectedEntities, float Damage, int32 OwnTeam, int32 &AmountKilled)
{
	UMassSignalSubsystem* SignalSubsystem = GetWorld()->GetSubsystem<UMassSignalSubsystem>();
	TArray<FMassEntityHandle> Signaled = TArray<FMassEntityHandle>();
	for (auto EntityData : SelectedEntities)
	{
		FMassEntityHandle EnemyHandle = FMassEntityHandle(EntityData.Index, EntityData.SerialNumber);
		if (!EntityManagerPtr->IsEntityValid(EnemyHandle)) continue;
		FDefenceFragment* DefenceFragment = EntityManagerPtr->GetFragmentDataPtr<FDefenceFragment>(EnemyHandle);
		if (!DefenceFragment) continue;
		if (DefenceFragment->HP <= 0) continue;
		FTeamFragment* TeamFragment = EntityManagerPtr->GetFragmentDataPtr<FTeamFragment>(EnemyHandle);
		if (!TeamFragment) continue;
		if (TeamFragment->TeamID == OwnTeam) continue;
		Signaled.Add(EnemyHandle);
		DefenceFragment->HP -= Damage;
		if (DefenceFragment->HP <= 0)
		{
			DefenceFragment->HP = 0;
			AmountKilled += 1;
		}
		
		if (Signaled.Num() <= 0) return;
		SignalSubsystem->DelaySignalEntities(Smb::Signals::ReceivedDamage,Signaled,0.001f);
	}
}


void USmbSubsystem::DetatchActorSetHealth(FMassEntityHandle Handle)
{
	if (FMassActorFragment* ActorFrag = EntityManagerPtr->GetFragmentDataPtr<FMassActorFragment>(Handle))
	{
		if (FDefenceFragment* DefenceFrag = EntityManagerPtr->GetFragmentDataPtr<FDefenceFragment>(Handle))
		{
			USmbAnimComp* ScaleComponent = ActorFrag->Get()->FindComponentByClass<USmbAnimComp>();
			ScaleComponent->CurrentHealth = DefenceFrag->HP;
			ScaleComponent->OnHealthChange.Broadcast(ScaleComponent->CurrentHealth);
		}
		ActorFrag->ResetAndUpdateHandleMap();
	}
}

void USmbSubsystem::EditMassConfig(FMassEntityConfig& MassEntityConfig)
{


	//Save Edited Mass EntityConfig
	//MassEntityConfig.
}


void USmbSubsystem::SpawnAbilityDataDeferred(USmbAbilityData* AbilityData, const FTransform& Transform, float Delay)
{
	AbilitySpawningDataArray.Add(FAbilitySpawningData(AbilityData,Transform,Delay));
}


void USmbSubsystem::Spawn(UMassEntityConfigAsset* EntityConfig, const TArray<FVector>& Locations, int Count, int32 NewTeam)
{
	const FMassEntityTemplate& EntityTemplate = EntityConfig->GetOrCreateEntityTemplate(*GetWorld());

	TArray<FMassEntityHandle> Entities;
	if (Count < 1) return;
	if (Locations.Num() <= 0) return;
	auto CreationContext = EntityManagerPtr->BatchCreateEntities(EntityTemplate.GetArchetype(), EntityTemplate.GetSharedFragmentValues(), Count, Entities);
	TConstArrayView<FInstancedStruct> FragmentInstances = EntityTemplate.GetInitialFragmentValues();
	EntityManagerPtr->BatchSetEntityFragmentValues(CreationContext->GetEntityCollections(*EntityManagerPtr.Get()), FragmentInstances);

	for (int i = 0; i < Entities.Num(); ++i)
	{
		FMassEntityView EntityView(EntityTemplate.GetArchetype(), Entities[i]);
		auto& TransformData = EntityView.GetFragmentData<FTransformFragment>();
		FTeamFragment* TeamFragment = EntityView.GetFragmentDataPtr<FTeamFragment>();
		if (TeamFragment && NewTeam != -1) TeamFragment->TeamID = NewTeam;

		const int32 SelectedLocationIndex = int32(FMath::RandRange(0,Locations.Num()-1));
		FVector EntityLocation = Locations[SelectedLocationIndex];
		TransformData.SetTransform(FTransform(EntityLocation));
	}
}



FVector2D USmbSubsystem::VectorToCell(FVector Location)
{
	int32 X = static_cast<int32>(Location.X/CellSize);
	int32 Y = static_cast<int32>(Location.Y/CellSize);
	return FVector2D(X, Y);
}

TArray<FMassEntityHandle> UGrid::GetAt(int32 X, int32 Y)
{
	if (!XCells.Contains(X)) return TArray<FMassEntityHandle>();
	UGridCellY* CellY = XCells[X];
	if (!CellY) return TArray<FMassEntityHandle>();
	if (!CellY->YCells.Contains(Y)) return TArray<FMassEntityHandle>();
	UGridCell* Cell = CellY->YCells[Y];
	if (!Cell) return TArray<FMassEntityHandle>();
	return Cell->Handles;
}

TArray<FMassEntityHandle> UGrid::RemoveAt(int32 X, int32 Y, FMassEntityHandle ToRemoveHandle)
{
	if (!XCells.Contains(X)) return TArray<FMassEntityHandle>();
	UGridCellY* CellY = XCells[X];
	if (!CellY) return TArray<FMassEntityHandle>();
	if (!CellY->YCells.Contains(Y)) return TArray<FMassEntityHandle>();
	UGridCell* Cell = CellY->YCells[Y];
	if (!Cell) return TArray<FMassEntityHandle>();
	Cell->Handles.Remove(ToRemoveHandle);
	return Cell->Handles;
}

TArray<FMassEntityHandle> UGrid::GetAround(int32 X, int32 Y, int32 Radius)
{
	TArray<FMassEntityHandle> Handles = TArray<FMassEntityHandle>();
	for (int i = -Radius; i < Radius; ++i)
	{
		for (int j = -Radius; j < Radius; ++j)
		{
			Handles.Append(GetAt(X+i,Y+j));
		}
	}
	
	return Handles;
}

void UGrid::AddToGrid(int32 X, int32 Y, FMassEntityHandle Handle)
{
	if (!XCells.Contains(X)) XCells.Add(X, NewObject<UGridCellY>());
	UGridCellY* CellY = XCells[X];
	//if (!CellY) CellY = NewObject<UGridCellY>();
	if (!CellY->YCells.Contains(Y)) CellY->YCells.Add(Y, NewObject<UGridCell>());
	UGridCell* Cell = CellY->YCells[Y];
	//if (!Cell) Cell = NewObject<UGridCell>();
	Cell->Handles.Add(Handle);
}

void UGridCell::EmptySelf()
{
	Handles.Empty();
	this->MarkAsGarbage();
}

void UGridCellY::EmptySelf()
{
	//For pair empty pointer
	for (auto& [Key, Value] : YCells)
	{
		Value->EmptySelf();
	}
	this->MarkAsGarbage();
}

void UGrid::EmptySelf()
{
	for (auto& [Key, Value] : XCells)
	{
		Value->EmptySelf();
	}
	this->MarkAsGarbage();
}

