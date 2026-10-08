#include "Devices/DeviceBase.h"
#include "Combat/RuleProjectile.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "MothEffect.h"
#include "TimerManager.h"
#include "Types/MothCollisionChannels.h"

ADeviceBase::ADeviceBase()
{
	PrimaryActorTick.bCanEverTick = false;
	PhysicsBody = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsBody"));
	SetRootComponent(PhysicsBody);
	PhysicsBody->InitSphereRadius(BodyRadiusCm);
	PhysicsBody->SetCollisionProfileName(TEXT("PhysicsActor"));
	PhysicsBody->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Ignore);
	PhysicsBody->SetGenerateOverlapEvents(false);
	PhysicsBody->BodyInstance.bUseCCD = true;

	ShotCollider = CreateDefaultSubobject<USphereComponent>(TEXT("ShotCollider"));
	ShotCollider->SetupAttachment(PhysicsBody);
	ShotCollider->InitSphereRadius(ShotRadiusCm);
	ShotCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ShotCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	ShotCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ShotCollider->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Block);
	ShotCollider->SetGenerateOverlapEvents(false);

	DeviceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeviceMesh"));
	DeviceMesh->SetupAttachment(PhysicsBody);
	DeviceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EmitterDirectionMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("EmitterDirectionMarker"));
	EmitterDirectionMarker->SetupAttachment(PhysicsBody);
	EmitterDirectionMarker->SetAbsolute(false, true, false);
	EmitterDirectionMarker->ArrowColor = FColor::Orange;
	EmitterDirectionMarker->ArrowLength = 60.0f;
	EmitterDirectionMarker->SetHiddenInGame(false);
	EmitterDirectionMarker->SetVisibility(false, true);
	EmitterProjectileClass = ARuleProjectile::StaticClass();
}

void ADeviceBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const float BodyRadius = FMath::Max(1.0f, BodyRadiusCm);
	PhysicsBody->SetSphereRadius(BodyRadius);
	ShotCollider->SetSphereRadius(FMath::Max(BodyRadius, ShotRadiusCm));
	EmitterDirectionMarker->SetAbsolute(false, true, false);
	ApplyStateCollision();
}

void ADeviceBase::BeginPlay()
{
	Super::BeginPlay();
	PhysicsBody->SetMassOverrideInKg(NAME_None, FMath::Max(0.01f, PhysicsMassKg));
	EmitterDirectionMarker->SetVisibility(false, true);
	ApplyStateCollision();
}

void ADeviceBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	bGameplayEnabled = false;
	StopActiveEffect();
	Holder = nullptr;
	TransitionTo(EDeviceState::Destroyed);
	Super::EndPlay(EndPlayReason);
}

bool ADeviceBase::ReceiveBallisticHit_Implementation(const FHitContext& Context)
{
	return TryActivate(Context);
}

bool ADeviceBase::CanBePickedUp() const
{
	return !bEndingPlay && !IsActorBeingDestroyed() && HasActorBegunPlay() && bGameplayEnabled
		&& GetWorld() && !GetWorld()->IsPaused() && DeviceState == EDeviceState::Dormant;
}

bool ADeviceBase::TryPickup(AMothEffectCharacter* NewHolder)
{
	return IsValid(NewHolder) && NewHolder->TryPickupDevice(this);
}

bool ADeviceBase::CommitHeld(AMothEffectCharacter* NewHolder, USceneComponent* HoldPoint)
{
	if (!CanBePickedUp() || !IsValid(NewHolder) || !NewHolder->HasActorBegunPlay()
		|| NewHolder->GetWorld() != GetWorld()
		|| !NewHolder->IsGameplayEnabled() || !IsValid(HoldPoint) || HoldPoint->GetOwner() != NewHolder)
	{
		return false;
	}
	Holder = NewHolder;
	TransitionTo(EDeviceState::Held, false);
	if (!AttachToComponent(HoldPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale))
	{
		TransitionTo(EDeviceState::Dormant, false);
		return false;
	}
	// Character commits its reference/action before either object's state events run.
	return true;
}

bool ADeviceBase::CommitReleased(AMothEffectCharacter* ReleasingHolder, const FVector& Location, const FVector& Velocity)
{
	if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Held
		|| !IsValid(ReleasingHolder) || Holder != ReleasingHolder
		|| !ReleasingHolder->IsGameplayEnabled() || !GetWorld() || GetWorld()->IsPaused()
		|| Location.ContainsNaN() || Velocity.ContainsNaN())
	{
		return false;
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	TransitionTo(EDeviceState::Dormant, false);
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PhysicsBody->SetPhysicsLinearVelocity(Velocity);
	return true;
}

bool ADeviceBase::TryActivate(const FHitContext& Context)
{
	if (bEndingPlay || !HasActorBegunPlay() || !bGameplayEnabled || !GetWorld()
		|| GetWorld()->IsPaused() || DeviceState != EDeviceState::Dormant
		|| IsActorBeingDestroyed() || !Context.HitId.IsValid() || Context.ImpactPoint.ContainsNaN()
		|| Context.ShotDirection.ContainsNaN() || !FMath::IsFinite(Context.Damage) || Context.Damage < 0.0f)
	{
		LogHit(Context, false);
		return false;
	}
	const FVector Direction = Context.ShotDirection.GetSafeNormal();
	if (Direction.ContainsNaN() || Direction.IsNearlyZero())
	{
		LogHit(Context, false);
		return false;
	}

	// Save scalar/vector evidence, not references to the source actor or instigator.
	ActivationHitId = Context.HitId;
	ActivationDirection = Direction;
	ActivationImpactPoint = Context.ImpactPoint;
	FHitContext ActivationContext = Context;
	ActivationContext.ShotDirection = Direction;

	// Commit before delegates or effects can re-enter the ballistic interface.
	if (!TransitionTo(EDeviceState::Active))
	{
		LogHit(ActivationContext, false);
		return false;
	}
	LogHit(ActivationContext, true);
	if (!bEndingPlay && bGameplayEnabled && DeviceState == EDeviceState::Active)
	{
		bEffectRunning = true;
		ActivateEffect(ActivationContext);
	}
	return true;
}

bool ADeviceBase::FinishActivation()
{
	if (bEndingPlay || DeviceState != EDeviceState::Active)
	{
		return false;
	}
	TransitionTo(EDeviceState::Spent);
	StopActiveEffect();
	if (DeviceKind == EDeviceKind::Emitter && DeviceState == EDeviceState::Spent
		&& !bEndingPlay && !IsActorBeingDestroyed())
	{
		SetLifeSpan(FMath::Max(0.01f, EmitterSpentVisualSeconds));
	}
	return true;
}

void ADeviceBase::DestroyDevice()
{
	if (bEndingPlay || DeviceState == EDeviceState::Destroyed)
	{
		return;
	}
	bGameplayEnabled = false;
	Holder = nullptr;
	TransitionTo(EDeviceState::Destroyed);
	StopActiveEffect();
	Destroy();
}

void ADeviceBase::SetGameplayEnabled(bool bEnabled)
{
	if (bEndingPlay || DeviceState == EDeviceState::Destroyed)
	{
		return;
	}
	bGameplayEnabled = bEnabled;
	if (!bEnabled)
	{
		if (DeviceState == EDeviceState::Held)
		{
			DestroyDevice();
		}
		else
		{
			FinishActivation();
		}
	}
}

bool ADeviceBase::TransitionTo(EDeviceState NewState, bool bNotify)
{
	if (DeviceState == NewState || DeviceState == EDeviceState::Destroyed)
	{
		return false;
	}
	bool bAllowed = NewState == EDeviceState::Destroyed;
	switch (DeviceState)
	{
	case EDeviceState::Dormant:
		bAllowed |= NewState == EDeviceState::Active || NewState == EDeviceState::Spent
			|| (NewState == EDeviceState::Held && IsValid(Holder));
		break;
	case EDeviceState::Held:
		bAllowed |= NewState == EDeviceState::Dormant;
		break;
	case EDeviceState::Active:
		bAllowed |= NewState == EDeviceState::Spent;
		break;
	default:
		break;
	}
	if (!bAllowed)
	{
		return false;
	}
	const EDeviceState OldState = DeviceState;
	DeviceState = NewState;
	if (NewState != EDeviceState::Held)
	{
		Holder = nullptr;
	}
	ApplyStateCollision();
	if (bNotify)
	{
		NotifyStateChanged(OldState, NewState);
	}
	return true;
}

void ADeviceBase::NotifyStateChanged(EDeviceState OldState, EDeviceState NewState)
{
#if !UE_BUILD_SHIPPING
	if (bLogDeviceEvents)
	{
		UE_LOG(LogMothEffect, Log, TEXT("Device %s: %s -> %s"), *GetName(),
			*UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(NewState));
	}
#endif
	OnDeviceStateChanged.Broadcast(this, OldState, NewState);
	OnDeviceStateChangedNative.Broadcast(this, OldState, NewState);
}

void ADeviceBase::ApplyStateCollision()
{
	DeviceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const bool bPhysical = DeviceState == EDeviceState::Dormant || DeviceState == EDeviceState::Active;
	PhysicsBody->SetCollisionResponseToChannel(WeaponTraceChannel, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Ignore);
	ShotCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	ShotCollider->SetCollisionResponseToChannel(WeaponTraceChannel, ECR_Block);
	ShotCollider->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Block);
	if (bPhysical)
	{
		PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PhysicsBody->SetEnableGravity(true);
		PhysicsBody->SetSimulatePhysics(HasActorBegunPlay());
		ShotCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		PhysicsBody->SetSimulatePhysics(false);
		PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ShotCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ADeviceBase::StopActiveEffect()
{
	if (bEffectRunning)
	{
		bEffectRunning = false;
		StopEffect();
	}
}

void ADeviceBase::ActivateEffect(const FHitContext& Context)
{
	if (DeviceKind != EDeviceKind::Emitter)
	{
		// D01/D02 retain their T06 state shell until T10/T11.
		return;
	}
	EmitterInstigator = Context.InstigatorPawn.Get();
	EmitterShotAttempts = 0;
	EmitterProjectilesSpawned = 0;
	EmitterNextShotIndex = 0;
	EmitterStartedAt = GetWorld()->GetTimeSeconds();
	EmitterExpiresAt = EmitterStartedAt + FMath::Max(0.01f, EmitterDurationSeconds);
	EmitterDirectionMarker->SetWorldRotation(ActivationDirection.Rotation());
	EmitterDirectionMarker->SetVisibility(true, true);
	GetWorldTimerManager().SetTimer(EmitterExpiryTimer, this, &ADeviceBase::ExpireEmitter,
		FMath::Max(0.01f, EmitterDurationSeconds), false);
	ScheduleEmitterShot();
}

void ADeviceBase::StopEffect()
{
	GetWorldTimerManager().ClearTimer(EmitterShotTimer);
	GetWorldTimerManager().ClearTimer(EmitterExpiryTimer);
	EmitterInstigator.Reset();
	EmitterDirectionMarker->SetVisibility(false, true);
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
	ARuleProjectile* Projectile = GetWorld()->SpawnActorDeferred<ARuleProjectile>(
		EmitterProjectileClass, Transform, this, EmitterInstigator.Get(),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return;
	}
	if (!Projectile->InitializeProjectile(ActivationDirection, this, EmitterInstigator.Get()))
	{
		Projectile->Destroy();
		return;
	}
	Projectile->FinishSpawning(Transform);
	if (!IsValid(Projectile) || !Projectile->HasActorBegunPlay())
	{
		Projectile->Destroy();
		return;
	}
	if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Active)
	{
		Projectile->Destroy();
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

void ADeviceBase::LogHit(const FHitContext& Context, bool bAccepted) const
{
#if !UE_BUILD_SHIPPING
	if (bLogDeviceEvents)
	{
		UE_LOG(LogMothEffect, Log,
			TEXT("Device %s hit=%s source=%s instigator=%s point=%s direction=%s damage=%.2f accepted=%d state=%s"),
			*GetName(), *Context.HitId.ToString(), *GetNameSafe(Context.SourceActor.Get()),
			*GetNameSafe(Context.InstigatorPawn.Get()), *Context.ImpactPoint.ToString(),
			*Context.ShotDirection.ToString(), Context.Damage, bAccepted,
			*UEnum::GetValueAsString(DeviceState));
	}
#endif
}
