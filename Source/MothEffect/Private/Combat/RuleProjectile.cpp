#include "Combat/RuleProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interfaces/BallisticReactive.h"
#include "MothEffect.h"
#include "Types/MothCollisionChannels.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Subsystems/ProjectilePoolSubsystem.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "Devices/DeviceBase.h"
#include "Engine/LatentActionManager.h"
#include "TimerManager.h"

ARuleProjectile::ARuleProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(CollisionRadiusCm);
	CollisionSphere->SetCollisionObjectType(MothCollision::Projectile);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionSphere->SetGenerateOverlapEvents(false);
	CollisionSphere->OnComponentHit.AddDynamic(this, &ARuleProjectile::HandleComponentHit);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bSweepCollision = true;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ARuleProjectile::HandleMovementStopped);
}

void ARuleProjectile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	CollisionSphere->SetSphereRadius(FMath::Max(0.1f, CollisionRadiusCm));
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ARuleProjectile::InitializeProjectile(const FVector& WorldDirection, AActor* SourceActor, APawn* InstigatorPawn)
{
	const FVector Direction = WorldDirection.GetSafeNormal();
	if (bInitialized || bEndingPlay || bImpactProcessed || bPoolInactive || bReturnPending || WorldDirection.ContainsNaN()
		|| Direction.ContainsNaN() || Direction.IsNearlyZero()
		|| (IsValid(SourceActor) && SourceActor->GetWorld() != GetWorld())
		|| (IsValid(InstigatorPawn) && InstigatorPawn->GetWorld() != GetWorld()))
	{
		return false;
	}
	UDeviceLifecycleSubsystem* Lifecycle = GetWorld() ? GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>() : nullptr;
	if (!Lifecycle) return false;
	const uint64 Generation = Lifecycle->GetLifecycleGeneration();
	if (!Lifecycle->RegisterProjectile(this)) return false;
	if (!Lifecycle->IsGenerationCurrent(Generation))
	{
		Lifecycle->UnregisterProjectile(this);
		return false;
	}
	ActivationGeneration = Generation;
	bInitialized = true;
	HitId = FGuid::NewGuid();
	FlightDirection = Direction;
	FiringSource = SourceActor;
	FiringInstigator = InstigatorPawn;
	bFiredByDevice = Cast<ADeviceBase>(SourceActor) != nullptr;
	// The activating pawn is attribution, not immunity. Ignore only the firing actor.
	if (IsValid(SourceActor))
	{
		CollisionSphere->IgnoreActorWhenMoving(SourceActor, true);
	}
	return true;
}

void ARuleProjectile::BeginPlay()
{
	Super::BeginPlay();
	if ((!bInitialized && !InitializeProjectile(GetActorForwardVector(), GetOwner(), GetInstigator()))
		|| !StartFlight())
	{
		ReturnToPool();
	}
}

bool ARuleProjectile::IsActivationCurrent() const
{
	const UDeviceLifecycleSubsystem* Lifecycle = GetWorld() ? GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>() : nullptr;
	return bInitialized && !bPoolInactive && !bReturnPending && !bEndingPlay && !IsActorBeingDestroyed()
		&& Lifecycle && Lifecycle->IsGenerationCurrent(ActivationGeneration);
}

bool ARuleProjectile::StartFlight()
{
	if (!IsActivationCurrent() || bImpactProcessed) return false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(PrimaryActorTick.bStartWithTickEnabled);
	CollisionSphere->SetSimulatePhysics(false);
	CollisionSphere->SetSphereRadius(FMath::Max(0.1f, CollisionRadiusCm));
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// StopSimulating clears UpdatedComponent after a hit; Activate alone cannot restore flight.
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->ResetInterpolation();
	ProjectileMovement->ClearPendingForce(true);
	ProjectileMovement->bSimulationEnabled = true;
	ProjectileMovement->PreviousHitTime = 1.0f;
	ProjectileMovement->PreviousHitNormal = FVector::ZeroVector;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->InitialSpeed = FMath::Max(1.0f, SpeedCmPerSec);
	ProjectileMovement->MaxSpeed = ProjectileMovement->InitialSpeed;
	ProjectileMovement->Velocity = FlightDirection * ProjectileMovement->InitialSpeed;
	ProjectileMovement->UpdateComponentVelocity();
	ProjectileMovement->Activate(true);
	SetLifeSpan(FMath::Max(0.01f, LifetimeSeconds));
	if (auto* Feedback = GetWorld()->GetSubsystem<UMothFeedbackSubsystem>())
		Feedback->Emit(bFiredByDevice ? EMothFeedbackCue::EmitterShot : EMothFeedbackCue::EnemyShot,
			GetActorLocation(), FlightDirection);
	if (!IsActivationCurrent()) return false;
#if !UE_BUILD_SHIPPING
	if (bLogProjectileEvents)
		UE_LOG(LogMothEffect, Log, TEXT("Projectile %s event=spawn hit=%s source=%s instigator=%s position=%s direction=%s speed=%.1f lifetime=%.2f"),
			*GetName(), *HitId.ToString(), *GetNameSafe(FiringSource.Get()), *GetNameSafe(FiringInstigator.Get()),
			*GetActorLocation().ToString(), *FlightDirection.ToString(), ProjectileMovement->InitialSpeed, GetLifeSpan());
#endif
	return true;
}

void ARuleProjectile::PreparePoolActivation(const FTransform& Transform, AActor* SourceActor, APawn* InstigatorPawn, bool bLogEvents)
{
	StopFlight();
	bPoolInactive = false;
	bReturnPending = false;
	bInitialized = false;
	bImpactProcessed = false;
	bFiredByDevice = false;
	HitId.Invalidate();
	FlightDirection = FVector::ZeroVector;
	FiringSource.Reset();
	FiringInstigator.Reset();
	CollisionSphere->ClearMoveIgnoreActors();
	CollisionSphere->ClearMoveIgnoreComponents();
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	SetOwner(SourceActor);
	SetInstigator(InstigatorPawn);
	SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	bLogProjectileEvents = bLogEvents;
}

void ARuleProjectile::StopFlight()
{
	SetLifeSpan(0.0f);
	if (GetWorld())
		if (auto* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()) Lifecycle->UnregisterProjectile(this);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->ClearPendingForce(true);
	ProjectileMovement->Deactivate();
}

void ARuleProjectile::DeactivateForPool()
{
	// Mark dormant before mutating components so any nested return is idempotent.
	bPoolInactive = true;
	StopFlight();
	GetWorldTimerManager().ClearAllTimersForObject(this);
	GetWorld()->GetLatentActionManager().RemoveActionsForObject(this);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	ProjectileMovement->SetUpdatedComponent(nullptr);
	CollisionSphere->ClearMoveIgnoreActors();
	CollisionSphere->ClearMoveIgnoreComponents();
	SetOwner(nullptr);
	SetInstigator(nullptr);
	FiringSource.Reset();
	FiringInstigator.Reset();
	HitId.Invalidate();
	FlightDirection = FVector::ZeroVector;
	ActivationGeneration = 0;
	bFiredByDevice = false;
	bInitialized = false;
	bImpactProcessed = false;
	bReturnPending = false;
	OnImpactNative.Clear();
}

void ARuleProjectile::ReturnToPool()
{
	if (bEndingPlay || IsActorBeingDestroyed()) return;
	if (UProjectilePoolSubsystem* Pool = OwningPool.Get()) Pool->RecycleProjectile(this);
	else Destroy();
}

void ARuleProjectile::ForgetSourceActor(AActor* Actor)
{
	if (!Actor) return;
	// An actor pointer can represent another life after checkout. Its new life must
	// neither inherit the old shot's source immunity nor receive its attribution.
	CollisionSphere->IgnoreActorWhenMoving(Actor, false);
	if (FiringSource.Get() == Actor) FiringSource.Reset();
	if (FiringInstigator.Get() == Actor) FiringInstigator.Reset();
	if (GetOwner() == Actor) SetOwner(nullptr);
	if (GetInstigator() == Actor) SetInstigator(nullptr);
}

void ARuleProjectile::LifeSpanExpired()
{
#if !UE_BUILD_SHIPPING
	if (bLogProjectileEvents && !bImpactProcessed)
		UE_LOG(LogMothEffect, Log, TEXT("Projectile %s event=expired hit=%s source=%s position=%s"),
			*GetName(), *HitId.ToString(), *GetNameSafe(FiringSource.Get()), *GetActorLocation().ToString());
#endif
	ReturnToPool();
}

void ARuleProjectile::FellOutOfWorld(const UDamageType& DamageType)
{
	if (OwningPool.IsValid()) ReturnToPool();
	else Super::FellOutOfWorld(DamageType);
}

void ARuleProjectile::OutsideWorldBounds()
{
	if (OwningPool.IsValid()) ReturnToPool();
	else Super::OutsideWorldBounds();
}

void ARuleProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	StopFlight();
	CollisionSphere->OnComponentHit.RemoveDynamic(this, &ARuleProjectile::HandleComponentHit);
	ProjectileMovement->OnProjectileStop.RemoveDynamic(this, &ARuleProjectile::HandleMovementStopped);
	Super::EndPlay(EndPlayReason);
}

float ARuleProjectile::GetCollisionRadiusCm() const
{
	// Blueprint class defaults do not run OnConstruction. Read the configured radius
	// instead of the sphere's possibly stale native 4cm value when planning a birth.
	// Their world-transform cache also need not reflect serialized root scale yet.
	const float Scale = HasAnyFlags(RF_ClassDefaultObject)
		? CollisionSphere->GetRelativeScale3D().GetAbsMin()
		: CollisionSphere->GetShapeScale();
	return FMath::Max(0.1f, CollisionRadiusCm) * Scale;
}

void ARuleProjectile::HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	ProcessBlockingHit(Hit);
}

void ARuleProjectile::HandleMovementStopped(const FHitResult& Hit)
{
	ProcessBlockingHit(Hit);
}

bool ARuleProjectile::ProcessBlockingHit(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!IsActivationCurrent() || bImpactProcessed || !HasActorBegunPlay()
		|| !GetWorld() || GetWorld()->IsPaused() || !Hit.bBlockingHit
		|| (FiringSource.IsValid() && HitActor == FiringSource.Get()))
	{
		return false;
	}

	FHitContext Context;
	Context.HitId = HitId;
	Context.SourceActor = FiringSource.Get();
	Context.InstigatorPawn = FiringInstigator.Get();
	Context.ImpactPoint = Hit.ImpactPoint;
	Context.ShotDirection = FlightDirection;
	Context.Damage = FMath::Max(0.0f, Damage);
	Context.bDeviceSource = bFiredByDevice;
	// Commit before calling any interface/delegate: rejection still consumes the projectile.
	bImpactProcessed = true;
	bImpactDispatching = true;
	StopFlight();
	const bool bAccepted = IsValid(HitActor) && HitActor->Implements<UBallisticReactive>()
		&& IBallisticReactive::Execute_ReceiveBallisticHit(HitActor, Context);
#if !UE_BUILD_SHIPPING
	if (bLogProjectileEvents)
		UE_LOG(LogMothEffect, Log, TEXT("Projectile %s event=impact hit=%s source=%s instigator=%s target=%s point=%s direction=%s accepted=%d"),
			*GetName(), *HitId.ToString(), *GetNameSafe(Context.SourceActor.Get()), *GetNameSafe(Context.InstigatorPawn.Get()),
			*GetNameSafe(HitActor), *Context.ImpactPoint.ToString(), *FlightDirection.ToString(), bAccepted);
#endif
	if (!bEndingPlay && !IsActorBeingDestroyed())
	{
		if (auto* Feedback = GetWorld()->GetSubsystem<UMothFeedbackSubsystem>())
			Feedback->Emit(EMothFeedbackCue::Impact, Hit.ImpactPoint+Hit.ImpactNormal*2, Hit.ImpactNormal);
		OnImpactNative.Broadcast(this, HitActor, Context);
		if (!bEndingPlay && !IsActorBeingDestroyed())
		{
			OnProjectileImpact(Context, HitActor, bAccepted);
		}
	}
	bImpactDispatching = false;
	ReturnToPool();
	return true;
}
