// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeProcessors.h"
#include "SmbSignalProcessor.generated.h"

/**
 * 
 */
UCLASS()
class SCALABLEMASSBEHAVIOUR_API USmbSignalProcessor : public UMassStateTreeProcessor
{
	GENERATED_BODY()

protected:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;
};
