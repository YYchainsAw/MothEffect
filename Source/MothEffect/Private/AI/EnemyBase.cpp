#include "AI/EnemyBase.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Animation/AnimInstance.h"
#include "Combat/RuleProjectile.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ExternalImpulseComponent.h"
#include "Components/HealthComponent.h"
#include "Feedback/MothCharacterVoiceComponent.h"
#include "Feedback/MothAudioSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "Devices/DeviceBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/LatentActionManager.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Framework/MothEffectPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MothEffect.h"
#include "NavigationSystem.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Subsystems/EnemyPoolSubsystem.h"
#include "Subsystems/ProjectilePoolSubsystem.h"
#include "Types/MothCollisionChannels.h"
#include "UI/MothEnemyHealthWidget.h"
#include "UI/MothHUDWidget.h"
#include "UObject/ConstructorHelpers.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	CharacterVoice = CreateDefaultSubobject<UMothCharacterVoiceComponent>(TEXT("CharacterVoice"));
	CharacterVoice->ConfigureDrongoDefaults();
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	BaseEyeHeight = 60.0f;
	GetCapsuleComponent()->InitCapsuleSize(MothCollision::EnemyCapsuleRadiusCm, MothCollision::EnemyCapsuleHalfHeightCm);
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
	BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackpackMesh"));
	BackpackMesh->SetupAttachment(GetCapsuleComponent());
	BackpackMesh->SetRelativeLocation(FVector(-45, 0, 15));
	BackpackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BackpackMesh->SetCanEverAffectNavigation(false);
	BackpackMesh->SetVisibility(false);
	DropHintText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DropHintText"));
	DropHintText->SetupAttachment(GetCapsuleComponent());
	DropHintText->SetRelativeLocation(FVector(0, 0, 90));
	DropHintText->SetAbsolute(false, true, false);
	DropHintText->SetHorizontalAlignment(EHTA_Center);
	DropHintText->SetWorldSize(22);
	DropHintText->SetVisibility(false);
	DropHintText->SetCanEverAffectNavigation(false);
	// Retain legacy components for existing BP references; Designer widgets own UI.
	BodyMesh->SetHiddenInGame(true);
	TelegraphText->SetHiddenInGame(true);
	DropHintText->SetHiddenInGame(true);
	EnemyHealthWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyHealthWidget"));
	EnemyHealthWidget->SetupAttachment(GetCapsuleComponent());
	EnemyHealthWidget->SetRelativeLocation(FVector(0, 0, 145));
	EnemyHealthWidget->SetWidgetSpace(EWidgetSpace::Screen);
	EnemyHealthWidget->SetDrawSize(FVector2D(160, 30));
	EnemyHealthWidget->SetPivot(FVector2D(0.5f, 1.0f));
	EnemyHealthWidget->SetDrawAtDesiredSize(false);
	EnemyHealthWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnemyHealthWidget->SetCanEverAffectNavigation(false);
	EnemyHealthWidget->SetHiddenInGame(true);
}

void AEnemyBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	HealthComponent->ConfigureMaxHealth(MaxHealth);
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(1.0f, WalkSpeedCmPerSec);
	if (bPoolInactive) { SetActorHiddenInGame(true); SetActorEnableCollision(false); }
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	const auto* Session = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (Session && !Session->IsGameplayEnabled()) { Destroy(); return; }
	HealthComponent->OnDied.AddDynamic(this, &AEnemyBase::HandleEnemyDied);
	PoolAnimationMode = static_cast<uint8>(GetMesh()->GetAnimationMode());
	PoolAnimationClass = GetMesh()->GetAnimClass();
	PoolMeshRelativeTransform = GetMesh()->GetRelativeTransform();
	bPoolMeshHidden = GetMesh()->bHiddenInGame;
	bPoolAnimationPaused = GetMesh()->bPauseAnims;
	bPoolAnimationInitialized = true;
	ProgressLocation = GetActorLocation();
	LastProgressAt = GetWorld()->GetTimeSeconds();
	NextFireAt = LastProgressAt + FMath::Max(0.01f, TelegraphSeconds);
	RefreshDropAppearance();
	// Old BP defaults can override constructor visibility, so enforce this per instance.
	BodyMesh->SetHiddenInGame(true);
	TelegraphText->SetHiddenInGame(true);
	DropHintText->SetHiddenInGame(true);
	InitializeEnemyHealthWidget();
	UpdateEnemyHealthWidgetVisibility();
	LogEvent(TEXT("begin"));
	if (bPoolInactive) SuspendPoolTicks();
}

void AEnemyBase::InitializeEnemyHealthWidget()
{
	if (!EnemyHealthWidget || !GetWorld() || !GetWorld()->IsGameWorld() || GetNetMode() == NM_DedicatedServer) return;
	if (HealthWidgetClass && EnemyHealthWidget->GetWidgetClass() != HealthWidgetClass)
		EnemyHealthWidget->SetWidgetClass(HealthWidgetClass);
	EnemyHealthWidget->InitWidget();
	if (auto* Widget = Cast<UMothEnemyHealthWidget>(EnemyHealthWidget->GetUserWidgetObject()))
		Widget->SetObservedEnemy(this);
}

void AEnemyBase::UpdateEnemyHealthWidgetVisibility()
{
	if (!EnemyHealthWidget || !GetWorld()) return;
	// Pause freezes the last presentation; it does not tear down and rebuild bindings.
	if (GetWorld()->IsPaused()) return;
	const auto Hide = [this]() { EnemyHealthWidget->SetHiddenInGame(true); };
	const auto* Session = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (bEndingPlay || bDeathProcessed || !bGameplayEnabled || !HealthComponent->IsAlive()
		|| (Session && !Session->IsGameplayEnabled())) { Hide(); return; }
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->GetLocalPlayer()) { Hide(); return; }
	if (const auto* MothController = Cast<AMothEffectPlayerController>(PlayerController))
	{
		const UMothHUDWidget* HUD = MothController->GetGameplayUI();
		if (!HUD) { Hide(); return; }
		const EMothUIPage Page = HUD->GetHUDState().Page;
		if (Page == EMothUIPage::Pause) return;
		if (Page != EMothUIPage::Gameplay && Page != EMothUIPage::Tutorial) { Hide(); return; }
	}
	EnemyHealthWidget->SetOwnerPlayer(PlayerController->GetLocalPlayer());
	if (!EnemyHealthWidget->GetUserWidgetObject()) InitializeEnemyHealthWidget();
	auto* Widget = Cast<UMothEnemyHealthWidget>(EnemyHealthWidget->GetUserWidgetObject());
	if (!Widget) { Hide(); return; }
	if (Widget->GetObservedEnemy() != this) Widget->SetObservedEnemy(this);
	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector Location = EnemyHealthWidget->GetComponentLocation();
	if (HealthWidgetMaxDistanceCm <= 0 || FVector::DistSquared(CameraLocation, Location) > FMath::Square(HealthWidgetMaxDistanceCm))
	{ Hide(); return; }
	FVector2D Screen;
	int32 Width = 0, Height = 0;
	PlayerController->GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0 || !PlayerController->ProjectWorldLocationToScreen(Location, Screen, true)
		|| Screen.X < 80 || Screen.X > Width - 80 || Screen.Y < 30 || Screen.Y > Height)
	{ Hide(); return; }
	// Screen-space widgets otherwise reveal enemies through every wall.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothEnemyHealthSight), true, this);
	if (PlayerController->GetPawn()) Params.AddIgnoredActor(PlayerController->GetPawn());
	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, GetPawnViewLocation(), ECC_Visibility, Params);
	EnemyHealthWidget->SetHiddenInGame(bBlocked);
}

void AEnemyBase::ReleaseEnemyHealthWidget()
{
	if (!EnemyHealthWidget) return;
	EnemyHealthWidget->SetHiddenInGame(true);
	if (auto* Widget = Cast<UMothEnemyHealthWidget>(EnemyHealthWidget->GetUserWidgetObject()))
		Widget->ClearObservedEnemy();
	EnemyHealthWidget->SetWidget(nullptr);
}

bool AEnemyBase::IsCombatEnabled() const
{
	const auto* Session = GetWorld() ? GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>() : nullptr;
	return !bPoolInactive && bGameplayEnabled && !bDeathProcessed && !bEndingPlay && !IsActorBeingDestroyed()
		&& HasActorBegunPlay() && HealthComponent->IsAlive() && GetWorld() && !GetWorld()->IsPaused()
		&& (!Session || Session->IsGameplayEnabled());
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
	if (bPoolInactive) return false;
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
	TelegraphText->SetHiddenInGame(true);
	AimArrow->SetHiddenInGame(true);
	if (bTelegraphVisible == bVisible) return;
	bTelegraphVisible = bVisible;
	const uint64 LifeGeneration = PoolLifeGeneration;
	TGuardValue<int32> DispatchGuard(PoolCallbackDepth, PoolCallbackDepth + 1);
	LogEvent(bVisible ? TEXT("telegraph") : TEXT("telegraph_end"));
	OnTelegraphChangedNative.Broadcast(this, bVisible);
	if (!bEndingPlay && !IsActorBeingDestroyed() && LifeGeneration == PoolLifeGeneration && bTelegraphVisible == bVisible)
		OnTelegraphChanged.Broadcast(this, bVisible);
}

void AEnemyBase::StopNavigation()
{
	if (AAIController* AI = Cast<AAIController>(GetController())) { AI->StopMovement(); AI->ClearFocus(EAIFocusPriority::Gameplay); }
}

void AEnemyBase::SetGameplayEnabled(bool bEnabled)
{
	if (bPoolInactive || bGameplayEnabled == bEnabled || bDeathProcessed || bEndingPlay) return;
	++CombatGeneration;
	bGameplayEnabled = bEnabled;
	if (!bEnabled)
		if (auto* Audio = GetWorld() ? GetWorld()->GetSubsystem<UMothAudioSubsystem>() : nullptr) Audio->StopSpeaker(this);
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
	UpdateEnemyHealthWidgetVisibility();
}

void AEnemyBase::Tick(float DeltaSeconds)
{
	const uint64 LifeGeneration = PoolLifeGeneration;
	TGuardValue<int32> DispatchGuard(PoolCallbackDepth, PoolCallbackDepth + 1);
	Super::Tick(DeltaSeconds);
	if (bPoolInactive || LifeGeneration != PoolLifeGeneration) return;
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= NextHealthWidgetVisibilityAt)
	{
		NextHealthWidgetVisibilityAt = GetWorld()->GetTimeSeconds() + 0.1;
		UpdateEnemyHealthWidgetVisibility();
	}
	if (!IsCombatEnabled()) return;
	if (DropDeviceClass && CombatTarget.IsValid()) DropHintText->SetWorldRotation((AimPoint() - DropHintText->GetComponentLocation()).Rotation());
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
	auto* Pool = GetWorld()->GetSubsystem<UProjectilePoolSubsystem>();
	ARuleProjectile* Projectile = Pool ? Pool->SpawnProjectile(ProjectileClass, Transform, Direction, this, this, bLogEnemyEvents) : nullptr;
	if (!Projectile) return;
	if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed() || Projectile->IsPoolInactive()) return;
	if (Generation != CombatGeneration || !IsCombatEnabled()) { Projectile->ReturnToPool(); return; }
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
	if (Victim != this || bPoolInactive || bDeathProcessed || bEndingPlay) return;
	const uint64 LifeGeneration = PoolLifeGeneration;
	TGuardValue<int32> DispatchGuard(PoolCallbackDepth, PoolCallbackDepth + 1);
	bDeathProcessed = true;
	// Snapshot before any presentation/death observer can reset the world or destroy us.
	const auto FixedDropClass = DropDeviceClass;
	const EDeviceKind FixedDropKind = DropKind;
	const FVector DeathLocation = GetActorLocation();
	const UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	const uint64 DropGeneration = Lifecycle ? Lifecycle->GetLifecycleGeneration() : 0;
	bDropAttempted = FixedDropClass != nullptr;
	const auto CanContinueDeath = [&]()
	{
		// A recycled/new life owns its own result. Ordinary destruction still records
		// the committed old life's cancelled drop, as it did before pooling.
		if (bPoolInactive || LifeGeneration != PoolLifeGeneration) return false;
		if (!bEndingPlay && !IsActorBeingDestroyed()) return true;
		if (FixedDropClass) DropResult = EEnemyDropResult::Cancelled;
		return false;
	};
	ReleaseEnemyHealthWidget();
	if (!CanContinueDeath()) return;
	bGameplayEnabled = false;
	++CombatGeneration;
	CombatPhase = EEnemyCombatPhase::Dead;
	StopNavigation();
	ExternalImpulseComponent->SetGameplayEnabled(false);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetTelegraph(false);
	if (!CanContinueDeath()) return;
	TelegraphText->SetText(FText::FromString(TEXT("DEAD")));
	TelegraphText->SetHiddenInGame(true);
	SetLifeSpan(FMath::Max(0.01f, DeathVisualSeconds));
	LogEvent(TEXT("death"));
	if (!bEndingPlay && !IsActorBeingDestroyed()) OnEnemyDiedNative.Broadcast(this, Source);
	if (!CanContinueDeath()) return;
	OnEnemyDied.Broadcast(this, Source);
	if (!CanContinueDeath()) return;
	TryDropDevice(FixedDropClass, FixedDropKind, DeathLocation, DropGeneration, LifeGeneration);
}

void AEnemyBase::ReturnToPool()
{
	if (bPoolManaged)
	{
		if (auto* Pool = GetWorld() ? GetWorld()->GetSubsystem<UEnemyPoolSubsystem>() : nullptr)
		{
			Pool->RecycleEnemy(this);
			return;
		}
	}
	Destroy();
}

void AEnemyBase::LifeSpanExpired()
{
	if (bPoolManaged) ReturnToPool();
	else Super::LifeSpanExpired();
}

void AEnemyBase::SuspendPoolTicks()
{
	if (!bPoolTicksSuspended)
	{
		// Deferred actors start with disabled tick functions until registration/BeginPlay.
		// Preserve their authored startup setting rather than that temporary disabled state.
		bPoolActorTickEnabled = HasActorBegunPlay() ? IsActorTickEnabled()
			: PrimaryActorTick.bCanEverTick && PrimaryActorTick.bStartWithTickEnabled;
		bPoolTicksSuspended = true;
	}
	const auto SuspendComponent = [this](UActorComponent* Component)
	{
		if (!PoolComponentTicks.Contains(Component))
		{
			const bool bEnabled = Component->HasBegunPlay() ? Component->IsComponentTickEnabled()
				: Component->PrimaryComponentTick.bCanEverTick && Component->PrimaryComponentTick.bStartWithTickEnabled;
			PoolComponentTicks.Add(Component, bEnabled);
		}
		Component->SetComponentTickEnabled(false);
	};
	SetActorTickEnabled(false);
	TInlineComponentArray<UActorComponent*> Components(this);
	for (UActorComponent* Component : Components) SuspendComponent(Component);
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (PoolController.Get() != AI)
		{
			PoolController = AI;
			bPoolControllerTickEnabled = AI->HasActorBegunPlay() ? AI->IsActorTickEnabled()
				: AI->PrimaryActorTick.bCanEverTick && AI->PrimaryActorTick.bStartWithTickEnabled;
		}
		AI->SetActorTickEnabled(false);
		if (UBrainComponent* Brain = AI->GetBrainComponent()) Brain->StopLogic(TEXT("Enemy returned to pool"));
		TInlineComponentArray<UActorComponent*> ControllerComponents(AI);
		for (UActorComponent* Component : ControllerComponents) SuspendComponent(Component);
	}
}

void AEnemyBase::RestorePoolTicks()
{
	for (const auto& Entry : PoolComponentTicks)
		if (UActorComponent* Component = Entry.Key.Get()) Component->SetComponentTickEnabled(Entry.Value);
	PoolComponentTicks.Reset();
	SetActorTickEnabled(bPoolActorTickEnabled);
	bPoolTicksSuspended = false;
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->SetActorTickEnabled(bPoolControllerTickEnabled);
		if (UBrainComponent* Brain = AI->GetBrainComponent()) Brain->RestartLogic();
	}
	PoolController.Reset();
}

void AEnemyBase::PrepareForPoolAcquisition()
{
	++PoolLifeGeneration;
	++CombatGeneration;
	bPoolInactive = true;
	bGameplayEnabled = false;
	HealthComponent->SetPoolInactive(true);
	SetLifeSpan(0.0f);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	ExternalImpulseComponent->SetGameplayEnabled(false);
	StopNavigation();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->ClearAccumulatedForces();
	SetBase(static_cast<FMovementBaseInterfaceData*>(nullptr));
	SuspendPoolTicks();
}

void AEnemyBase::DeactivateForPool()
{
	PrepareForPoolAcquisition();
	// Old shots can outlive the corpse and must not treat a later checkout as their source.
	if (GetWorld())
		for (TActorIterator<ARuleProjectile> It(GetWorld()); It; ++It)
			if (!It->IsActorBeingDestroyed() && !It->IsPoolInactive()) It->ForgetSourceActor(this);
	CombatTarget.Reset();
	CombatPhase = EEnemyCombatPhase::Disabled;
	ReleaseEnemyHealthWidget();
	GetCharacterMovement()->DisableMovement();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
		GetWorld()->GetLatentActionManager().RemoveActionsForObject(this);
	}
	if (auto* Audio = GetWorld() ? GetWorld()->GetSubsystem<UMothAudioSubsystem>() : nullptr) Audio->StopSpeaker(this);
	if (UAnimInstance* Animation = GetMesh()->GetAnimInstance()) Animation->StopAllMontages(0.0f);
	SetTelegraph(false);
}

void AEnemyBase::ActivateFromPool()
{
	if (!bPoolManaged || !bPoolInactive || bEndingPlay || IsActorBeingDestroyed()) return;
	const uint64 LifeGeneration = PoolLifeGeneration;
	TGuardValue<int32> DispatchGuard(PoolCallbackDepth, PoolCallbackDepth + 1);
	HealthComponent->ResetForPoolReuse(MaxHealth);
	ExternalImpulseComponent->ResetForPoolReuse();
	CharacterVoice->ResetForPoolReuse();
	bDeathProcessed = false;
	bDropAttempted = false;
	DroppedDevice.Reset();
	DropResult = DropDeviceClass ? EEnemyDropResult::Ready : EEnemyDropResult::Unconfigured;
	CombatTarget.Reset();
	CombatPhase = EEnemyCombatPhase::Idle;
	LastSeenLocation = RecoveryGoal = FVector::ZeroVector;
	ProgressLocation = GetActorLocation();
	LastSeenAt = -1.0e9;
	LastProgressAt = GetWorld()->GetTimeSeconds();
	NextFireAt = LastProgressAt + FMath::Max(0.01f, TelegraphSeconds);
	TelegraphEndsAt = NextRepathAt = RecoveryExpiresAt = NextHealthWidgetVisibilityAt = 0;
	ShotAttempts = ProjectilesSpawned = NavigationRequests = StallRecoveries = RecoveryIndex = 0;
	bTelegraphVisible = bRecovering = false;
	SetLifeSpan(0.0f);
	StopNavigation();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->ClearAccumulatedForces();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(1.0f, WalkSpeedCmPerSec);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (bPoolAnimationInitialized)
	{
		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		GetMesh()->SetRelativeTransform(PoolMeshRelativeTransform);
		GetMesh()->SetAnimationMode(static_cast<EAnimationMode::Type>(PoolAnimationMode), true);
		if (PoolAnimationClass) GetMesh()->SetAnimInstanceClass(PoolAnimationClass);
		if (UAnimInstance* Animation = GetMesh()->GetAnimInstance()) Animation->StopAllMontages(0.0f);
		GetMesh()->bPauseAnims = bPoolAnimationPaused;
		GetMesh()->SetHiddenInGame(bPoolMeshHidden);
	}
	RefreshDropAppearance();
	BodyMesh->SetHiddenInGame(true);
	TelegraphText->SetHiddenInGame(true);
	TelegraphText->SetText(FText::FromString(TEXT("FIRING")));
	DropHintText->SetHiddenInGame(true);
	AimArrow->SetHiddenInGame(true);
	InitializeEnemyHealthWidget();
	// Widget/animation initialization can invoke game code that recycles this checkout.
	if (bEndingPlay || IsActorBeingDestroyed() || LifeGeneration != PoolLifeGeneration) return;
	RestorePoolTicks();
	if (bEndingPlay || IsActorBeingDestroyed() || LifeGeneration != PoolLifeGeneration) return;
	const auto* Session = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!Session || !Session->IsGenerationCurrent(PoolCheckoutLifecycleGeneration) || GetWorld()->IsPaused()) return;
	// Animation/widget/AI callbacks above may have added a blocker after FinishSpawning's probe.
	if (const auto* Pool = GetWorld()->GetSubsystem<UEnemyPoolSubsystem>(); !Pool || !Pool->IsBirthplaceClear(this))
	{
		ReturnToPool();
		return;
	}
	bPoolInactive = false;
	bGameplayEnabled = true;
	HealthComponent->SetPoolInactive(false);
	ExternalImpulseComponent->SetGameplayEnabled(true);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	UpdateEnemyHealthWidgetVisibility();
}

void AEnemyBase::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	ReleaseEnemyHealthWidget();
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
