#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "Combat/RuleProjectile.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ExternalImpulseComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "MothEffect.h"
#include "TimerManager.h"
#include "Types/MothCollisionChannels.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Feedback/MothDeviceFeedbackComponent.h"
#include "Feedback/MothFeedbackSubsystem.h"

ADeviceBase::ADeviceBase()
{
	PrototypeArtProfile = TSoftObjectPtr<UMothPrototypeArtProfile>(FSoftObjectPath(UMothPrototypeArtProfile::DefaultPath()));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	DefaultFeedback = CreateDefaultSubobject<UMothDeviceFeedbackComponent>(TEXT("DefaultFeedback"));
	ExternalImpulseComponent = CreateDefaultSubobject<UExternalImpulseComponent>(TEXT("ExternalImpulseComponent"));
	PhysicsBody = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsBody"));
	SetRootComponent(PhysicsBody);
	PhysicsBody->InitSphereRadius(BodyRadiusCm);
	PhysicsBody->SetCollisionProfileName(TEXT("PhysicsActor"));
	PhysicsBody->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
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
	ShotCollider->SetCanEverAffectNavigation(false);

	DroppedCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("DroppedCollider"));
	DroppedCollider->SetupAttachment(PhysicsBody);
	DroppedCollider->InitCapsuleSize(MothCollision::EnemyCapsuleRadiusCm, MothCollision::EnemyCapsuleHalfHeightCm);
	DroppedCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DroppedCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	DroppedCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	DroppedCollider->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Block);
	DroppedCollider->SetGenerateOverlapEvents(false);
	DroppedCollider->SetCanEverAffectNavigation(false);

	VisualPivot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualPivot"));
	VisualPivot->SetupAttachment(PhysicsBody);

	DeviceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeviceMesh"));
	DeviceMesh->SetupAttachment(VisualPivot);
	DeviceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DeviceMesh->SetGenerateOverlapEvents(false);
	DeviceMesh->SetCanEverAffectNavigation(false);

	EmitterDirectionMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("EmitterDirectionMarker"));
	EmitterDirectionMarker->SetupAttachment(PhysicsBody);
	EmitterDirectionMarker->SetAbsolute(false, true, false);
	EmitterDirectionMarker->ArrowColor = FColor::Orange;
	EmitterDirectionMarker->ArrowLength = 60.0f;
	EmitterDirectionMarker->SetHiddenInGame(false);
	EmitterDirectionMarker->SetVisibility(false, true);
	EmitterProjectileClass = ARuleProjectile::StaticClass();
	LauncherColumnClass = ALaunchColumn::StaticClass();
	LauncherFeedback = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LauncherFeedback"));
	LauncherFeedback->SetupAttachment(PhysicsBody);
	LauncherFeedback->SetAbsolute(false, true, true);
	LauncherFeedback->SetRelativeLocation(FVector(0.0, 0.0, 65.0));
	LauncherFeedback->SetHorizontalAlignment(EHTA_Center);
	LauncherFeedback->SetWorldSize(24.0f);
	LauncherFeedback->SetTextRenderColor(FColor::Red);
	LauncherFeedback->SetVisibility(false);
	LifecycleFeedback = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LifecycleFeedback"));
	LifecycleFeedback->SetupAttachment(PhysicsBody);
	LifecycleFeedback->SetAbsolute(false, true, true);
	LifecycleFeedback->SetRelativeLocation(FVector(0.0, 0.0, 90.0));
	LifecycleFeedback->SetHorizontalAlignment(EHTA_Center);
	LifecycleFeedback->SetWorldSize(22.0f);
	LifecycleFeedback->SetTextRenderColor(FColor::Yellow);
	LifecycleFeedback->SetVisibility(false);
}

void ADeviceBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EmitterDirectionMarker->SetAbsolute(false, true, false);
	RefreshDeviceSize();
	ApplyStateCollision();
}

void ADeviceBase::BeginPlay()
{
	UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	const bool bAdmitted = Lifecycle && Lifecycle->RegisterDevice(this);
	if (!bAdmitted) bGameplayEnabled = false;
	IdleSegmentStartedAt = GetWorld()->GetTimeSeconds();
	Super::BeginPlay();
	if (!bAdmitted) { DestroyDevice(); return; }
	if (bEndingPlay || IsActorBeingDestroyed()) return;
	RefreshPrototypeAppearance();
	PhysicsBody->SetMassOverrideInKg(NAME_None, FMath::Max(0.01f, PhysicsMassKg));
	EmitterDirectionMarker->SetVisibility(false, true);
	ApplyStateCollision();
	UpdateGrounding();
}

void ADeviceBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	bGameplayEnabled = false;
	StopActiveEffect();
	if (IsValid(LaunchColumn)) LaunchColumn->Destroy();
	LaunchColumn = nullptr;
	if (UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()) Lifecycle->UnregisterDevice(this);
	Holder = nullptr;
	TransitionTo(EDeviceState::Destroyed);
	OnActivatedNative.Clear();
	Super::EndPlay(EndPlayReason);
}

bool ADeviceBase::ReceiveBallisticHit_Implementation(const FHitContext& Context)
{
	return TryActivate(Context);
}

bool ADeviceBase::CanReceiveExternalImpulse() const
{
	const auto* Lifecycle = GetWorld() ? GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>() : nullptr;
	return HasActorBegunPlay() && bGameplayEnabled && !bEndingPlay && !IsActorBeingDestroyed()
		&& GetWorld() && !GetWorld()->IsPaused() && Lifecycle && Lifecycle->IsGameplayEnabled()
		&& (DeviceState == EDeviceState::Dormant
			|| (DeviceState == EDeviceState::Active && DeviceKind == EDeviceKind::Emitter));
}

void ADeviceBase::ReceiveImpulse_Implementation(FVector Velocity, bool bOverrideZ, FGuid EventId)
{
	if (CanReceiveExternalImpulse()) ExternalImpulseComponent->QueueImpulse(Velocity, bOverrideZ, EventId);
}

bool ADeviceBase::CanBePickedUp() const
{
	return !bEndingPlay && !IsActorBeingDestroyed() && HasActorBegunPlay() && bGameplayEnabled
		&& GetWorld() && !GetWorld()->IsPaused() && DeviceState == EDeviceState::Dormant && GetRemainingIdleSeconds() > 0.0f;
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
	bGrounded = false;
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
	bGrounded = false;
	GroundingAllowedAt = GetWorld()->GetTimeSeconds() + (Velocity.IsNearlyZero() ? 0.0 : 0.05);
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	TransitionTo(EDeviceState::Dormant, false);
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PhysicsBody->SetPhysicsLinearVelocity(Velocity);
	if (Velocity.IsNearlyZero()) UpdateGrounding();
	return true;
}

bool ADeviceBase::TryActivate(const FHitContext& Context)
{
	if (bEndingPlay || !HasActorBegunPlay() || !bGameplayEnabled || !GetWorld()
		|| GetWorld()->IsPaused() || DeviceState != EDeviceState::Dormant
		|| IsActorBeingDestroyed() || !Context.HitId.IsValid() || Context.ImpactPoint.ContainsNaN()
		|| Context.ShotDirection.ContainsNaN() || !FMath::IsFinite(Context.Damage) || Context.Damage < 0.0f
		|| !FMath::IsFinite(ActivationDelaySeconds) || ActivationDelaySeconds < 0.0f)
	{
		LogHit(Context, false);
		return false;
	}
	FVector Direction = Context.ShotDirection.GetSafeNormal();
	if (Direction.ContainsNaN() || Direction.IsNearlyZero())
	{
		LogHit(Context, false);
		return false;
	}
	if (DeviceKind == EDeviceKind::Emitter)
	{
		Direction = Direction.GetSafeNormal2D();
		// A vertical hit has no horizontal heading; use a stable world direction.
		if (Direction.IsNearlyZero()) Direction = FVector::ForwardVector;
	}
	UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (GetRemainingIdleSeconds() <= 0.0f || !Lifecycle || !Lifecycle->ReserveActivation(this))
	{
		LogHit(Context, false);
		return false;
	}
	if (bEndingPlay || IsActorBeingDestroyed() || !bGameplayEnabled || DeviceState != EDeviceState::Dormant)
	{
		Lifecycle->CommitDeviceState(this);
		return false;
	}

	// Save scalar/vector evidence, not references to the source actor or instigator.
	ActivationHitId = Context.HitId;
	ActivationDirection = Direction;
	ActivationImpactPoint = Context.ImpactPoint;
	if (DeviceKind == EDeviceKind::Bomb) ExplosionCenter = GetActorLocation();
	if (DeviceKind == EDeviceKind::Launcher) LauncherOrigin = GetActorLocation();
	FHitContext ActivationContext = Context;
	ActivationContext.ShotDirection = Direction;
	PendingActivationSource = Context.SourceActor.Get();
	PendingActivationInstigator = Context.InstigatorPawn.Get();
	PendingActivationDamage = Context.Damage;
	bActivationPending = true;

	// Commit before delegates or effects can re-enter the ballistic interface.
	if (!TransitionTo(EDeviceState::Active))
	{
		StopActiveEffect();
		Lifecycle->CommitDeviceState(this);
		LogHit(ActivationContext, false);
		return false;
	}
	LogHit(ActivationContext, true);
	if (!bEndingPlay && bGameplayEnabled && DeviceState == EDeviceState::Active)
	{
		if (DeviceKind == EDeviceKind::Emitter)
		{
			EmitterDirectionMarker->SetWorldRotation(ActivationDirection.Rotation());
			EmitterDirectionMarker->SetVisibility(true, true);
		}
		OnActivatedNative.Broadcast(this, ActivationContext);
		if (!bEndingPlay && !IsActorBeingDestroyed() && bGameplayEnabled && DeviceState == EDeviceState::Active)
		{
			GetWorld()->GetSubsystem<UMothFeedbackSubsystem>()->Emit(EMothFeedbackCue::Hit, ActivationImpactPoint, ActivationDirection,
				IsValid(Context.InstigatorPawn) && Context.InstigatorPawn->IsPlayerControlled() && Context.InstigatorPawn->IsLocallyControlled());
			if (ActivationDelaySeconds > 0.0f)
			{
				GetWorldTimerManager().SetTimer(ActivationDelayTimer, this,
					&ADeviceBase::BeginActivationEffect, ActivationDelaySeconds, false);
			}
			else BeginActivationEffect();
		}
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
	if (DeviceState == EDeviceState::Spent
		&& !bEndingPlay && !IsActorBeingDestroyed())
	{
		const float SpentSeconds = DeviceKind == EDeviceKind::Bomb ? BombSpentVisualSeconds
			: DeviceKind == EDeviceKind::Emitter ? EmitterSpentVisualSeconds
			: FMath::Max(LauncherSpentVisualSeconds, IsValid(LaunchColumn) ? LaunchColumn->GetRetractSeconds() + 0.05f : 0.0f);
		SetLifeSpan(FMath::Max(0.01f, SpentSeconds));
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
	if (IsValid(LaunchColumn)) LaunchColumn->Destroy();
	LaunchColumn = nullptr;
	Destroy();
}

void ADeviceBase::SetGameplayEnabled(bool bEnabled)
{
	if (bEndingPlay || DeviceState == EDeviceState::Destroyed)
	{
		return;
	}
	AccumulateIdleTime();
	bGameplayEnabled = bEnabled;
	ExternalImpulseComponent->SetGameplayEnabled(bEnabled);
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
	AccumulateIdleTime();
	DeviceState = NewState;
	if (UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()) Lifecycle->CommitDeviceState(this);
	if (NewState != EDeviceState::Dormant)
	{
		bExpiryWarningActive = false;
		LifecycleFeedback->SetVisibility(false);
	}
	if (!CanReceiveExternalImpulse()) ExternalImpulseComponent->ClearPending();
	if (NewState != EDeviceState::Held)
	{
		Holder = nullptr;
	}
	ApplyStateCollision();
	if (DefaultFeedback && DefaultFeedback->HasBegunPlay()) DefaultFeedback->RefreshPresentation();
	if (bNotify)
	{
		NotifyStateChanged(OldState, NewState);
	}
	if (NewState == EDeviceState::Spent && DeviceState == EDeviceState::Spent && bGameplayEnabled && !bEndingPlay && !IsActorBeingDestroyed())
		GetWorld()->GetSubsystem<UMothFeedbackSubsystem>()->Emit(EMothFeedbackCue::Spent, GetActorLocation()+FVector::UpVector*60);
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
	const bool bPhysical = DeviceState == EDeviceState::Dormant
		|| (DeviceState == EDeviceState::Active && DeviceKind != EDeviceKind::Launcher);
	if (DeviceKind == EDeviceKind::Launcher)
		DeviceMesh->SetVisibility(DeviceState == EDeviceState::Dormant || DeviceState == EDeviceState::Held
			|| (DeviceState == EDeviceState::Spent && LauncherFailureReason != ELauncherFailureReason::None));
	PhysicsBody->SetCollisionResponseToChannel(WeaponTraceChannel, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Ignore);
	ShotCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	ShotCollider->SetCollisionResponseToChannel(WeaponTraceChannel, ECR_Block);
	ShotCollider->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Block);
	DroppedCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	DroppedCollider->SetCollisionResponseToChannel(WeaponTraceChannel, ECR_Block);
	DroppedCollider->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Block);
	const bool bDropped = DeviceState == EDeviceState::Dormant && bGrounded;
	DroppedCollider->SetCollisionEnabled(bDropped ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (bPhysical)
	{
		// Stop the body before removing physics collision; Chaos must never see incompatible options.
		if (bGrounded) PhysicsBody->SetSimulatePhysics(false);
		PhysicsBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		PhysicsBody->SetCollisionEnabled(bGrounded ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		PhysicsBody->SetEnableGravity(true);
		if (!bGrounded) PhysicsBody->SetSimulatePhysics(HasActorBegunPlay());
		ShotCollider->SetCollisionEnabled(bDropped ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}
	else
	{
		PhysicsBody->SetSimulatePhysics(false);
		PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ShotCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	UpdateDroppedPresentation(0.0f);
}

void ADeviceBase::StopActiveEffect()
{
	GetWorldTimerManager().ClearTimer(ActivationDelayTimer);
	bActivationPending = false;
	PendingActivationSource.Reset();
	PendingActivationInstigator.Reset();
	EmitterDirectionMarker->SetVisibility(false, true);
	if (bEffectRunning)
	{
		bEffectRunning = false;
		StopEffect();
	}
}

void ADeviceBase::BeginActivationEffect()
{
	GetWorldTimerManager().ClearTimer(ActivationDelayTimer);
	const UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!bActivationPending || bEndingPlay || IsActorBeingDestroyed() || !bGameplayEnabled
		|| DeviceState != EDeviceState::Active || !Lifecycle || !Lifecycle->IsGameplayEnabled())
	{
		StopActiveEffect();
		return;
	}
	FHitContext Context;
	Context.HitId = ActivationHitId;
	Context.SourceActor = PendingActivationSource.Get();
	Context.InstigatorPawn = PendingActivationInstigator.Get();
	Context.ImpactPoint = ActivationImpactPoint;
	Context.ShotDirection = ActivationDirection;
	Context.Damage = PendingActivationDamage;
	bActivationPending = false;
	PendingActivationSource.Reset();
	PendingActivationInstigator.Reset();
	bEffectRunning = true;
	ActivateEffect(Context);
}

void ADeviceBase::ActivateEffect(const FHitContext& Context)
{
	if (DeviceKind == EDeviceKind::Bomb)
	{
		ExplodeBomb(Context);
		return;
	}
	if (DeviceKind == EDeviceKind::Launcher)
	{
		StartLauncher();
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
	GetWorld()->GetSubsystem<UMothFeedbackSubsystem>()->Emit(EMothFeedbackCue::Emitter, GetActorLocation(), ActivationDirection);
	GetWorldTimerManager().SetTimer(EmitterExpiryTimer, this, &ADeviceBase::ExpireEmitter,
		FMath::Max(0.01f, EmitterDurationSeconds), false);
	ScheduleEmitterShot();
}

void ADeviceBase::StopEffect()
{
	if (IsValid(LaunchColumn))
	{
		if (bEndingPlay || IsActorBeingDestroyed() || DeviceState == EDeviceState::Destroyed) LaunchColumn->Destroy();
		else LaunchColumn->BeginRetraction();
	}
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
	Projectile->SetEventLoggingEnabled(bLogDeviceEvents);
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

float ADeviceBase::GetDormantIdleSeconds() const
{
	const double CurrentSegment = DeviceState == EDeviceState::Dormant && bGameplayEnabled && HasActorBegunPlay() && GetWorld()
		? FMath::Max(0.0, static_cast<double>(GetWorld()->GetTimeSeconds()) - IdleSegmentStartedAt) : 0.0;
	return static_cast<float>(DormantIdleElapsed + CurrentSegment);
}

float ADeviceBase::GetRemainingIdleSeconds() const
{
	const UDeviceLifecycleSubsystem* Lifecycle = GetWorld() ? GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>() : nullptr;
	return FMath::Max(0.0f, (Lifecycle ? Lifecycle->GetIdleLifetimeSeconds() : 30.0f) - GetDormantIdleSeconds());
}

void ADeviceBase::AccumulateIdleTime()
{
	DormantIdleElapsed = GetDormantIdleSeconds();
	IdleSegmentStartedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void ADeviceBase::UpdateIdleLifecycle(float Lifetime, float WarningSeconds, float SpentSeconds)
{
	if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Dormant) return;
	const float Remaining = FMath::Max(0.0f, Lifetime - GetDormantIdleSeconds());
	if (Remaining <= 0.0f)
	{
		TransitionTo(EDeviceState::Spent);
		if (bEndingPlay || IsActorBeingDestroyed() || DeviceState != EDeviceState::Spent) return;
		LifecycleFeedback->SetText(FText::FromString(TEXT("EXPIRED")));
		LifecycleFeedback->SetTextRenderColor(FColor::Red);
		LifecycleFeedback->SetVisibility(true);
		SetLifeSpan(SpentSeconds);
		return;
	}
	if (WarningSeconds > 0.0f && Remaining <= WarningSeconds)
	{
		LifecycleFeedback->SetText(FText::FromString(FString::Printf(TEXT("EXPIRES %ds"), FMath::CeilToInt(Remaining))));
		LifecycleFeedback->SetVisibility(true);
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			if (PC->GetPawn()) LifecycleFeedback->SetWorldRotation((PC->GetPawn()->GetActorLocation() - LifecycleFeedback->GetComponentLocation()).Rotation());
		if (!bExpiryWarningActive)
		{
			bExpiryWarningActive = true;
			GetWorld()->GetSubsystem<UMothFeedbackSubsystem>()->Emit(EMothFeedbackCue::Expiry, GetActorLocation());
#if !UE_BUILD_SHIPPING
			if (bLogDeviceEvents) UE_LOG(LogMothEffect, Log, TEXT("Device %s expiryWarning remaining=%.3f idle=%.3f"), *GetName(), Remaining, GetDormantIdleSeconds());
#endif
			OnDormantExpiryWarning.Broadcast(this, Remaining);
		}
	}
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
