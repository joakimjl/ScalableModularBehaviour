// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ScalableMassBehaviour.h"
#include "MassSubsystemBase.h"
#include "SmbFragments.h"
#include "TaskSyncManager.h"

#include "SmbSubsystem.generated.h"


struct FMassEntityConfig;
class USmbAbilityData;
class ASmbProjectileHandler;
class USmbAnimComp;
class ASmbPhysicsManager;
class UMassAgentComponent;


struct FMassEntityHandle;

namespace UE::Mass
{
	struct FEntityBuilder;
}

struct FMassEntityManager;


USTRUCT(BlueprintType)
struct FSmbEntityData
{
	GENERATED_BODY()

	FSmbEntityData() = default;

	FSmbEntityData(int32 InSerial, int32 InIndex)
	{
		SerialNumber = InSerial;
		Index = InIndex;
	}

	FSmbEntityData(const FMassEntityHandle InHandle)
	{
		SerialNumber = InHandle.SerialNumber;
		Index = InHandle.Index;
	}
	
	UPROPERTY(EditAnywhere, Category = "Smb")
	int32 SerialNumber = -1;
	
	UPROPERTY(EditAnywhere, Category = "Smb")
	int32 Index = -1;
};

USTRUCT()
struct FProcessableReqArr
{
	GENERATED_BODY()

	void EmptyArr()
	{
		TypeArr.Empty();
		AmountArr.Empty();
	}
	
	TArray<EProcessable> TypeArr = TArray<EProcessable>();
	TArray<int32> AmountArr = TArray<int32>();
};

USTRUCT()
struct FResourceReqMap
{
	GENERATED_BODY()

	void EmptyMap()
	{
		for (auto& Pair : ReqMap)
		{
			Pair.Value.EmptyArr();
		}
		ReqMap.Empty();
	}
	
	TMap<EProcessable, FProcessableReqArr> ReqMap = TMap<EProcessable, FProcessableReqArr>();
};

UCLASS()
class UGridCell : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMassEntityHandle> Handles;

	UFUNCTION()
	void EmptySelf();
};

UCLASS()
class UGridCellY : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Smb")
	TMap<int32, UGridCell*> YCells;

	void EmptySelf();
};

UCLASS()
class UGrid : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<int32, UGridCellY*> XCells = TMap<int32, UGridCellY*>();
	UFUNCTION()
	TArray<FMassEntityHandle> GetAt(int32 X, int32 Y);
	UFUNCTION()
	TArray<FMassEntityHandle> RemoveAt(int32 X, int32 Y, FMassEntityHandle ToRemoveHandle);
	UFUNCTION()
	TArray<FMassEntityHandle> GetAround(int32 X, int32 Y, int32 Radius);

	UFUNCTION()
	void AddToGrid(int32 X, int32 Y, FMassEntityHandle Handle);

	UFUNCTION()
	void EmptySelf();
};

USTRUCT()
struct FProcessableArr
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> Locations = TArray<FVector>();

	UPROPERTY()
	TArray<FMassEntityHandle> Handles = TArray<FMassEntityHandle>();
};

USTRUCT()
struct FPhysicsManagerStruct
{
	GENERATED_BODY()

	FPhysicsManagerStruct() = default;
	
	UPROPERTY()
	ASmbPhysicsManager* PhysicsManagerPtr = nullptr;

	UPROPERTY()
	FString MeshName = FString("none");
};


template<>
struct TMassExternalSubsystemTraits<USmbSubsystem>
{
	static constexpr bool GameThreadOnly = false;
	static constexpr bool ThreadSafeWrite = false;
};

/**
 * 
 */
UCLASS()
class USmbSubsystem : public UMassTickableSubsystemBase
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	//TStatid as needed:
	virtual TStatId GetStatId() const override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Smb")
	float TimeSinceRemoval = 0.f;
	UFUNCTION(BlueprintCallable, Category = "Smb")
	FVector2D VectorToCell(FVector Location);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	bool RegisterResource(FVector Location, EProcessable Type, UMassAgentComponent* Component);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	TArray<FVector> GetResources(EProcessable Type);
	UFUNCTION(Category = "Smb")
	FMassEntityHandle GetClosestResource(EProcessable Type, FVector Location);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	TMap<EProcessable, int32> GetTotalStored();
	UFUNCTION(Category = "Smb")
	bool AddToEntity(FMassEntityHandle EntityHandle, EProcessable Type, int32 Amount) const;
	UFUNCTION(Category = "Smb")
	bool RemoveFromEntity(FMassEntityHandle EntityHandle, EProcessable Type, int32 Amount) const;
	UFUNCTION(Category = "Smb")
	FVector GetEntityLocation(FMassEntityHandle Handle);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	int32 GetEntityResources(EProcessable Type, FVector Location);
	/* Deals Damage to Single Entity */
	UFUNCTION(BlueprintCallable, Category = "Smb")
	bool DealDamageToEnemy(FSmbEntityData TargetData, float DamageAmount, EDamageType DamageType);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	FVector GetEntityDataLocation(FSmbEntityData TargetData);
	UFUNCTION(Category = "Smb")
	bool IsEntityValidManager(FMassEntityHandle Handle) const;
	UFUNCTION(Category = "Smb")
	float FindEntityAnimationTime(USmbAnimComp* ScaleComponent);

	/* Gets entities inside the given square */
	UFUNCTION(BlueprintCallable, Category = "Smb")
	TArray<FSmbEntityData> SelectEntitiesInside(FVector TopLeftLocation, FVector BottomRightLocation, int32 Team = -1, float YawRotation = 0.f);

	/* Sets walk target vector for given entities */ 
	UFUNCTION(BlueprintCallable, Category = "Smb")
	bool MoveEntities(TArray<FSmbEntityData> Units, FVector NewLocation, int32 Team = -1);

	/* Deals Damage in an AOE */
	UFUNCTION(BlueprintCallable, Category = "Smb")
	bool DealDamageAoe(FVector InLocation, float Radius, float DamageAmount, EDamageType DamageType, int32 OwnTeam, int32 &AmountKilled);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	bool AddProjectile(FVector SpawnVector, FVector TargetVector, USmbAbilityData *Data, int32 TeamId = -1);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	void SetProjectileLocations(TArray<FVector> Positions);

	UFUNCTION(BlueprintCallable, Category = "Smb")
	FSmbEntityData GetClosestEnemy(FVector Location, int32 TeamId, float Radius);
	//UFUNCTION()
	//TArray<FMassEntityHandle> GetNearbyUnits(FVector Location, float Radius);
	UFUNCTION()
	TArray<FMassEntityHandle> GetNumberClosestEntities(FVector Location, float Radius, int32 Amount, int32 Team = -1);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	bool LowerResource(TMap<EProcessable, int32> CostResourceMap);

	UFUNCTION()
	FVector RegisterToGrid(FVector NewLocation, FMassEntityHandle Handle, FVector OldLocation);

	UFUNCTION(BlueprintCallable, Category = "Smb")
	bool RegisterPhysicsManager(ASmbPhysicsManager* InScalePhysicsManager, FString MeshName);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	bool RegisterProjectileManager(ASmbProjectileHandler* InProjectileManager, int32 &OutId);
	UFUNCTION(BlueprintCallable, Category = "Smb")
	void NewDeath(FString MeshName, int32 TotalDeaths, TArray<FVector> Locations);

	UFUNCTION()
	void DestroyEntity(FMassEntityHandle Handle);

	UFUNCTION()
	void DetatchActorSetHealth(FMassEntityHandle Handle);

	UFUNCTION()
	void EditMassConfig(FMassEntityConfig &MassEntityConfig);

	/* Entities given will take InDamage */
	UFUNCTION(BlueprintCallable, Category = "Smb")
	void DamageSelectedEntities(TArray<FSmbEntityData> SelectedEntities, float Damage, int32 OwnTeam, int32 &AmountKilled);

	UFUNCTION(BlueprintCallable, Category = "Smb")
	void GetEntitiesLocationsAndHealth(TArray<FSmbEntityData> SelectedEntities, TArray<FVector>& Locations, TArray<float>& HealthPercentage, FVector Offset = FVector::ZeroVector, int32 OwnTeam = -1);

	UPROPERTY()
	FResourceReqMap ReqMap = FResourceReqMap();

	UPROPERTY()
	TMap<EProcessable, FProcessableArr> RegisteredResources;

	UPROPERTY(BlueprintReadWrite, Category = "Smb")
	TMap<EProcessable, int32> CarryingFree = TMap<EProcessable, int32>();
	
	UFUNCTION()
	bool AddPhysicsManagerToWorld(TSoftObjectPtr<UStaticMesh> StaticMesh);
	UPROPERTY(EditAnywhere, Category = "Smb")
	TArray<FPhysicsManagerStruct> PhysicsManagers = TArray<FPhysicsManagerStruct>();
	UPROPERTY(EditAnywhere, Category = "Smb")
	TArray<ASmbProjectileHandler*> ProjectileHandlerArray = TArray<ASmbProjectileHandler*>();
	
protected:

	UFUNCTION()
	void DestroyStalledEntity();
	
	UPROPERTY()
	TObjectPtr<UGrid> Grid = TObjectPtr<UGrid>();

	UPROPERTY()
	TArray<FMassEntityHandle> ToDestroy = TArray<FMassEntityHandle>();
	
	FMassEntityManager* EntityManagerPtr = nullptr;
	UE::Mass::FEntityBuilder* EntityBuilder = nullptr;

	UPROPERTY()
	float CellSize = 500.f;
};

