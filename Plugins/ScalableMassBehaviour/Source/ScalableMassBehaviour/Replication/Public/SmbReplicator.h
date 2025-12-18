// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "MassReplicationProcessor.h"
#include "SmbReplicator.generated.h"

/**
 * 
 */
UCLASS()
class SCALABLEMASSBEHAVIOUR_API USmbReplicator : public UMassReplicatorBase
{
	GENERATED_BODY()

public:
	/**
	 * Overridden to add specific entity query requirements for replication.
	 * Usually we add replication processor handler requirements.
	 */
	void AddRequirements(FMassEntityQuery& EntityQuery) override;

	/**
	 * Overridden to process the client replication.
	 * This methods should call CalculateClientReplication with the appropriate callback implementation.
	 */
	void ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext) override;
};
