// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"


#include "MassReplicationTypes.h"
#include "MassClientBubbleHandler.h"
#include "MassReplicationTransformHandlers.h"
#include "SmbFragments.h"
#include "SmbReplicatedAgent.generated.h"


USTRUCT()
struct SCALABLEMASSBEHAVIOUR_API FSmbReplicatedMoveTarget
{
	GENERATED_BODY()

	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;
};

template<typename AgentArrayItem>
class TSmbClientTargetPositionHandler
{
public:
	TSmbClientTargetPositionHandler(TClientBubbleHandlerBase<AgentArrayItem>& InOwnerHandler)
		: OwnerHandler(InOwnerHandler)
	{}

#if UE_REPLICATION_COMPILE_SERVER_CODE
	/** Sets move target location for clients from the server */
	void SetBubbleMoveTargetFromLocation(const FMassReplicatedAgentHandle Handle, const FVector& Location);

	// Another function  SetBubbleTransform() could be added here if required
#endif // UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
	/**
	 * When entities are spawned in Mass by the replication system on the client, a spawn query is used to set the data on the spawned entities.
	 * The following functions are used to configure the query and then set the position and yaw data.
	 */
	static void AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery);
	void CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext);
	void ClearFragmentViewsForSpawnQuery();

	void SetSpawnedEntityData(const int32 EntityIdx, const FSmbReplicatedMoveTarget& ReplicatedMoveTarget) const;

	/** Call this when an Entity that has already been spawned is modified on the client */
	static void SetModifiedEntityData(const FMassEntityView& EntityView, const FSmbReplicatedMoveTarget& ReplicatedMoveTarget);

	// We could easily add support replicating FReplicatedAgentTransformData here if required
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
	static void SetEntityData(FLocationDataFragment& LocationDataFragment, const FSmbReplicatedMoveTarget& ReplicatedMoveTarget);
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
	TArrayView<FLocationDataFragment> LocationDataList;

	TClientBubbleHandlerBase<AgentArrayItem>& OwnerHandler;
};

#if UE_REPLICATION_COMPILE_SERVER_CODE
template<typename AgentArrayItem>
void TSmbClientTargetPositionHandler<AgentArrayItem>::SetBubbleMoveTargetFromLocation(const FMassReplicatedAgentHandle Handle, const FVector& Location)
{
	bool bMarkDirty = false;
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=7
	auto Agent = OwnerHandler.GetAgentChecked(Handle);
	FSmbReplicatedMoveTarget& ReplicatedMoveTarget = Agent.GetReplicatedMoveTargetDataMutable();
#else
	
	check(OwnerHandler.AgentHandleManager.IsValidHandle(Handle));

	const int32 AgentsIdx = OwnerHandler.AgentLookupArray[Handle.GetIndex()].AgentsIdx;

	AgentArrayItem& Item = (*OwnerHandler.Agents)[AgentsIdx];

	checkf(Item.Agent.GetNetID().IsValid(), TEXT("TSmbClientTargetPositionHandler::SetBubbleMoveTargetFromLocation, Invalid ID! First Add the Agent!"));

	// GetReplicatedMoveTargetDataMutable() must be defined in your FReplicatedAgentBase derived class
	FSmbReplicatedMoveTarget& ReplicatedMoveTarget = Item.Agent.GetReplicatedMoveTargetDataMutable();
#endif
	
	// Only update the Pos and mark the item as dirty if it has changed more than the tolerance
	const FVector Pos = Location;
	if (!Pos.Equals(ReplicatedMoveTarget.TargetLocation, UE::Mass::Replication::PositionReplicateTolerance) &&
		!Pos.Equals(FVector::ZeroVector, UE::Mass::Replication::PositionReplicateTolerance))
	{
		ReplicatedMoveTarget.TargetLocation = Pos;
		bMarkDirty = true;
	}

	/*
	const float Yaw = static_cast<float>(FMath::DegreesToRadians(Transform.GetRotation().Rotator().Yaw));

	// Only update the Yaw and mark the item as dirty if it has changed more than the tolerance
	if (FMath::Abs(FMath::FindDeltaAngleRadians(Yaw, ReplicatedPositionYaw.GetYaw())) > UE::Mass::Replication::YawReplicateTolerance)
	{
		ReplicatedPositionYaw.SetYaw(Yaw);
		bMarkDirty = true;
	}*/

#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=7
	if (bMarkDirty)
	{
		//Mark Dirty
	}
#else
	if (bMarkDirty)
	{
		OwnerHandler.Serializer->MarkItemDirty(Item);
	}
#endif
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE


#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TSmbClientTargetPositionHandler<AgentArrayItem>::AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery)
{
	//InQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	InQuery.AddRequirement<FLocationDataFragment>(EMassFragmentAccess::ReadWrite);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TSmbClientTargetPositionHandler<AgentArrayItem>::CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext)
{
	LocationDataList = InExecContext.GetMutableFragmentView<FLocationDataFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TSmbClientTargetPositionHandler<AgentArrayItem>::ClearFragmentViewsForSpawnQuery()
{
	LocationDataList = TArrayView<FLocationDataFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TSmbClientTargetPositionHandler<AgentArrayItem>::SetSpawnedEntityData(const int32 EntityIdx, const FSmbReplicatedMoveTarget& ReplicatedMoveTarget) const
{
	FLocationDataFragment& LocationDataFrag = LocationDataList[EntityIdx];

	SetEntityData(LocationDataFrag, ReplicatedMoveTarget);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE



#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TSmbClientTargetPositionHandler<AgentArrayItem>::SetModifiedEntityData(const FMassEntityView& EntityView, const FSmbReplicatedMoveTarget& ReplicatedMoveTarget)
{
	FLocationDataFragment& LocationDataFrag = EntityView.GetFragmentData<FLocationDataFragment>();

	SetEntityData(LocationDataFrag, ReplicatedMoveTarget);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TSmbClientTargetPositionHandler<AgentArrayItem>::SetEntityData(FLocationDataFragment& LocationDataFragment, const FSmbReplicatedMoveTarget& ReplicatedMoveTarget)
{
	LocationDataFragment.bNewLocation = true;
	//UE_LOG(LogTemp, Display, TEXT("New Target Location X: %f Y: %f"),ReplicatedMoveTarget.TargetLocation.X,ReplicatedMoveTarget.TargetLocation.Y);
	LocationDataFragment.NextLocation = ReplicatedMoveTarget.TargetLocation;
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE



class FSmbReplicationProcessorWalkTargetHandlerBase
{
public:
	static SCALABLEMASSBEHAVIOUR_API void AddRequirements(FMassEntityQuery& InQuery)
	{
		InQuery.AddRequirement<FLocationDataFragment>(EMassFragmentAccess::ReadWrite);
	};
	SCALABLEMASSBEHAVIOUR_API void CacheFragmentViews(FMassExecutionContext& ExecContext)
	{
		LocationDataList = ExecContext.GetMutableFragmentView<FLocationDataFragment>();
	};

protected:
	TArrayView<FLocationDataFragment> LocationDataList;
};

class FSmbReplicationProcessorWalkTargetHandler : public FSmbReplicationProcessorWalkTargetHandlerBase
{
public:
	SCALABLEMASSBEHAVIOUR_API void AddEntity(const int32 EntityIdx, FSmbReplicatedMoveTarget& InOutReplicatedPathData) const
	{
		const FLocationDataFragment& LocationDataFragment = LocationDataList[EntityIdx];
		InOutReplicatedPathData.TargetLocation = LocationDataFragment.NextLocation;
	};

	template<typename AgentArrayItem>
	void ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TSmbClientTargetPositionHandler<AgentArrayItem>& BubblePathHandler);
};

template<typename AgentArrayItem>
void FSmbReplicationProcessorWalkTargetHandler::ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TSmbClientTargetPositionHandler<AgentArrayItem>& BubblePathHandler)
{
	const FLocationDataFragment& LocationDataFragment = LocationDataList[EntityIdx];

	//UE_LOG(LogTemp, Display, TEXT("Bubble view location (server) X: %f Y:  %f"),LocationDataFragment.WalkToLocation.X,LocationDataFragment.WalkToLocation.Y);

	BubblePathHandler.SetBubbleMoveTargetFromLocation(Handle, LocationDataFragment.WalkToLocation);
}


/** The data that is replicated specific to each Crowd agent */
USTRUCT()
struct SCALABLEMASSBEHAVIOUR_API FSmbReplicatedUnitAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()

	const FReplicatedAgentPositionYawData& GetReplicatedPositionYawData() const { return PositionYaw; }
	FReplicatedAgentPositionYawData& GetReplicatedPositionYawDataMutable() { return PositionYaw; }

	const FSmbReplicatedMoveTarget& GetReplicatedMoveTargetData() const { return MoveTarget; }
	FSmbReplicatedMoveTarget& GetReplicatedMoveTargetDataMutable() { return MoveTarget; }

private:
	UPROPERTY()
	FReplicatedAgentPositionYawData PositionYaw; // replicated data

	UPROPERTY()
	FSmbReplicatedMoveTarget MoveTarget; // replicated data
};

/** Fast array item for efficient agent replication. Remember to make this dirty if any FReplicatedCrowdAgent member variables are modified */
USTRUCT()
struct SCALABLEMASSBEHAVIOUR_API FSmbUnitFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()

	FSmbUnitFastArrayItem() = default;
	FSmbUnitFastArrayItem(const FSmbReplicatedUnitAgent& InAgent, const FMassReplicatedAgentHandle InHandle)
		: FMassFastArrayItemBase(InHandle)
		, Agent(InAgent)
	{}

	/** This typedef is required to be provided in FMassFastArrayItemBase derived classes (with the associated FReplicatedAgentBase derived class) */
	using FReplicatedAgentType = FSmbReplicatedUnitAgent;

	UPROPERTY()
	FSmbReplicatedUnitAgent Agent;
};