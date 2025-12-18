// Copyright © 2025 Land Chaunax, All rights reserved.


#include "SmbProcessors.h"

#include "AnimationCoreLibrary.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "SmbFragments.h"
#include "MassRepresentationSubsystem.h"
#include "MassRepresentationProcessor.h"
#include "MassCrowdRepresentationSubsystem.h"
#include "SmbSubsystem.h"
#include "ScalableMassBehaviour.h"
#include "MassActorSubsystem.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassSignalSubsystem.h"
#include "NavigationSystem.h"
#include "SmbAnimComp.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UAnimationProcessor::UAnimationProcessor()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void UAnimationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAnimationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSharedRequirement<FVertexAnimations>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.RegisterWithProcessor(*this);
}

void UAnimationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	TimeAccumulator = GetWorld()->GetTimeSeconds();
	//TArray<FString> MeshNames = AnimationSubsystem.GetMeshNames();
	UMassCrowdRepresentationSubsystem* RepresentationSubsystem = Context.GetWorld()->GetSubsystem<UMassCrowdRepresentationSubsystem>();

	TArray<EAnimationState> Ordering = TArray<EAnimationState>();
	Ordering.Add(EAnimationState::Idle);
	Ordering.Add(EAnimationState::Attacking);
	Ordering.Add(EAnimationState::Running);
	Ordering.Add(EAnimationState::Dead);
	Ordering.Add(EAnimationState::ProcessingResource);
	
	TArray<FString> MeshNames;
	EntityQuery.ForEachEntityChunk(Context, [this, DeltaTime, RepresentationSubsystem, Ordering](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();
		const TConstArrayView<FMassRepresentationFragment> RepresentationFragmentArrayView = Context.GetFragmentView<FMassRepresentationFragment>();
		FMassInstancedStaticMeshInfoArrayView ISMInfosView = RepresentationSubsystem->GetMutableInstancedStaticMeshInfos();
		TArrayView<FAnimationFragment> AnimationFragmentArrayView = Context.GetMutableFragmentView<FAnimationFragment>();
		FVertexAnimations VertFrag = Context.GetSharedFragment<FVertexAnimations>();
		TConstArrayView<FMassActorFragment> ActorFragmentArrayView = Context.GetMutableFragmentView<FMassActorFragment>();
		const TConstArrayView<FTransformFragment> TransformFragmentArrayView = Context.GetFragmentView<FTransformFragment>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
		{
			FAnimationFragment& AnimationFragment = AnimationFragmentArrayView[EntityIndex];
			FMassRepresentationFragment RepresentationFragment = RepresentationFragmentArrayView[EntityIndex];
			if (!ISMInfosView.IsValidIndex(RepresentationFragment.StaticMeshDescHandle.ToIndex())) continue;
			FMassInstancedStaticMeshInfo ISMInfo = ISMInfosView[RepresentationFragment.StaticMeshDescHandle.ToIndex()];
			
			const FString CurMeshName = ISMInfo.GetDesc().Meshes[0].Mesh->GetName();
			const FMassRepresentationLODFragment& RepresentationLOD = RepresentationLODList[EntityIndex];
			const FMassActorFragment& ActorFragment = ActorFragmentArrayView[EntityIndex];
			
			float CumulativeFrames = 0;
			float StartFrame = 0;
			float EndFrame = 0;
			float Framerate = 0;

			//TODO Separate channel for Damage received animation and fragment shader (Fire Ice...).

			if (AnimationFragment.LerpAlpha > 0)
			{
				AnimationFragment.LerpAlpha = FMath::Clamp(AnimationFragment.LerpAlpha-DeltaTime*AnimationFragment.BlendSpeed,0.f,1.f);
			}
			for (int i = 0; i < Ordering.Num(); ++i)
			{
				auto Key = Ordering[i];
				if (!VertFrag.AnimSequences.Contains(Key)) continue;
				auto EleAnim = VertFrag.AnimSequences[Key];
				int32 CurrentMaxFrame = EleAnim->GetNumberOfSampledKeys();
				if (AnimationFragment.CurrentState == Key)
				{
					StartFrame = CumulativeFrames;
					EndFrame = CumulativeFrames+CurrentMaxFrame-1;
					Framerate = EleAnim->GetSamplingFrameRate().AsDecimal();
				}
				CumulativeFrames += CurrentMaxFrame;
			}
			AnimationFragment.CurrentAnimationFrame += AnimationFragment.AnimationSpeed*DeltaTime*Framerate;

			//New Animation (Note switching requires blending to be finished)
			if (AnimationFragment.CurrentState != AnimationFragment.PreviousState && AnimationFragment.LerpAlpha <= 0.f)
			{
				AnimationFragment.LerpAlpha = 1.f;
				AnimationFragment.PreviousAnimationFrame = AnimationFragment.CurrentAnimationFrame;
				AnimationFragment.CurrentAnimationFrame = 0;
				if (AnimationFragment.CurrentState == EAnimationState::Running)
				{
					AnimationFragment.CurrentAnimationFrame = FMath::RandRange(StartFrame,EndFrame);
				}
				AnimationFragment.PreviousState = AnimationFragment.CurrentState;
				AnimationFragment.TimeInCurrentAnimation = 0;
			}
			if (AnimationFragment.LerpAlpha <= 0.f)
			{
				AnimationFragment.PrevStart = StartFrame;
				AnimationFragment.PrevEnd = EndFrame;
			}
			if (RepresentationFragment.CurrentRepresentation == EMassRepresentationType::HighResSpawnedActor || RepresentationFragment.CurrentRepresentation == EMassRepresentationType::LowResSpawnedActor
				|| RepresentationFragment.PrevRepresentation == EMassRepresentationType::HighResSpawnedActor || RepresentationFragment.PrevRepresentation == EMassRepresentationType::LowResSpawnedActor)
			{
				USmbAnimComp* ScaleComponent = nullptr;
				if (ActorFragment.IsValid() && ActorFragment.IsOwnedByMass())
				{
					ScaleComponent = ActorFragment.Get()->FindComponentByClass<USmbAnimComp>();
				}
				if (ScaleComponent)
				{
					float CurrentFrame = StartFrame+FMath::Modulo(AnimationFragment.CurrentAnimationFrame,EndFrame-StartFrame);	
					ScaleComponent->CurrentFrame = CurrentFrame-StartFrame;
					ScaleComponent->AnimationType = AnimationFragment.CurrentState;
					FTransformFragment TransformFrag = TransformFragmentArrayView[EntityIndex];
					FVector Location = TransformFrag.GetTransform().GetLocation();
					if (AnimationFragment.ActorOffset == FVector::ZeroVector)
					{
						ACharacter* Character = Cast<ACharacter>(ScaleComponent->GetOwner());
						if (Character != nullptr)
						{
							AnimationFragment.ActorOffset = -Character->GetMesh()->GetRelativeLocation();
						}
					}
					ScaleComponent->Location = Location+AnimationFragment.ActorOffset;
					if (ISMInfo.GetDesc().bUseTransformOffset)
					{
						ScaleComponent->Rotation = TransformFrag.GetTransform().GetRotation();
					} else
					{
						ScaleComponent->Rotation = TransformFrag.GetTransform().GetRotation();
					}
					
					//UE_LOG(LogTemp, Warning, TEXT("Current Frame: %f"), CurrentFrame);
				}
			}
			if (RepresentationFragment.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance || RepresentationFragment.PrevRepresentation == EMassRepresentationType::StaticMeshInstance)
			{
				float CurrentFrame = 1;
				if (EndFrame != 0.f)
				{
					CurrentFrame = StartFrame+FMath::Modulo(AnimationFragment.CurrentAnimationFrame,EndFrame-StartFrame);	
				}
				float PreviousFrame = 1.f;
				if (AnimationFragment.PrevEnd != 0.f)
				{
					PreviousFrame = AnimationFragment.PrevStart+FMath::Modulo(AnimationFragment.PreviousAnimationFrame,AnimationFragment.PrevEnd-AnimationFragment.PrevStart);	
				}
				ISMInfo.AddBatchedCustomDataFloats({CurrentFrame,
					PreviousFrame,
					AnimationFragment.LerpAlpha,
					AnimationFragment.AnimationUnitScale},
					RepresentationLOD.LODSignificance, RepresentationLOD.PrevLOD);
			}
			AnimationFragment.TimeInCurrentAnimation += DeltaTime;
		}
	});
}

URegisterProcessor::URegisterProcessor()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
	bRequiresGameThreadExecution = true;
	//ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void URegisterProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLocationDataFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAnimationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<USmbSubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URegisterProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	float DeltaTime = FMath::Min(Context.GetDeltaTimeSeconds(),0.2f);

	EntityQuery.ForEachEntityChunk(Context, [this, DeltaTime](FMassExecutionContext& Context)
	{
		USmbSubsystem& SmbSubsystem = Context.GetMutableSubsystemChecked<USmbSubsystem>();
		TArrayView<FTransformFragment> TransformFragmentArrayView = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FLocationDataFragment> LocationDataFragmentArrayView = Context.GetMutableFragmentView<FLocationDataFragment>();
		TArrayView<FMassMoveTargetFragment> MoveTargetFragmentArrayView = Context.GetMutableFragmentView<FMassMoveTargetFragment>();
		UMassSignalSubsystem& SignalSubsystem = Context.GetMutableSubsystemChecked<UMassSignalSubsystem>();
		TArrayView<FAnimationFragment> AnimationFragmentArrayView = Context.GetMutableFragmentView<FAnimationFragment>();

		
		TArray<FMassEntityHandle> EntitiesToSignal = TArray<FMassEntityHandle>();
		
		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
		{
			FLocationDataFragment& LocationDataFragment = LocationDataFragmentArrayView[EntityIndex];
			FTransform& Transform = TransformFragmentArrayView[EntityIndex].GetMutableTransform();
			FVector Location = Transform.GetLocation();

			//UE_LOG(LogTemp, Warning, TEXT("Location: %s"), *Location.ToString());
			
			if (LocationDataFragment.HitResultAsync.IsValid() && !LocationDataFragment.bTraceResultIn && LocationDataFragment.bShouldRecheckNav)
			{
				LocationDataFragment.bTraceResultIn = true;
				FTraceDatum Datum;
				GetWorld()->QueryTraceData(LocationDataFragment.HitResultAsync,Datum);
				FMassMoveTargetFragment& MoveTargetFragment = MoveTargetFragmentArrayView[EntityIndex];
				if (Datum.OutHits.Num() >= 1)
				{
					if (FMath::Abs(Location.Z - Datum.OutHits[0].ImpactPoint.Z) >= 20.f/LocationDataFragment.TraceDistMulti )
					{
						LocationDataFragment.bNewLocation = true;
						EntitiesToSignal.Add(Context.GetEntity(EntityIndex));
						LocationDataFragment.TimeSince += (LocationDataFragment.BaseRefresh/10)*LocationDataFragment.TraceDistMulti;
						LocationDataFragment.TraceDistMulti = FMath::Clamp(LocationDataFragment.TraceDistMulti*2.5f,0.22f,4.0f);
					}
					else // Trace was close enough didn't have to move
					{ 
						LocationDataFragment.TraceDistMulti = FMath::Clamp(LocationDataFragment.TraceDistMulti/2.5f,0.22f,4.0f);
					}
					//MoveTargetFragment.Center = Datum.OutHits.Last().ImpactPoint;
				} else // Didn't find anything in trace
				{
					LocationDataFragment.TraceDistMulti = FMath::Clamp(LocationDataFragment.TraceDistMulti*3.0f,0.22f,4.0f);
					LocationDataFragment.TimeSince += (LocationDataFragment.BaseRefresh/10)*LocationDataFragment.TraceDistMulti;
				}
			}
			
			LocationDataFragment.TimeSince += DeltaTime;
			if (LocationDataFragment.TimeSince < LocationDataFragment.BaseRefresh) continue;

			if (LocationDataFragment.bShouldRecheckNav)
			{
				LocationDataFragment.HitResultAsync = GetWorld()->AsyncLineTraceByChannel(
					EAsyncTraceType::Single,
					Location+FVector::UpVector*455.f*LocationDataFragment.TraceDistMulti,
					Location+FVector::UpVector*-650.f*LocationDataFragment.TraceDistMulti,
					ECollisionChannel::ECC_WorldStatic);
				LocationDataFragment.bTraceResultIn = false;
			}

			// If Entities didn't move on average last checks, set animation to walk.
			LocationDataFragment.ExponentialMove /= 1.2f;
			LocationDataFragment.ExponentialMove += (LocationDataFragment.OldLocation-Transform.GetLocation()).Size();
			if (LocationDataFragment.ExponentialMove <= 40.f)
			{
				FAnimationFragment& AnimFrag = AnimationFragmentArrayView[EntityIndex];
				LocationDataFragment.DidNotMoveStreak += 1;
				if (AnimFrag.CurrentState == EAnimationState::Idle)
				{
					// If the entity is standing Idle we want to remove the streak to enable initial movement easier.
					LocationDataFragment.DidNotMoveStreak = 0;
				}
				if (LocationDataFragment.DidNotMoveStreak >= 2)
				{
					if (LocationDataFragment.PrevWalkToLocationBeforeAttack != FVector::ZeroVector &&
						(LocationDataFragment.PrevWalkToLocationBeforeAttack-Transform.GetLocation()).Size() > 200.f)
					{
						LocationDataFragment.bIsAttackLocation = false;
						LocationDataFragment.WalkToLocation = LocationDataFragment.PrevWalkToLocationBeforeAttack;
					}
					AnimFrag.CurrentState = EAnimationState::Idle;
					LocationDataFragment.WalkToLocation = Transform.GetLocation();
				}
			} else
			{
				LocationDataFragment.DidNotMoveStreak = 0;
			}
			FMassEntityHandle EntityHandle = Context.GetEntity(EntityIndex);
			FVector NewOldLocation = SmbSubsystem.RegisterToGrid(Location,
				EntityHandle,
				LocationDataFragment.OldLocation);
			LocationDataFragment.OldLocation = NewOldLocation;
			
			LocationDataFragment.TimeSince = 0.f+FMath::RandRange(0.f,LocationDataFragment.BaseRefresh*0.3f);
		}
		if (EntitiesToSignal.Num() > 0)
		{
			//UE_LOG(LogMass, Display, TEXT("Told %i Entities to recheck move"),EntitiesToSignal.Num());
			SignalSubsystem.SignalEntitiesDeferred(Context,Smb::Signals::MoveTargetChanged,EntitiesToSignal);
		}
	});
}



UCollisionProcessor::UCollisionProcessor()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void UCollisionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FCollisionDataFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAgentRadiusFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<USmbSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UCollisionProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	float DeltaTime = FMath::Min(Context.GetDeltaTimeSeconds(),0.2f);

	EntityQuery.ParallelForEachEntityChunk(Context, [this, DeltaTime](FMassExecutionContext& Context)
	{
		USmbSubsystem& SmbSubsystem = Context.GetMutableSubsystemChecked<USmbSubsystem>();
		TArrayView<FTransformFragment> TransformFragmentArrayView = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FCollisionDataFragment> CollisionDataFragmentArrayView = Context.GetMutableFragmentView<FCollisionDataFragment>();
		TArrayView<FAgentRadiusFragment> AgentRadiusFragmentArrayView = Context.GetMutableFragmentView<FAgentRadiusFragment>();
		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
		{
			FCollisionDataFragment& CollisionDataFragment = CollisionDataFragmentArrayView[EntityIndex];
			FTransform& MutableTransform = TransformFragmentArrayView[EntityIndex].GetMutableTransform();
			FVector Location = MutableTransform.GetLocation();
			FAgentRadiusFragment& AgentRadiusFragment = AgentRadiusFragmentArrayView[EntityIndex];
			
			CollisionDataFragment.TimeSinceLastCheck += DeltaTime*FMath::RandRange(0.8f,1.2f);
			if (!(CollisionDataFragment.TimeSinceLastCheck <= CollisionDataFragment.CheckDelay))
			{
				TArray<FMassEntityHandle> Handles = SmbSubsystem.GetNumberClosestEntities(Location,
					AgentRadiusFragment.Radius*2.1f,
					CollisionDataFragment.MaxEntitiesToCheck);
				TStaticArray<FMassEntityHandle, COLLISION_ARR_SIZE> StaticHandlesArray;
				for (int i = 0; i < FMath::Min(COLLISION_ARR_SIZE,Handles.Num()); ++i)
				{
					if (Handles[i].IsValid())
						StaticHandlesArray[i] = Handles[i];
				}
				//UE_LOG(LogTemp, Warning, TEXT("Closest Entities: %d"), Handles.Num());
				CollisionDataFragment.ClosestEntities = StaticHandlesArray;
				CollisionDataFragment.TimeSinceLastCheck = 0;
			}
			FMassEntityManager& EntityManager = Context.GetEntityManagerChecked();
			for (FMassEntityHandle Handle : CollisionDataFragment.ClosestEntities)
			{
				if (!EntityManager.IsEntityValid(Handle)) continue;
				FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Handle);
				if (!TransformFrag) continue;
				FAgentRadiusFragment* OtherRadius = EntityManager.GetFragmentDataPtr<FAgentRadiusFragment>(Handle);
				if (!OtherRadius) continue;
				FVector OtherLocation = TransformFrag->GetTransform().GetLocation();
				float Distance = (Location - OtherLocation).Size();
				if (Distance <= AgentRadiusFragment.Radius+OtherRadius->Radius)
				{
					FCollisionDataFragment* OtherCollisionData = EntityManager.GetFragmentDataPtr<FCollisionDataFragment>(Handle);
					if (!OtherCollisionData) continue;
					float WeightMulti = 0.5f + OtherCollisionData->CollisionMass / CollisionDataFragment.CollisionMass;
					FVector SelfPushed = (Location - OtherLocation).GetSafeNormal() * WeightMulti * DeltaTime*80.f;
					SelfPushed.Z = 0.f;
					//UE_LOG(LogTemp, Warning, TEXT("Pushing: %s"), *SelfPushed.ToString());
					MutableTransform.SetLocation(MutableTransform.GetLocation() + SelfPushed);
				}
			}
		}
	});
}

ULocateEnemy::ULocateEnemy()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void ULocateEnemy::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FNearEnemiesFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTeamFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<USmbSubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void ULocateEnemy::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	float DeltaTime = FMath::Min(Context.GetDeltaTimeSeconds(),0.1f);
	
	EntityQuery.ParallelForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		USmbSubsystem& SmbSubsystem = Context.GetMutableSubsystemChecked<USmbSubsystem>();
		UMassSignalSubsystem& SignalSubsystem = Context.GetMutableSubsystemChecked<UMassSignalSubsystem>();
		TArrayView<FTransformFragment> TransformView = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FNearEnemiesFragment> NearEnemiesFragmentView = Context.GetMutableFragmentView<FNearEnemiesFragment>();
		TArrayView<FTeamFragment> TeamFragmentView = Context.GetMutableFragmentView<FTeamFragment>();

		TArray<FMassEntityHandle> EntitiesToSignal = TArray<FMassEntityHandle>();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
		{
			FNearEnemiesFragment& NearEnemiesFragment = NearEnemiesFragmentView[EntityIndex];
			NearEnemiesFragment.TimeSinceLastCheck += DeltaTime;
			if (NearEnemiesFragment.TimeSinceLastCheck < NearEnemiesFragment.CheckPeriod) continue;
			auto Prev = NearEnemiesFragment.ClosestEnemies;
			NearEnemiesFragment.TimeSinceLastCheck = 0.f+FMath::RandRange(0.f,NearEnemiesFragment.CheckPeriod/3);
			FTransformFragment TransformFragment = TransformView[EntityIndex];
			FTeamFragment TeamFragment = TeamFragmentView[EntityIndex];
			NearEnemiesFragment.ClosestEnemies = SmbSubsystem.GetNumberClosestEntities(
				TransformFragment.GetTransform().GetLocation(),
				NearEnemiesFragment.CheckRadius,
				NearEnemiesFragment.AmountOfEnemies,
				TeamFragment.TeamID);
			//Call found enemy
			if (NearEnemiesFragment.ClosestEnemies.Num() > 0)
			{
				if (Prev.Num() <= 0)
				{
					//SignalSubsystem.SignalEntityDeferred(Context,Smb::Signals::FoundEnemy,Context.GetEntity(EntityIndex));
					EntitiesToSignal.Add(Context.GetEntity(EntityIndex));
				}
				//TSet<FMassEntityHandle> NewSet = TSet<FMassEntityHandle>();
				//else if (!NearEnemiesFragment.ClosestEnemies.Contains(Prev[0]))
				//{
				//	//SignalSubsystem.SignalEntityDeferred(Context,Smb::Signals::FoundEnemy,Context.GetEntity(EntityIndex));
				//	EntitiesToSignal.Add(Context.GetEntity(EntityIndex));
				//}
			}
		}
		if (EntitiesToSignal.Num() > 0)
		{
			SignalSubsystem.SignalEntitiesDeferred(Context,Smb::Signals::FoundEnemy,EntitiesToSignal);
		}
	});
}


UProjectileProcessor::UProjectileProcessor()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void UProjectileProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FProjectileFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FProjectileParams>();
	EntityQuery.AddSubsystemRequirement<USmbSubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FProjectileTag>(EMassFragmentPresence::All);
}

void UProjectileProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	TArray<FVector> Positions = TArray<FVector>(); 
	EntityQuery.ForEachEntityChunk(Context, [&Positions](FMassExecutionContext& Context)
	{
		const float DeltaTime = FMath::Min(0.1f, Context.GetDeltaTimeSeconds());

		USmbSubsystem& SmbSubsystem = Context.GetMutableSubsystemChecked<USmbSubsystem>();
		
		const TArrayView<FTransformFragment> TransformFragArr = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FProjectileFragment> ProjectileFragArr = Context.GetMutableFragmentView<FProjectileFragment>();
		const FProjectileParams ProjectileParam = Context.GetConstSharedFragment<FProjectileParams>();
		
		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FTransform& MutableTransform = TransformFragArr[EntityIt].GetMutableTransform();
			FProjectileFragment& ProjectileFrag = ProjectileFragArr[EntityIt];
			ProjectileFrag.Velocity = ProjectileFrag.Velocity*(1 - 0.0001f*DeltaTime) + FVector(0.f,0.f,-980.f)*DeltaTime;
			MutableTransform.SetLocation(MutableTransform.GetLocation() + DeltaTime*ProjectileFrag.Velocity);
			if ((MutableTransform.GetLocation()-ProjectileFrag.Target).Size() <= 30.f)
			{
				int32 KillCount = 0;
				SmbSubsystem.DealDamageAoe(MutableTransform.GetLocation(),ProjectileParam.AreaOfEffectRadius
					,ProjectileParam.Damage,
					ProjectileParam.DamageType,
					ProjectileParam.TeamId,
					KillCount);
			}
			Positions.Add(MutableTransform.GetLocation());
		}
	});
	GetWorld()->GetSubsystem<USmbSubsystem>()->SetProjectileLocations(Positions);
	Positions.Empty();
}

UAddDeathFragmentProcessor::UAddDeathFragmentProcessor()
	:EntityQuery(*this)
{
	ObservedType = FAliveTag::StaticStruct();
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION<7 
	Operation = EMassObservedOperation::Add;
#else
	ObservedOperations = EMassObservedOperationFlags::Add;
#endif
	//bAutoRegisterWithProcessingPhases = true;
	//ExecutionFlags = (int32)(EProcessorExecutionFlags::AllNetModes);
	//ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void UAddDeathFragmentProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddSharedRequirement<FDeathPhysicsSharedFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FAliveTag>(EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FLocationDataFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<USmbSubsystem>(EMassFragmentAccess::ReadOnly);
}

void UAddDeathFragmentProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const FDeathPhysicsSharedFragment* DeathPhysFragment = Context.GetMutableSharedFragmentPtr<FDeathPhysicsSharedFragment>();
		TConstArrayView<FTransformFragment> TransformFragmentView = Context.GetFragmentView<FTransformFragment>();
		TArrayView<FLocationDataFragment> LocationDataFragmentView = Context.GetMutableFragmentView<FLocationDataFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FTransform Transform = TransformFragmentView[EntityIt].GetTransform();
			FLocationDataFragment& LocationDataFrag = LocationDataFragmentView[EntityIt];
			LocationDataFrag.WalkToLocation = Transform.GetLocation();
		}
		
		USmbSubsystem* SmbSubsystem = Context.GetWorld()->GetSubsystem<USmbSubsystem>();
		if (DeathPhysFragment->StaticMesh)
		{
			DeathPhysFragment->StaticMesh.LoadSynchronous();
			
			SmbSubsystem->AddPhysicsManagerToWorld(DeathPhysFragment->StaticMesh);
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("Death Physics Fragment missing mesh"));
		}
	});
}



UHeightProcessor::UHeightProcessor()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void UHeightProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FHeightFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAnimationFragment>(EMassFragmentAccess::ReadWrite);
}

void UHeightProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	float DeltaTime = FMath::Min(Context.GetDeltaTimeSeconds(),0.21f);
	
	
	EntityQuery.ParallelForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		UNavigationSystemV1* NavMeshSubsystem = Cast<UNavigationSystemV1>(Context.GetWorld()->GetNavigationSystem());
		TArrayView<FTransformFragment> TransformFragmentView = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FHeightFragment> HeightFragmentView = Context.GetMutableFragmentView<FHeightFragment>();
		TArrayView <FMassDesiredMovementFragment> DesiredMovementFragmentView = Context.GetMutableFragmentView<FMassDesiredMovementFragment>();
		TArrayView<FAnimationFragment> AnimationFragmentView = Context.GetMutableFragmentView<FAnimationFragment>();
		
		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			if (!NavMeshSubsystem) continue;
			FTransformFragment& TransformFragment = TransformFragmentView[EntityIt];
			FTransform& Transform = TransformFragment.GetMutableTransform();
			FHeightFragment& HeightFragment = HeightFragmentView[EntityIt];
			FAnimationFragment& AnimationFragment = AnimationFragmentView[EntityIt];
			FMassDesiredMovementFragment& DesiredMovementFragment = DesiredMovementFragmentView[EntityIt];
			
			if (AnimationFragment.CurrentState == EAnimationState::Attacking)
			{
				DesiredMovementFragment.DesiredVelocity = DesiredMovementFragment.DesiredVelocity*0.95f;
			}
			HeightFragment.TimeSinceRefresh += DeltaTime;
			if (HeightFragment.CurrentHeight <= -999999999.f)
			{
				HeightFragment.CurrentHeight = Transform.GetLocation().Z;
			}
			if (HeightFragment.TimeSinceRefresh > HeightFragment.BaseRefreshPeriod)
			{
				FNavLocation NavLocation;
				bool bFoundLocation = NavMeshSubsystem->ProjectPointToNavigation(Transform.GetLocation()+DesiredMovementFragment.DesiredVelocity*HeightFragment.BaseRefreshPeriod,NavLocation, FVector(100.f,100.f,700.f));
				//UE_LOG(LogTemp, Display, TEXT("Links total: %i "),NavMeshSubsystem->GetNav);
				if (bFoundLocation)
				{
					HeightFragment.TargetHeight = NavLocation.Location.Z;
					//HeightFragment.CurrentHeight = Transform.GetLocation().Z;
					//TransformFragmentView[EntityIt].GetMutableTransform().SetLocation(FVector(NavLocation.Location.X,NavLocation.Location.Y,NavLocation.Location.Z));
				} else
				{
					bFoundLocation = NavMeshSubsystem->ProjectPointToNavigation(Transform.GetLocation()+DesiredMovementFragment.DesiredVelocity*HeightFragment.BaseRefreshPeriod,NavLocation, FVector(200.f,200.f,700.f)*HeightFragment.DistanceAwayToCheckNavMulti);
					if (bFoundLocation)
					{
						Transform.SetLocation(NavLocation.Location);
						HeightFragment.TargetHeight = NavLocation.Location.Z;
					}
				}
				HeightFragment.TimeSinceRefresh = 0.f;
			}

			float DistanceZRemaining = HeightFragment.TargetHeight-HeightFragment.CurrentHeight;

			float ZVelocity = FMath::Clamp(DistanceZRemaining,-1.f,1.f)*HeightFragment.HeightInterpolationSpeed*DeltaTime;
			if (FMath::Abs(HeightFragment.HeightInterpolationSpeed/100.f) >= FMath::Abs(DistanceZRemaining))
			{
				Transform.SetLocation(FVector(Transform.GetLocation().X,Transform.GetLocation().Y,HeightFragment.CurrentHeight));
			} else
			{
				Transform.SetLocation(Transform.GetLocation()+FVector::UpVector*(ZVelocity+HeightFragment.CurrentHeight-Transform.GetLocation().Z));
			}
			
			HeightFragment.CurrentHeight = Transform.GetLocation().Z;
		}
	});
}

UAbilityProcessor::UAbilityProcessor()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void UAbilityProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTeamFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<USmbSubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FNearEnemiesFragment>(EMassFragmentAccess::ReadWrite);
	//EntityQuery.AddRequirement<FCollisionDataFragment>(EMassFragmentAccess::ReadWrite);
	//EntityQuery.AddRequirement<FLocationDataFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAnimationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAbilityDataFragment>(EMassFragmentAccess::ReadWrite);
}

void UAbilityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	float DeltaTime = FMath::Min(Context.GetDeltaTimeSeconds(),0.1f);
	
	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TArrayView<FTransformFragment> TransformFragmentView = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FTeamFragment> TeamFragmentView = Context.GetMutableFragmentView<FTeamFragment>();
		TArrayView<FNearEnemiesFragment> NearEnemiesFragmentView = Context.GetMutableFragmentView<FNearEnemiesFragment>();
		//TArrayView<FCollisionDataFragment> CollisionDataFragmentView = Context.GetMutableFragmentView<FCollisionDataFragment>();
		//TArrayView<FLocationDataFragment> LocationDataFragmentView = Context.GetMutableFragmentView<FLocationDataFragment>();
		TArrayView<FAnimationFragment> AnimationFragmentView = Context.GetMutableFragmentView<FAnimationFragment>();
		USmbSubsystem& SmbSubsystem = Context.GetMutableSubsystemChecked<USmbSubsystem>();
		UMassSignalSubsystem& SignalSubsystem = Context.GetMutableSubsystemChecked<UMassSignalSubsystem>();
		TArrayView<FAbilityDataFragment> AbilityDataFragmentView = Context.GetMutableFragmentView<FAbilityDataFragment>();

		TArray<FMassEntityHandle> EntitiesToSignal = TArray<FMassEntityHandle>();
		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FAbilityDataFragment& AbilityDataFragment = AbilityDataFragmentView[EntityIt];
			if (!AbilityDataFragment.IsAttacking)
			{
				AbilityDataFragment.TimeInAttack = 0.f;
				continue;
			}
			FMassEntityManager& EntityManager = Context.GetEntityManagerChecked();
			if (!EntityManager.IsEntityValid(Context.GetEntity(EntityIt))) continue;
			
			FTransformFragment& TransformFragment = TransformFragmentView[EntityIt];
			FTeamFragment& TeamFragment = TeamFragmentView[EntityIt];
			FNearEnemiesFragment& EnemiesNear = NearEnemiesFragmentView[EntityIt];
			//FCollisionDataFragment& CollisionDataFragment = CollisionDataFragmentView[EntityIt];
			//FLocationDataFragment& LocationDataFragment = LocationDataFragmentView[EntityIt];
			FAnimationFragment& AnimationFragment = AnimationFragmentView[EntityIt];
			
			FMassEntityHandle EnemyHandle = AbilityDataFragment.TargetEntity;
			float EnemyRadius = 0.f;
			if (SmbSubsystem.IsEntityValidManager(EnemyHandle))
			{
				AbilityDataFragment.TargetLocation = SmbSubsystem.GetEntityLocation(EnemyHandle);
			} else
			{
				if (EnemiesNear.ClosestEnemies.Num() <= 0)
				{
					AbilityDataFragment.IsAttacking = false;
					AbilityDataFragment.TimeInAttack = 0.f;
					AbilityDataFragment.TimesHit = 0;
					EntitiesToSignal.Add(Context.GetEntity(EntityIt));
					continue;
				}
				AbilityDataFragment.TargetEntity = EnemiesNear.ClosestEnemies[0];
				if (SmbSubsystem.IsEntityValidManager(EnemiesNear.ClosestEnemies[0]))
				{
					FAgentRadiusFragment* AgentRadiusFragment = EntityManager.GetFragmentDataPtr<FAgentRadiusFragment>(EnemiesNear.ClosestEnemies[0]);
					FVector OwnLocation = TransformFragment.GetTransform().GetLocation();
					EnemyRadius = AgentRadiusFragment->Radius;
					FVector EnemyLoc = SmbSubsystem.GetEntityLocation(EnemiesNear.ClosestEnemies[0]);
					AbilityDataFragment.TargetLocation = OwnLocation+(EnemyLoc-OwnLocation).GetSafeNormal()*EnemyRadius*2;
					//AbilityDataFragment.TargetLocation = SmbSubsystem.GetEntityLocation(EnemiesNear.ClosestEnemies[0]);
				} else
				{
					AbilityDataFragment.IsAttacking = false;
					AbilityDataFragment.TimeInAttack = 0.f;
					AbilityDataFragment.TimesHit = 0;
					EntitiesToSignal.Add(Context.GetEntity(EntityIt));
					continue;
				}
			}
			FVector Location = AbilityDataFragment.TargetLocation;
			FTransform Transform = TransformFragment.GetMutableTransform();
			FTransform EnemyTransform = FTransform();
			EnemyTransform.SetLocation(Location);

			// Too far away
			if ((Location-Transform.GetLocation()).Size() >= AbilityDataFragment.CurrentAbility->Range*1.7+EnemyRadius*2)
			{
				AbilityDataFragment.IsAttacking = false;
				AbilityDataFragment.TimeInAttack = 0.f;
				AbilityDataFragment.TimesHit = 0;
				EntitiesToSignal.Add(Context.GetEntity(EntityIt));
				//UE_LOG(LogTemp, Warning, TEXT("Too far away %f"),AbilityDataFragment.CurrentAbility->Range+EnemyRadius);
				continue;
			}

			AbilityDataFragment.TimeInAttack += DeltaTime;

			// Wait if not hitting yet
			if (AbilityDataFragment.TimeInAttack <= AbilityDataFragment.CurrentAbility->TimeUntilHit) continue;

			USmbAbilityData* Ability = AbilityDataFragment.CurrentAbility;

			if (AbilityDataFragment.TimesHit < 1)
			{
				AbilityDataFragment.TimesHit += 1;
				
				FVector EnemyLocation = AbilityDataFragment.TargetLocation;
				EnemyHandle = FMassEntityHandle(AbilityDataFragment.TargetEntity.Index, AbilityDataFragment.TargetEntity.SerialNumber);
				if (SmbSubsystem.IsEntityValidManager(EnemyHandle))
				{
					EnemyLocation = SmbSubsystem.GetEntityLocation(EnemyHandle);
				} else
				{
					EnemyLocation = Transform.GetLocation();
				}

				FGameplayTagContainer AbilityContainer = FGameplayTagContainer(
					FGameplayTag::RequestGameplayTag(FName("Skill.Type")));
				//Find skill type
				AbilityContainer = Ability->AbilityTagContainer.Filter(AbilityContainer);
				if (AbilityContainer.Num() == 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("No skill found"));
					AbilityDataFragment.IsAttacking = false;
					AbilityDataFragment.TimeInAttack = 0.f;
					AbilityDataFragment.TimesHit = 0;
					EntitiesToSignal.Add(Context.GetEntity(EntityIt));
					continue;
				}
				FGameplayTagContainer AllAbilityContainer = FGameplayTagContainer(
					FGameplayTag::RequestGameplayTag(FName("Skill")));
				AllAbilityContainer = Ability->AbilityTagContainer.Filter(AllAbilityContainer);
				
				//Switch on skill type
				//If melee skill
				
				if (AllAbilityContainer.HasAny(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Skill.Type.Melee")))))
				{
					int32 Killed = 0;
					if (AllAbilityContainer.HasAny(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Skill.Modifier.Area")))))
					{
						SmbSubsystem.DealDamageAoe(EnemyLocation,
						Ability->AreaOfEffect,
						Ability->Damage,
						EDamageType::Normal,
						TeamFragment.TeamID,
						Killed);
					} else
					{
						SmbSubsystem.DealDamageToEnemy(
							AbilityDataFragment.TargetEntity,
							Ability->Damage,
							EDamageType::Normal);
					}
				}
				else if (AbilityContainer.HasAny(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Skill.Type.Ranged")))))
				{
					SmbSubsystem.AddProjectile(Transform.GetLocation()+FVector::UpVector*180.f,
						EnemyLocation,
						Ability,
						TeamFragment.TeamID);
				}
				else if (AbilityContainer.HasAny(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Skill.Type.Cast")))))
				{
					//TODO fire cast projectile and remove mana
					UE_LOG(LogTemp, Warning, TEXT("Cast not implemented"));
				}
				else if (AbilityContainer.HasAny(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Skill.Type.Gather")))))
				{
					//TODO gather nearby
					UE_LOG(LogTemp, Warning, TEXT("Gather not implemented"));
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("No skill found"));
					AbilityDataFragment.IsAttacking = false;
					AbilityDataFragment.TimeInAttack = 0.f;
					AbilityDataFragment.TimesHit = 0;
					EntitiesToSignal.Add(Context.GetEntity(EntityIt));
					continue;
				}
				//Spawn VFX if should hit
				
				SmbSubsystem.SpawnAbilityDataDeferred(Ability, EnemyTransform, 0.f);
			}
			
			if (AbilityDataFragment.TimeInAttack >= Ability->TimeUntilHit+Ability->RecoveryTime)
			{
				AnimationFragment.CurrentState = EAnimationState::Idle;
				AbilityDataFragment.IsAttacking = false;
				AbilityDataFragment.TimeInAttack = 0.f;
				AbilityDataFragment.TimesHit = 0;
				EntitiesToSignal.Add(Context.GetEntity(EntityIt));
			}
		}
		if (EntitiesToSignal.Num() >= 1)
		{
			SignalSubsystem.SignalEntitiesDeferred(Context, Smb::Signals::AttackFinished, EntitiesToSignal);	
		}
	});
}

UClientMoveProcessor::UClientMoveProcessor()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void UClientMoveProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FLocationDataFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FMassMovementParameters>();
}

void UClientMoveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	float DeltaTime = FMath::Min(Context.GetDeltaTimeSeconds(),0.1f);
	
	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		TArrayView<FTransformFragment> TransformFragmentView = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FLocationDataFragment> LocationDataFragmentView = Context.GetMutableFragmentView<FLocationDataFragment>();
		TArrayView<FMassDesiredMovementFragment> DesiredMovementFragmentsView = Context.GetMutableFragmentView<FMassDesiredMovementFragment>();
		FMassMovementParameters MovementParameters = Context.GetConstSharedFragment<FMassMovementParameters>();
		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FTransformFragment& TransformFragment = TransformFragmentView[EntityIt];
			FLocationDataFragment& LocationDataFragment = LocationDataFragmentView[EntityIt];
			FMassDesiredMovementFragment& DesiredMovementFragment = DesiredMovementFragmentsView[EntityIt];

			if (LocationDataFragment.NextLocation != FVector::ZeroVector)
			{
				//UE_LOG(LogTemp, Display, TEXT("New Location"));
				FTransform MutableTransform = TransformFragment.GetMutableTransform();
				FVector DesiredDirection = (LocationDataFragment.NextLocation-MutableTransform.GetLocation()).GetSafeNormal();
				DesiredMovementFragment.DesiredVelocity = MovementParameters.DefaultDesiredSpeed*DesiredDirection;
			}
		}
	});
}



/*
UScaleProcessors::UScaleProcessors()
	:EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void UScaleProcessors::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	//FMassEntityQuery EntityQuery(EntityManager);

	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	
	
}

void UScaleProcessors::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	float DeltaTime = FMath::Min(Context.GetDeltaTimeSeconds(),0.2f);
	
	ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context){
	
	});
}

*/