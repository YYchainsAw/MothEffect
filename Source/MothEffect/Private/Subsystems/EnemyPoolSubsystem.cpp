#include "Subsystems/EnemyPoolSubsystem.h"
#include "AI/EnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/HealthComponent.h"
#include "Engine/World.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"

bool UEnemyPoolSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

bool UEnemyPoolSubsystem::IsBirthplaceClear(const AEnemyBase* Enemy) const
{
	if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed() || Enemy->GetWorld() != GetWorld()) return false;
	const UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
	if (!Capsule) return false;
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	if (!FMath::IsFinite(Radius) || !FMath::IsFinite(HalfHeight) || Radius <= 0.0f || HalfHeight < Radius
		|| Capsule->GetComponentLocation().ContainsNaN() || Capsule->GetComponentQuat().ContainsNaN()) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothEnemyPoolBirth), false, Enemy);
	return !GetWorld()->OverlapBlockingTestByChannel(Capsule->GetComponentLocation(), Capsule->GetComponentQuat(),
		Capsule->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius, HalfHeight), Params,
		FCollisionResponseParams(Capsule->GetCollisionResponseToChannels()));
}

AEnemyBase* UEnemyPoolSubsystem::AcquireEnemy(TSubclassOf<AEnemyBase> EnemyClass, const FTransform& Transform, AActor* Owner)
{
	if (bShuttingDown || !GetWorld() || !EnemyClass || EnemyClass->HasAnyClassFlags(CLASS_Abstract)
		|| !GetWorld()->HasBegunPlay() || GetWorld()->IsPaused() || Transform.ContainsNaN()
		|| (Owner && (!IsValid(Owner) || Owner->IsActorBeingDestroyed() || Owner->GetWorld() != GetWorld()))) return nullptr;
	auto* Session = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!Session || !Session->IsGameplayEnabled()) return nullptr;
	const uint64 LifecycleGeneration = Session->GetLifecycleGeneration();
	AEnemyBase* Enemy = nullptr;
	ManagedEnemies.RemoveAll([](const TObjectPtr<AEnemyBase>& Entry) { return !IsValid(Entry) || Entry->IsActorBeingDestroyed(); });
	for (int32 Index = InactiveEnemies.Num() - 1; Index >= 0; --Index)
	{
		AEnemyBase* Candidate = InactiveEnemies[Index].Get();
		if (!IsValid(Candidate) || Candidate->IsActorBeingDestroyed())
		{
			InactiveEnemies.RemoveAtSwap(Index);
			continue;
		}
		if (Candidate->GetClass() != EnemyClass.Get()) continue;
		// Multicast listeners still on the old hit/death stack must not observe a new life.
		if (Candidate->PoolCallbackDepth > 0 || Candidate->HealthComponent->HitDispatchDepth > 0) continue;
		Enemy = Candidate;
		InactiveEnemies.RemoveAtSwap(Index);
		++ReuseCount;
		break;
	}
	if (!Enemy)
	{
		Enemy = GetWorld()->SpawnActorDeferred<AEnemyBase>(EnemyClass, Transform, Owner, nullptr,
			ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
		if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed()) return nullptr;
		// Actor-spawn observers run inside SpawnActorDeferred and may close/reset the run.
		if (bShuttingDown || !Session->IsGenerationCurrent(LifecycleGeneration) || GetWorld()->IsPaused())
		{
			Enemy->Destroy();
			return nullptr;
		}
		Enemy->bPoolManaged = true;
		ManagedEnemies.Add(Enemy);
		++CreatedCount;
	}
	PendingCheckout.Add(Enemy);
	Enemy->PoolCheckoutLifecycleGeneration = LifecycleGeneration;
	Enemy->PrepareForPoolAcquisition();
	if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed() || !PendingCheckout.Contains(Enemy)) return nullptr;
	if (!Session->IsGenerationCurrent(LifecycleGeneration) || GetWorld()->IsPaused())
	{
		RecycleEnemy(Enemy);
		return nullptr;
	}
	// A rejected checkout is a pending birth, not a death from the previous life.
	// Keep damage admission closed while exposing this new life's health to the director.
	if (Enemy->HasActorBegunPlay()) Enemy->HealthComponent->ResetForPoolReuse(Enemy->MaxHealth);
	Enemy->SetOwner(Owner);
	Enemy->SetInstigator(nullptr);
	Enemy->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	return Enemy;
}

void UEnemyPoolSubsystem::FinishSpawningEnemy(AEnemyBase* Enemy, const FTransform& Transform)
{
	if (bShuttingDown || !IsValid(Enemy) || Enemy->IsActorBeingDestroyed() || Enemy->GetWorld() != GetWorld()
		|| !PendingCheckout.Remove(Enemy)) return;
	auto* Session = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	const auto IsCheckoutCurrent = [&]()
	{
		return !bShuttingDown && GetWorld()->HasBegunPlay() && !GetWorld()->IsPaused()
			&& Session && Session->IsGenerationCurrent(Enemy->PoolCheckoutLifecycleGeneration);
	};
	if (Transform.ContainsNaN() || !IsCheckoutCurrent()) { RecycleEnemy(Enemy); return; }
	const uint64 LifeGeneration = Enemy->PoolLifeGeneration;
	if (!Enemy->HasActorBegunPlay()) Enemy->FinishSpawning(Transform);
	if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed() || LifeGeneration != Enemy->PoolLifeGeneration) return;
	if (!IsCheckoutCurrent() || !IsBirthplaceClear(Enemy)) { RecycleEnemy(Enemy); return; }
	Enemy->ActivateFromPool();
	if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed() && LifeGeneration == Enemy->PoolLifeGeneration
		&& !IsCheckoutCurrent()) RecycleEnemy(Enemy);
}

void UEnemyPoolSubsystem::RecycleEnemy(AEnemyBase* Enemy)
{
	if (bShuttingDown || !IsValid(Enemy) || Enemy->IsActorBeingDestroyed() || Enemy->GetWorld() != GetWorld()) return;
	if (!Enemy->bPoolManaged || !ManagedEnemies.Contains(Enemy)) { Enemy->Destroy(); return; }
	if (InactiveEnemies.Contains(Enemy) || RecyclingEnemies.Contains(Enemy)) return;
	PendingCheckout.Remove(Enemy);
	// Deferred actors have not run their construction scripts/BeginPlay. They are not reusable yet.
	if (!Enemy->HasActorBegunPlay())
	{
		ManagedEnemies.Remove(Enemy);
		Enemy->Destroy();
		return;
	}
	RecyclingEnemies.Add(Enemy);
	// The actor is unavailable to nested checkouts while death/telegraph observers run.
	Enemy->DeactivateForPool();
	RecyclingEnemies.Remove(Enemy);
	if (!bShuttingDown && IsValid(Enemy) && !Enemy->IsActorBeingDestroyed() && Enemy->HasActorBegunPlay())
	{
		if (GetInactiveCount() < FMath::Max(0, MaxRetainedEnemies)) InactiveEnemies.AddUnique(Enemy);
		else { ManagedEnemies.Remove(Enemy); Enemy->Destroy(); }
	}
}

int32 UEnemyPoolSubsystem::GetInactiveCount(TSubclassOf<AEnemyBase> EnemyClass) const
{
	int32 Count = 0;
	for (const auto& Entry : InactiveEnemies)
		if (const AEnemyBase* Enemy = Entry.Get(); IsValid(Enemy) && !Enemy->IsActorBeingDestroyed()
			&& (!EnemyClass || Enemy->GetClass() == EnemyClass.Get())) ++Count;
	return Count;
}

void UEnemyPoolSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
	bShuttingDown = true;
	Super::OnWorldEndPlay(InWorld);
}

void UEnemyPoolSubsystem::Deinitialize()
{
	bShuttingDown = true;
	InactiveEnemies.Reset();
	PendingCheckout.Reset();
	RecyclingEnemies.Reset();
	ManagedEnemies.Reset();
	Super::Deinitialize();
}
