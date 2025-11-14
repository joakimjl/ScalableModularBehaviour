// Copyright © 2025 Land Chaunax, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "SmbProcessors.generated.h"

#define SCALE_API SCALABLEMASSBEHAVIOUR_API

UCLASS()
class UAnimationProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	SCALE_API UAnimationProcessor();

protected:
	SCALE_API virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	SCALE_API virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;

	float TimeAccumulator = 0.f;
};

UCLASS()
class SCALABLEMASSBEHAVIOUR_API URegisterProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	URegisterProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;
};


UCLASS()
class SCALABLEMASSBEHAVIOUR_API UCollisionProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UCollisionProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;
};


UCLASS()
class SCALABLEMASSBEHAVIOUR_API ULocateEnemy : public UMassProcessor
{
	GENERATED_BODY()

public:
	ULocateEnemy();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;
};


UCLASS()
class SCALABLEMASSBEHAVIOUR_API UProjectileProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UProjectileProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;
};


UCLASS()
class SCALABLEMASSBEHAVIOUR_API UAddDeathFragmentProcessor : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UAddDeathFragmentProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;
};

UCLASS()
class SCALABLEMASSBEHAVIOUR_API UHeightProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UHeightProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;
};

/*
UCLASS()
class SCALABLEMASSBEHAVIOUR_API UScaleProcessors : public UMassProcessor
{
	GENERATED_BODY()

public:
	UScaleProcessors();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;
};
*/

