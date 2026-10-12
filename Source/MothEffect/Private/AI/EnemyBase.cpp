#include "AI/EnemyBase.h"
#include "AIController.h"
#include "Combat/RuleProjectile.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ExternalImpulseComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MothEffect.h"
#include "NavigationSystem.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Types/MothCollisionChannels.h"
#include "UObject/ConstructorHelpers.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	BaseEyeHeight = 60.0f;
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(MothCollision::Projectile, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(MothCollision::GroundTrace, ECR_Ignore);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCanEverAffectNavigation(false);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0, 540, 0);
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	ExternalImpulseComponent = CreateDefaultSubobject<UExternalImpulseComponent>(TEXT("ExternalImpulseComponent"));
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCanEverAffectNavigation(false);
	BodyMesh->SetRelativeScale3D(FVector(0.55, 0.55, 1.4));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded()) BodyMesh->SetStaticMesh(Cube.Object);
	TelegraphText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TelegraphText"));
	TelegraphText->SetupAttachment(GetCapsuleComponent());
	TelegraphText->SetRelativeLocation(FVector(0, 0, 130));
	TelegraphText->SetWorldSize(28);
	TelegraphText->SetHorizontalAlignment(EHTA_Center);
	TelegraphText->SetTextRenderColor(FColor::Red);
	TelegraphText->SetText(FText::FromString(TEXT("FIRING")));
	TelegraphText->SetVisibility(false);
	TelegraphText->SetAbsolute(false, true, false);
	AimArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("AimArrow"));
	AimArrow->SetupAttachment(GetCapsuleComponent());
	AimArrow->SetArrowColor(FColor::Red);
	AimArrow->SetArrowSize(1.5f);
	AimArrow->SetHiddenInGame(true);
	AimArrow->SetCanEverAffectNavigation(false);
	ProjectileClass = ARuleProjectile::StaticClass();
}

void AEnemyBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	HealthComponent->ConfigureMaxHealth(MaxHealth);
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(1.0f, WalkSpeedCmPerSec);
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDied.AddDynamic(this, &AEnemyBase::HandleEnemyDied);
	ProgressLocation = GetActorLocation();
	LastProgressAt = GetWorld()->GetTimeSeconds();
	NextFireAt = LastProgressAt + FMath::Max(0.01f, TelegraphSeconds);
	LogEvent(TEXT("begin"));
}

bool AEnemyBase::IsCombatEnabled() const
{
	return bGameplayEnabled && !bDeathProcessed && !bEndingPlay && !IsActorBeingDestroyed()
		&& HasActorBegunPlay() && HealthComponent->IsAlive() && GetWorld() && !GetWorld()->IsPaused();
}

bool AEnemyBase::IsUsableTarget(const APawn* Target) const
{
	if (!IsValid(Target) || Target == this || Target->IsActorBeingDestroyed() || Target->GetWorld() != GetWorld()) return false;
	const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
	if (!Health || !Health->IsAlive()) return false;
	const AMothEffectCharacter* Player = Cast<AMothEffectCharacter>(Target);
	return !Player || Player->IsGameplayEnabled();
}

bool AEnemyBase::SetCombatTarget(APawn* Target)
{
	if (Target && !IsUsableTarget(Target)) return false;
	if (CombatTarget.Get() == Target) return true;
	++CombatGeneration;
	CombatTarget = Target;
	LastSeenAt = -1.0e9;
	NextRepathAt = 0;
	bRecovering = false;
	ProgressLocation = GetActorLocation();
	LastProgressAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0;
	StopNavigation();
	if (!bDeathProcessed && !bEndingPlay) CombatPhase = bGameplayEnabled ? EEnemyCombatPhase::Idle : EEnemyCombatPhase::Disabled;
	SetTelegraph(false);
	return true;
}

FVector AEnemyBase::AimPoint() const
{
	return CombatTarget.IsValid() ? CombatTarget->GetPawnViewLocation() : GetPawnViewLocation();
}

bool AEnemyBase::HasTargetSight() const
{
	if (!IsUsableTarget(CombatTarget.Get()) || FVector::DistSquared(GetActorLocation(), CombatTarget->GetActorLocation()) > FMath::Square(SightRangeCm)) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothEnemySight), false, this);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, GetPawnViewLocation(), AimPoint(), ECC_Visibility, Params)
		|| Hit.GetActor() == CombatTarget.Get();
}

void AEnemyBase::SetTelegraph(bool bVisible)
{
	TelegraphText->SetVisibility(bVisible);
	AimArrow->SetHiddenInGame(!bVisible);
	if (bTelegraphVisible == bVisible) return;
	bTelegraphVisible = bVisible;
	LogEvent(bVisible ? TEXT("telegraph") : TEXT("telegraph_end"));
	OnTelegraphChangedNative.Broadcast(this, bVisible);
	if (!bEndingPlay && !IsActorBeingDestroyed() && bTelegraphVisible == bVisible) OnTelegraphChanged.Broadcast(this, bVisible);
}

void AEnemyBase::StopNavigation()
{
	if (AAIController* AI = Cast<AAIController>(GetController())) { AI->StopMovement(); AI->ClearFocus(EAIFocusPriority::Gameplay); }
}

void AEnemyBase::SetGameplayEnabled(bool bEnabled)
{
	if (bGameplayEnabled == bEnabled || bDeathProcessed || bEndingPlay) return;
	++CombatGeneration;
	bGameplayEnabled = bEnabled;
	ExternalImpulseComponent->SetGameplayEnabled(bEnabled);
	StopNavigation();
	GetCharacterMovement()->StopMovementImmediately();
	CombatPhase = bEnabled ? EEnemyCombatPhase::Idle : EEnemyCombatPhase::Disabled;
	NextFireAt = GetWorld()->GetTimeSeconds() + FMath::Max(0.01f, TelegraphSeconds);
	LastSeenAt = -1.0e9;
	NextRepathAt = 0;
	bRecovering = false;
	LastProgressAt = GetWorld()->GetTimeSeconds();
	ProgressLocation = GetActorLocation();
	SetTelegraph(false);
}

void AEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsCombatEnabled()) return;
	if (!IsUsableTarget(CombatTarget.Get()))
	{
		SetCombatTarget(nullptr);
		if (bAutoAcquirePlayer) SetCombatTarget(UGameplayStatics::GetPlayerPawn(this, 0));
	}
	if (!IsCombatEnabled()) return;
	if (!IsUsableTarget(CombatTarget.Get())) { CombatPhase = EEnemyCombatPhase::Idle; StopNavigation(); SetTelegraph(false); return; }
	const double Now = GetWorld()->GetTimeSeconds();
	const bool bSight = HasTargetSight();
	if (bSight) { LastSeenAt = Now; LastSeenLocation = CombatTarget->GetActorLocation(); }
	const bool bCanAttack = bSight && FVector::DistSquared(GetActorLocation(), CombatTarget->GetActorLocation()) <= FMath::Square(AttackRangeCm);
	if (!bCanAttack)
	{
		CombatPhase = EEnemyCombatPhase::Pursuing;
		SetTelegraph(false);
		if (IsCombatEnabled()) Pursue(Now);
		return;
	}
	bRecovering = false;
	StopNavigation();
	const FVector Direction = (AimPoint() - GetPawnViewLocation()).GetSafeNormal();
	if (!Direction.IsNearlyZero()) SetActorRotation(FRotator(0, Direction.Rotation().Yaw, 0));
	AimArrow->SetWorldLocationAndRotation(GetPawnViewLocation(), Direction.Rotation());
	TelegraphText->SetWorldRotation((AimPoint() - TelegraphText->GetComponentLocation()).Rotation());
	if (CombatPhase == EEnemyCombatPhase::Telegraphing)
	{
		if (Now >= TelegraphEndsAt)
		{
			const uint64 Generation = CombatGeneration;
			CombatPhase = EEnemyCombatPhase::Cooldown;
			NextFireAt = Now + FMath::Max(FMath::Max(0.01f, ShotIntervalSeconds), TelegraphSeconds);
			SetTelegraph(false);
			if (Generation == CombatGeneration && IsCombatEnabled() && HasTargetSight()) FireProjectile();
		}
		return;
	}
	if (Now >= NextFireAt - FMath::Max(0.01f, TelegraphSeconds))
	{
		CombatPhase = EEnemyCombatPhase::Telegraphing;
		TelegraphEndsAt = Now + FMath::Max(0.01f, TelegraphSeconds);
		SetTelegraph(true);
	}
	else CombatPhase = EEnemyCombatPhase::Cooldown;
}

void AEnemyBase::Pursue(double Now)
{
	if (Now - LastSeenAt > FMath::Max(0.0f, SightMemorySeconds)) { bRecovering = false; StopNavigation(); return; }
	AAIController* AI = Cast<AAIController>(GetController());
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!AI || !Nav || Now < NextRepathAt) return;
	NextRepathAt = Now + FMath::Max(0.1f, RepathIntervalSeconds);
	if (FVector::DistSquared2D(GetActorLocation(), ProgressLocation) >= FMath::Square(StallProgressCm))
	{
		ProgressLocation = GetActorLocation(); LastProgressAt = Now;
	}
	if (bRecovering && (Now >= RecoveryExpiresAt || FVector::DistSquared2D(GetActorLocation(), RecoveryGoal) <= FMath::Square(80.0f))) bRecovering = false;
	FVector Goal = bRecovering ? RecoveryGoal : LastSeenLocation;
	if (Now - LastProgressAt >= FMath::Max(0.1f, StallTimeoutSeconds))
	{
		const FVector Forward = (Goal - GetActorLocation()).GetSafeNormal2D();
		const FVector Side(-Forward.Y, Forward.X, 0);
		const float Sign = (++RecoveryIndex % 2) ? 1.0f : -1.0f;
		Goal = GetActorLocation() + Side * Sign * RecoveryOffsetCm + Forward * RecoveryOffsetCm * 0.5f;
		// Keep a detour across ordinary repaths; progress must not immediately send
		// the pawn back into the same temporary collider it is trying to bypass.
		RecoveryGoal = Goal;
		bRecovering = true;
		RecoveryExpiresAt = Now + FMath::Max(0.5f, StallTimeoutSeconds * 2.0f);
		LastProgressAt = Now;
		++StallRecoveries;
		LogEvent(TEXT("stall_repath"));
	}
	FNavLocation Projected;
	if (!Nav->ProjectPointToNavigation(Goal, Projected, FVector(150, 150, 250))) return;
	++NavigationRequests;
	// Reissue regularly so temporary device/column obstructions cannot leave a stale path forever.
	AI->MoveToLocation(Projected.Location, 80.0f, true, true, false, true, nullptr, false);
}

void AEnemyBase::FireProjectile()
{
	if (!IsCombatEnabled() || !IsUsableTarget(CombatTarget.Get()) || !HasTargetSight()) return;
	++ShotAttempts;
	LogEvent(TEXT("shot_attempt"));
	UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!ProjectileClass || ProjectileClass->HasAnyClassFlags(CLASS_Abstract) || !Lifecycle || !Lifecycle->CanSpawnProjectile()) return;
	const float Radius = ProjectileClass.GetDefaultObject()->GetCollisionRadiusCm();
	const FVector Origin = GetPawnViewLocation();
	const FVector Direction = (AimPoint() - Origin).GetSafeNormal();
	if (Direction.ContainsNaN() || Direction.IsNearlyZero()) return;
	const FVector Muzzle = Origin + Direction * (GetCapsuleComponent()->GetScaledCapsuleRadius() + Radius + FMath::Max(0.0f, MuzzleClearanceCm));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothEnemyBirth), false, this);
	FCollisionObjectQueryParams Cover; Cover.AddObjectTypesToQuery(ECC_WorldStatic); Cover.AddObjectTypesToQuery(ECC_WorldDynamic);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	FHitResult CoverHit;
	if (GetWorld()->SweepSingleByObjectType(CoverHit, Origin, Muzzle, FQuat::Identity, Cover, Shape, Params)
		|| GetWorld()->OverlapAnyTestByObjectType(Muzzle, FQuat::Identity, Cover, Shape, Params)) { LogEvent(TEXT("muzzle_blocked")); return; }
	FHitResult BirthHit;
	const bool bBirthHit = GetWorld()->SweepSingleByChannel(BirthHit, Origin, Muzzle, FQuat::Identity, MothCollision::Projectile, Shape, Params);
	const uint64 Generation = CombatGeneration;
	const FTransform Transform(Direction.Rotation(), Muzzle);
	ARuleProjectile* Projectile = GetWorld()->SpawnActorDeferred<ARuleProjectile>(ProjectileClass, Transform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile) return;
	if (!Projectile->InitializeProjectile(Direction, this, this)) { Projectile->Destroy(); return; }
	Projectile->SetEventLoggingEnabled(bLogEnemyEvents);
	Projectile->FinishSpawning(Transform);
	if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed()) return;
	if (Generation != CombatGeneration || !IsCombatEnabled()) { Projectile->Destroy(); return; }
	++ProjectilesSpawned;
	LogEvent(TEXT("projectile_spawn"));
	if (bBirthHit) Projectile->ProcessBlockingHit(BirthHit);
}

bool AEnemyBase::ReceiveBallisticHit_Implementation(const FHitContext& Context)
{
	return IsCombatEnabled() && HealthComponent->ApplyHit(Context);
}

void AEnemyBase::ReceiveImpulse_Implementation(FVector Velocity, bool bOverrideZ, FGuid EventId)
{
	if (IsCombatEnabled()) ExternalImpulseComponent->QueueImpulse(Velocity, bOverrideZ, EventId);
}

void AEnemyBase::HandleEnemyDied(AActor* Victim, AActor* Source)
{
	if (Victim != this || bDeathProcessed || bEndingPlay) return;
	bDeathProcessed = true;
	bGameplayEnabled = false;
	++CombatGeneration;
	CombatPhase = EEnemyCombatPhase::Dead;
	StopNavigation();
	ExternalImpulseComponent->SetGameplayEnabled(false);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetTelegraph(false);
	TelegraphText->SetText(FText::FromString(TEXT("DEAD")));
	TelegraphText->SetVisibility(true);
	SetLifeSpan(FMath::Max(0.01f, DeathVisualSeconds));
	LogEvent(TEXT("death"));
	if (!bEndingPlay && !IsActorBeingDestroyed()) OnEnemyDiedNative.Broadcast(this, Source);
	if (!bEndingPlay && !IsActorBeingDestroyed()) OnEnemyDied.Broadcast(this, Source);
}

void AEnemyBase::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	bGameplayEnabled = false;
	++CombatGeneration;
	CombatPhase = bDeathProcessed ? EEnemyCombatPhase::Dead : EEnemyCombatPhase::Disabled;
	HealthComponent->OnDied.RemoveDynamic(this, &AEnemyBase::HandleEnemyDied);
	ExternalImpulseComponent->SetGameplayEnabled(false);
	StopNavigation();
	SetTelegraph(false);
	Super::EndPlay(Reason);
}

void AEnemyBase::LogEvent(const TCHAR* Event) const
{
#if !UE_BUILD_SHIPPING
	if (bLogEnemyEvents) UE_LOG(LogMothEffect, Log, TEXT("Enemy %s event=%s phase=%s target=%s sight=%d health=%.1f shots=%d spawned=%d paths=%d stalls=%d time=%.3f"),
		*GetName(), Event, *UEnum::GetValueAsString(CombatPhase), *GetNameSafe(CombatTarget.Get()), HasTargetSight(), HealthComponent->GetCurrentHealth(),
		ShotAttempts, ProjectilesSpawned, NavigationRequests, StallRecoveries, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
#endif
}
