// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleInfoBase.h"
#include "MassReplicationTransformHandlers.h"
#include "MassClientBubbleHandler.h"
#include "MassEntityView.h"
#include "SmbReplicatedAgent.h"

#include "SmbMassClientBubbleInfo.generated.h"

class SCALABLEMASSBEHAVIOUR_API FSmbUnitClientBubbleHandler : public TClientBubbleHandlerBase<FSmbUnitFastArrayItem>
{
public:
    typedef TClientBubbleHandlerBase<FSmbUnitFastArrayItem> Super;
    typedef TMassClientBubbleTransformHandler<FSmbUnitFastArrayItem> FMassClientBubbleTransformHandler;
    typedef TSmbClientTargetPositionHandler<FSmbUnitFastArrayItem> FSmbClientTargetPositionHandler;

    //Transform Handler and also Location Data Handler
    FSmbUnitClientBubbleHandler()
        : TransformHandler(*this),
        TargetPositionHandler(*this)
    {}

#if UE_REPLICATION_COMPILE_SERVER_CODE
    const FMassClientBubbleTransformHandler& GetTransformHandler() const { return TransformHandler; }
    FMassClientBubbleTransformHandler& GetTransformHandlerMutable() { return TransformHandler; }

    const FSmbClientTargetPositionHandler& GetTargetPositionHandler() const { return TargetPositionHandler; }
    FSmbClientTargetPositionHandler& GetTargetPositionHandlerMutable() { return TargetPositionHandler; }
#endif // UE_REPLICATION_COMPILE_SERVER_CODE


protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
    virtual void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) override;
    virtual void PostReplicatedChange(const TArrayView<int32> ChAMSedIndices, int32 FinalSize) override;

    void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FSmbReplicatedUnitAgent& Item) const;
#endif //UE_REPLICATION_COMPILE_CLIENT_CODE


    FMassClientBubbleTransformHandler TransformHandler;
    FSmbClientTargetPositionHandler TargetPositionHandler;
};

/** Mass client bubble, there will be one of these per client and it will handle replicating the fast array of Agents between the server and clients */
USTRUCT()
struct SCALABLEMASSBEHAVIOUR_API FSmbUnitClientBubbleSerializer : public FMassClientBubbleSerializerBase
{
    GENERATED_BODY()

    FSmbUnitClientBubbleSerializer()
    {
        Bubble.Initialize(Units, *this);
    };

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FSmbUnitFastArrayItem, FSmbUnitClientBubbleSerializer>(Units, DeltaParams, *this);
    }

public:
    FSmbUnitClientBubbleHandler Bubble;

protected:
    /** Fast Array of Agents for efficient replication. Maintained as a freelist on the server, to keep index consistency as indexes are used as Handles into the Array
     *  Note array order is not guaranteed between server and client so handles will not be consistent between them, FMassNetworkID will be.
     */
    UPROPERTY(Transient)
    TArray<FSmbUnitFastArrayItem> Units;
};

template<>
struct TStructOpsTypeTraits<FSmbUnitClientBubbleSerializer> : public TStructOpsTypeTraitsBase2<FSmbUnitClientBubbleSerializer>
{
    enum
    {
        WithNetDeltaSerializer = true,
        WithCopy = false,
    };
};

/**
 *  This class will allow us to replicate Mass data based on the fidelity required for each player controller. There is one AMassReplicationActor per PlayerController and
 *  which is also its owner.
 */
UCLASS()
class SCALABLEMASSBEHAVIOUR_API ASmbUnitClientBubbleInfo : public AMassClientBubbleInfoBase
{
    GENERATED_BODY()

public:
    ASmbUnitClientBubbleInfo(const FObjectInitializer& ObjectInitializer);

    FSmbUnitClientBubbleSerializer& GetUnitSerializer() { return UnitSerializer; }

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UPROPERTY(Replicated, Transient)
    FSmbUnitClientBubbleSerializer UnitSerializer;
};