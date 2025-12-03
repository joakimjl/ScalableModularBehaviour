// Copyright © 2025 Land Chaunax, All rights reserved.


#include "ScalableMassBehaviour/Replication/Public/SmbReplicator.h"

#include "MassExecutionContext.h"
#include "MassReplicationTransformHandlers.h"
#include "ScalableMassBehaviour/Replication/Public/SmbReplicatedAgent.h"
#include "ScalableMassBehaviour/Replication/Public/SmbMassClientBubbleInfo.h"

//----------------------------------------------------------------------//
//  SmbReplicator
//----------------------------------------------------------------------//
void USmbReplicator::AddRequirements(FMassEntityQuery& EntityQuery)
{
    FMassReplicationProcessorPositionYawHandler::AddRequirements(EntityQuery);
}

void USmbReplicator::ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE

    FMassReplicationProcessorPositionYawHandler PositionYawHandler;
    FMassReplicationSharedFragment* RepSharedFrag = nullptr;

    auto CacheViewsCallback = [&RepSharedFrag, &PositionYawHandler](FMassExecutionContext& Context)
    {
        PositionYawHandler.CacheFragmentViews(Context);
        RepSharedFrag = &Context.GetMutableSharedFragment<FMassReplicationSharedFragment>();
        check(RepSharedFrag);
    };

    auto AddEntityCallback = [&RepSharedFrag, &PositionYawHandler](FMassExecutionContext& Context, const int32 EntityIdx, FSmbReplicatedUnitAgent& InReplicatedAgent, const FMassClientHandle ClientHandle) -> FMassReplicatedAgentHandle
    {
        ASmbUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ASmbUnitClientBubbleInfo>(ClientHandle);

        PositionYawHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedPositionYawDataMutable());

        return UnitBubbleInfo.GetUnitSerializer().Bubble.AddAgent(Context.GetEntity(EntityIdx), InReplicatedAgent);
    };

    auto ModifyEntityCallback = [&RepSharedFrag, &PositionYawHandler](FMassExecutionContext& Context, const int32 EntityIdx, const EMassLOD::Type LOD, const double Time, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
    {
        ASmbUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ASmbUnitClientBubbleInfo>(ClientHandle);

        auto& Bubble = UnitBubbleInfo.GetUnitSerializer().Bubble;

        PositionYawHandler.ModifyEntity<FSmbUnitFastArrayItem>(Handle, EntityIdx, Bubble.GetTransformHandlerMutable());
    };

    auto RemoveEntityCallback = [&RepSharedFrag](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
    {
        ASmbUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ASmbUnitClientBubbleInfo>(ClientHandle);

        UnitBubbleInfo.GetUnitSerializer().Bubble.RemoveAgentChecked(Handle);
    };

    CalculateClientReplication<FSmbUnitFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}