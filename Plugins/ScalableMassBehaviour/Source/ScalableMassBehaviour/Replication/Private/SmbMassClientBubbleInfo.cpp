// Copyright © 2025 Land Chaunax, All rights reserved.


#include "ScalableMassBehaviour/Replication/Public/SmbMassClientBubbleInfo.h"

#include "MassEntityManager.h"
#include "Net/UnrealNetwork.h"
#include "MassExecutionContext.h"


#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FSmbUnitClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
    {
        TargetPositionHandler.AddRequirementsForSpawnQuery(InQuery);
    };

    auto CacheFragmentViewsForSpawnQuery = [this](FMassExecutionContext& InExecContext)
    {
        TargetPositionHandler.CacheFragmentViewsForSpawnQuery(InExecContext);
    };

    auto SetSpawnedEntityData = [this](const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& ReplicatedEntity, const int32 EntityIdx)
    {
        TargetPositionHandler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicatedMoveTargetData());
    };

    auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& Item)
    {
        PostReplicatedChangeEntity(EntityView, Item);
    };

    PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

    TargetPositionHandler.ClearFragmentViewsForSpawnQuery();
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FSmbUnitClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& Item)
    {
        PostReplicatedChangeEntity(EntityView, Item);
    };

    PostReplicatedChangeHelper(ChangedIndices, SetModifiedEntityData);
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FSmbUnitClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& Item) const
{
    TargetPositionHandler.SetModifiedEntityData(EntityView, Item.GetReplicatedMoveTargetData());
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

ASmbUnitClientBubbleInfo::ASmbUnitClientBubbleInfo(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Serializers.Add(&UnitSerializer);
}

void ASmbUnitClientBubbleInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams SharedParams;
    SharedParams.bIsPushBased = true;

    // Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it.
    DOREPLIFETIME_WITH_PARAMS_FAST(ASmbUnitClientBubbleInfo, UnitSerializer, SharedParams);
}


/*

#include "ScalableMassBehaviour/Replication/Public/SmbMassClientBubbleInfo.h"

#include "MassEntityManager.h"
#include "Net/UnrealNetwork.h"
#include "MassExecutionContext.h"


#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FSmbUnitClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
    {
        TransformHandler.AddRequirementsForSpawnQuery(InQuery);
    };

    auto CacheFragmentViewsForSpawnQuery = [this](FMassExecutionContext& InExecContext)
    {
        TransformHandler.CacheFragmentViewsForSpawnQuery(InExecContext);
    };

    auto SetSpawnedEntityData = [this](const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& ReplicatedEntity, const int32 EntityIdx)
    {
        TransformHandler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicatedPositionYawData());
    };

    auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& Item)
    {
        PostReplicatedChangeEntity(EntityView, Item);
    };

    PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

    TransformHandler.ClearFragmentViewsForSpawnQuery();
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FSmbUnitClientBubbleHandler::FSmbUnitClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& Item)
    {
        PostReplicatedChangeEntity(EntityView, Item);
    };

    PostReplicatedChangeHelper(ChangedIndices, SetModifiedEntityData);
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FSmbUnitClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& Item) const
{
    TransformHandler.SetModifiedEntityData(EntityView, Item.GetReplicatedPositionYawData());
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

ASmbUnitClientBubbleInfo::ASmbUnitClientBubbleInfo(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Serializers.Add(&UnitSerializer);
}

void ASmbUnitClientBubbleInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams SharedParams;
    SharedParams.bIsPushBased = true;

    // Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it.
    DOREPLIFETIME_WITH_PARAMS_FAST(ASmbUnitClientBubbleInfo, UnitSerializer, SharedParams);
} */