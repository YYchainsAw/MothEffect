#include "Subsystems/ProjectilePoolSubsystem.h"
#include "Combat/RuleProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"

bool UProjectilePoolSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

ARuleProjectile* UProjectilePoolSubsystem::SpawnProjectile(TSubclassOf<ARuleProjectile> Class,
	const FTransform& Transform, const FVector& Direction, AActor* Source, APawn* Instigator, bool bLogEvents)
{
	UWorld* World = GetWorld();
	if (bClosed || !World || !World->HasBegunPlay() || !Class || Class->HasAnyClassFlags(CLASS_Abstract)
		|| Transform.ContainsNaN() || Direction.ContainsNaN() || Direction.IsNearlyZero()
		|| (IsValid(Source) && Source->GetWorld() != World)
		|| (IsValid(Instigator) && Instigator->GetWorld() != World)) return nullptr;
	UDeviceLifecycleSubsystem* Lifecycle = World->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!Lifecycle) return nullptr;
	const uint64 Generation = Lifecycle->GetLifecycleGeneration();
	if (!Lifecycle->CanSpawnProjectile() || !Lifecycle->IsGenerationCurrent(Generation)) return nullptr;
	PruneInactive();

	ARuleProjectile* Projectile = nullptr;
	if (auto* Bucket = InactiveByClass.Find(Class.Get()))
	{
		while (Bucket->Num() && !Projectile)
		{
			ARuleProjectile* Candidate = Bucket->Pop(EAllowShrinking::No).Get();
			if (IsValid(Candidate) && !Candidate->IsActorBeingDestroyed() && Candidate->IsPoolInactive()
				&& Candidate->GetClass() == Class.Get() && Candidate->OwningPool.Get() == this) Projectile = Candidate;
		}
		if (!Bucket->Num()) InactiveByClass.Remove(Class.Get());
	}
	const bool bReused = Projectile != nullptr;
	if (!Projectile)
	{
		Projectile = World->SpawnActorDeferred<ARuleProjectile>(Class, Transform, Source, Instigator,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Projectile) return nullptr;
		++CreatedCount;
		if (bClosed || !IsValid(Projectile) || Projectile->IsActorBeingDestroyed()
			|| !Lifecycle->IsGenerationCurrent(Generation))
		{
			if (IsValid(Projectile) && !Projectile->IsActorBeingDestroyed()) Projectile->Destroy();
			return nullptr;
		}
		Projectile->OwningPool = this;
	}
	// A BeginPlay/feedback callback may end the session or try to recycle this shot.
	// Keep it out of the free list until the complete activation call has unwound.
	Projectile->bPoolActivationInProgress = true;
	Projectile->PreparePoolActivation(Transform, Source, Instigator, bLogEvents);
	bool bReady = Lifecycle->IsGenerationCurrent(Generation)
		&& Projectile->InitializeProjectile(Direction, Source, Instigator);
	if (bReady)
	{
		if (bReused) bReady = Projectile->StartFlight();
		else Projectile->FinishSpawning(Transform);
	}
	bReady = bReady && !bClosed && IsValid(Projectile) && !Projectile->IsActorBeingDestroyed()
		&& Projectile->HasActorBegunPlay() && Projectile->IsActivationCurrent()
		&& Lifecycle->IsGenerationCurrent(Generation);
	Projectile->bPoolActivationInProgress = false;
	if (!bReady)
	{
		RecycleProjectile(Projectile);
		return nullptr;
	}
	if (bReused) ++ReuseCount;
	return Projectile;
}

void UProjectilePoolSubsystem::RecycleProjectile(ARuleProjectile* Projectile)
{
	if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed() || Projectile->GetWorld() != GetWorld()) return;
	if (Projectile->OwningPool.Get() != this) { Projectile->Destroy(); return; }
	if (bClosed || Projectile->IsPoolInactive()) return;
	if (Projectile->bImpactDispatching || Projectile->bPoolActivationInProgress)
	{
		Projectile->bReturnPending = true;
		Projectile->StopFlight();
		return;
	}
	// A failed deferred birth has never run construction/BeginPlay and cannot be reused.
	if (!Projectile->HasActorBegunPlay()) { Projectile->Destroy(); return; }
	Projectile->DeactivateForPool();
	if (bClosed || !IsValid(Projectile) || Projectile->IsActorBeingDestroyed() || !Projectile->HasActorBegunPlay()) return;
	PruneInactive();
	if (GetInactiveCount() >= FMath::Max(0, MaxRetainedProjectiles))
	{
		Projectile->Destroy();
		return;
	}
	InactiveByClass.FindOrAdd(Projectile->GetClass()).Add(Projectile);
}

void UProjectilePoolSubsystem::PruneInactive()
{
	for (auto It = InactiveByClass.CreateIterator(); It; ++It)
	{
		It.Value().RemoveAllSwap([](const auto& Item)
		{
			return !Item.IsValid() || Item->IsActorBeingDestroyed() || !Item->IsPoolInactive();
		});
		if (!It.Key().IsValid() || !It.Value().Num()) It.RemoveCurrent();
	}
}

int32 UProjectilePoolSubsystem::GetInactiveCount() const
{
	int32 Count = 0;
	for (const auto& Bucket : InactiveByClass)
		for (const auto& Item : Bucket.Value)
			if (Item.IsValid() && !Item->IsActorBeingDestroyed() && Item->IsPoolInactive()) ++Count;
	return Count;
}

void UProjectilePoolSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
	bClosed = true;
	InactiveByClass.Reset();
	Super::OnWorldEndPlay(InWorld);
}

void UProjectilePoolSubsystem::Deinitialize()
{
	bClosed = true;
	InactiveByClass.Reset();
	Super::Deinitialize();
}
