#include "Combat/RuleProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interfaces/BallisticReactive.h"
#include "Types/MothCollisionChannels.h"

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
	if (bInitialized || bEndingPlay || bImpactProcessed || WorldDirection.ContainsNaN()
		|| Direction.ContainsNaN() || Direction.IsNearlyZero()
		|| (IsValid(SourceActor) && SourceActor->GetWorld() != GetWorld()))
	{
		return false;
	}
	bInitialized = true;
	HitId = FGuid::NewGuid();
	FlightDirection = Direction;
	FiringSource = SourceActor;
	FiringInstigator = InstigatorPawn;
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
		|| bEndingPlay || bImpactProcessed || IsActorBeingDestroyed())
	{
		Destroy();
		return;
	}
	CollisionSphere->SetSimulatePhysics(false);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->InitialSpeed = FMath::Max(1.0f, SpeedCmPerSec);
	ProjectileMovement->MaxSpeed = ProjectileMovement->InitialSpeed;
	ProjectileMovement->Velocity = FlightDirection * ProjectileMovement->InitialSpeed;
	ProjectileMovement->Activate(true);
	SetLifeSpan(FMath::Max(0.01f, LifetimeSeconds));
}

void ARuleProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	ProjectileMovement->Deactivate();
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionSphere->OnComponentHit.RemoveDynamic(this, &ARuleProjectile::HandleComponentHit);
	ProjectileMovement->OnProjectileStop.RemoveDynamic(this, &ARuleProjectile::HandleMovementStopped);
	Super::EndPlay(EndPlayReason);
}

float ARuleProjectile::GetCollisionRadiusCm() const
{
	// Blueprint class defaults do not run OnConstruction. Read the configured radius
	// instead of the sphere's possibly stale native 4cm value when planning a birth.
	return FMath::Max(0.1f, CollisionRadiusCm) * CollisionSphere->GetShapeScale();
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
	if (bEndingPlay || bImpactProcessed || !bInitialized || !HasActorBegunPlay()
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
	// Commit before calling any interface/delegate: rejection still consumes the projectile.
	bImpactProcessed = true;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	const bool bAccepted = IsValid(HitActor) && HitActor->Implements<UBallisticReactive>()
		&& IBallisticReactive::Execute_ReceiveBallisticHit(HitActor, Context);
	if (!bEndingPlay && !IsActorBeingDestroyed())
	{
		OnImpactNative.Broadcast(this, HitActor, Context);
		if (!bEndingPlay && !IsActorBeingDestroyed())
		{
			OnProjectileImpact(Context, HitActor, bAccepted);
			Destroy();
		}
	}
	return true;
}
