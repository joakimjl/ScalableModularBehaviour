// Copyright © 2025 Land Chaunax, All rights reserved.


#include "ScalableMassBehaviour/Replication/Public/SmbReplicator.h"

#include "MassExecutionContext.h"
#include "MassCrowdReplicator.h"
#include "MassCrowdBubble.h"
#include "ScalableMassBehaviour/Replication/Public/SmbReplicatedAgent.h"
#include "ScalableMassBehaviour/Replication/Public/SmbMassClientBubbleInfo.h"

//----------------------------------------------------------------------//
//  SmbReplicator
//----------------------------------------------------------------------//
void USmbReplicator::AddRequirements(FMassEntityQuery& EntityQuery)
{
    FSmbReplicationProcessorWalkTargetHandler::AddRequirements(EntityQuery);
}

void USmbReplicator::ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE

    FSmbReplicationProcessorWalkTargetHandler WalkTargetHandler;
    FMassReplicationSharedFragment* RepSharedFrag = nullptr;

    auto CacheViewsCallback = [&RepSharedFrag, &WalkTargetHandler](FMassExecutionContext& Context)
    {
        WalkTargetHandler.CacheFragmentViews(Context);
        RepSharedFrag = &Context.GetMutableSharedFragment<FMassReplicationSharedFragment>();
        check(RepSharedFrag);
    };

    auto AddEntityCallback = [&RepSharedFrag, &WalkTargetHandler](FMassExecutionContext& Context, const int32 EntityIdx, FSmbReplicatedUnitAgent& InReplicatedAgent, const FMassClientHandle ClientHandle) -> FMassReplicatedAgentHandle
    {
        ASmbUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ASmbUnitClientBubbleInfo>(ClientHandle);

        WalkTargetHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedMoveTargetDataMutable());

        return UnitBubbleInfo.GetUnitSerializer().Bubble.AddAgent(Context.GetEntity(EntityIdx), InReplicatedAgent);
    };

    auto ModifyEntityCallback = [&RepSharedFrag, &WalkTargetHandler](FMassExecutionContext& Context, const int32 EntityIdx, const EMassLOD::Type LOD, const double Time, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
    {
        ASmbUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ASmbUnitClientBubbleInfo>(ClientHandle);

        auto& Bubble = UnitBubbleInfo.GetUnitSerializer().Bubble;

        WalkTargetHandler.ModifyEntity<FSmbUnitFastArrayItem>(Handle, EntityIdx, Bubble.GetTargetPositionHandlerMutable());
    };

    auto RemoveEntityCallback = [&RepSharedFrag](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
    {
        ASmbUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ASmbUnitClientBubbleInfo>(ClientHandle);

        UnitBubbleInfo.GetUnitSerializer().Bubble.RemoveAgentChecked(Handle);
    };

    CalculateClientReplication<FSmbUnitFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}