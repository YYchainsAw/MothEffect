#include "Devices/DeviceBase.h"
#include "AI/EnemyBase.h"
#include "Combat/RuleProjectile.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "MothEffect.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Subsystems/ProjectilePoolSubsystem.h"
#include "TimerManager.h"
#include "Types/MothCollisionChannels.h"

FVector ADeviceBase::ResolveEmitterActivationDirection(const FVector& FallbackDirection) const
{
	if (!GetWorld() || !FMath::IsFinite(EmitterTargetRangeCm) || EmitterTargetRangeCm <= 0.0f)
	{
		return FallbackDirection;
	}
	const FVector Origin = PhysicsBody->GetComponentLocation();
	double NearestDistanceSquared = FMath::Square(static_cast<double>(EmitterTargetRangeCm));
	FVector Direction = FallbackDirection;
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		const AEnemyBase* Enemy = *It;
		if (!IsValid(Enemy) || !Enemy->IsCombatEnabled()) continue;
		const FVector ToEnemy = Enemy->GetActorLocation() - Origin;
		const double DistanceSquared = ToEnemy.SizeSquared();
		if (ToEnemy.ContainsNaN() || DistanceSquared <= UE_SMALL_NUMBER || DistanceSquared > NearestDistanceSquared) continue;
		NearestDistanceSquared = DistanceSquared;
		Direction = ToEnemy.GetSafeNormal();
	}
	return Direction;
}

void ADeviceBase::ScheduleEmitterShot()
{
	if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Active)
	{
		return;
	}
	const double DueAt = EmitterStartedAt + FMath::Max(0.0f, EmitterFirstShotDelaySeconds)
		+ EmitterNextShotIndex * static_cast<double>(FMath::Max(0.01f, EmitterShotIntervalSeconds));
	// A shot scheduled exactly at expiry is excluded, regardless of timer callback order.
	if (DueAt >= EmitterExpiresAt)
	{
		return;
	}
	const double Delay = DueAt - GetWorld()->GetTimeSeconds();
	if (Delay <= 0.0)
	{
		EmitterShotTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ADeviceBase::FireEmitterShot);
	}
	else
	{
		GetWorldTimerManager().SetTimer(EmitterShotTimer, this, &ADeviceBase::FireEmitterShot,
			static_cast<float>(Delay), false);
	}
}

void ADeviceBase::FireEmitterShot()
{
	if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Active || GetWorld()->IsPaused())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now >= EmitterExpiresAt)
	{
		FinishActivation();
		return;
	}
	++EmitterShotAttempts;
	++EmitterNextShotIndex;
	SpawnEmitterProjectile();
	if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Active)
	{
		return;
	}
	// Skip missed slots after a slow frame rather than firing a catch-up burst.
	const double Interval = FMath::Max(0.01f, EmitterShotIntervalSeconds);
	const double FirstDueAt = EmitterStartedAt + FMath::Max(0.0f, EmitterFirstShotDelaySeconds);
	EmitterNextShotIndex = FMath::Max(EmitterNextShotIndex,
		FMath::FloorToInt((Now - FirstDueAt) / Interval) + 1);
	ScheduleEmitterShot();
}

void ADeviceBase::ExpireEmitter()
{
	FinishActivation();
}

void ADeviceBase::SpawnEmitterProjectile()
{
	if (!EmitterProjectileClass || EmitterProjectileClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return;
	}
	UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!Lifecycle || !Lifecycle->CanSpawnProjectile()) return;
	const ARuleProjectile* Defaults = EmitterProjectileClass.GetDefaultObject();
	const float Radius = Defaults->GetCollisionRadiusCm();
	const FVector Origin = PhysicsBody->GetComponentLocation();
	const FVector Muzzle = Origin + ActivationDirection
		* (PhysicsBody->GetScaledSphereRadius() + Radius + FMath::Max(0.0f, EmitterMuzzleClearanceCm));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothEmitterBirth), false, this);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	// Source-to-muzzle static sweep plus endpoint overlap prevent spawning across/inside a wall.
	FCollisionObjectQueryParams StaticObjects;
	StaticObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	FHitResult StaticHit;
	const bool bStaticBlocked = GetWorld()->SweepSingleByObjectType(StaticHit, Origin, Muzzle,
		FQuat::Identity, StaticObjects, Shape, Params)
		|| GetWorld()->OverlapAnyTestByObjectType(Muzzle, FQuat::Identity, StaticObjects, Shape, Params);
	if (bStaticBlocked)
	{
#if !UE_BUILD_SHIPPING
		if (bLogDeviceEvents)
		{
			UE_LOG(LogMothEffect, Log, TEXT("Emitter %s shot=%d consumed: static muzzle obstruction"),
				*GetName(), EmitterShotAttempts);
		}
#endif
		return;
	}
	FHitResult BirthHit;
	const bool bBirthHit = GetWorld()->SweepSingleByChannel(BirthHit, Origin, Muzzle,
		FQuat::Identity, MothCollision::Projectile, Shape, Params);
	const FTransform Transform(ActivationDirection.Rotation(), Muzzle);
	UProjectilePoolSubsystem* Pool = GetWorld()->GetSubsystem<UProjectilePoolSubsystem>();
	ARuleProjectile* Projectile = Pool ? Pool->SpawnProjectile(EmitterProjectileClass, Transform,
		ActivationDirection, this, EmitterInstigator.Get(), bLogDeviceEvents) : nullptr;
	if (!Projectile)
	{
		return;
	}
	if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Active)
	{
		Projectile->ReturnToPool();
		return;
	}
	++EmitterProjectilesSpawned;
	if (bBirthHit)
	{
		Projectile->ProcessBlockingHit(BirthHit);
	}
#if !UE_BUILD_SHIPPING
	if (bLogDeviceEvents)
	{
		UE_LOG(LogMothEffect, Log, TEXT("Emitter %s shot=%d spawned=%d direction=%s origin=%s muzzle=%s birthHit=%d"),
			*GetName(), EmitterShotAttempts, EmitterProjectilesSpawned, *ActivationDirection.ToString(),
			*Origin.ToString(), *Muzzle.ToString(), bBirthHit);
	}
#endif
}
