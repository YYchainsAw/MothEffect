#include "Framework/MothWaveDirector.h"
#include "AI/EnemyBase.h"
#include "Characters/MothEffectCharacter.h"
#include "Combat/RuleProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MothEffect.h"
#include "NavigationSystem.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "UObject/ConstructorHelpers.h"

AMothWaveDirector::AMothWaveDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	SpawnPoints = { FVector(-1800,1800,0), FVector(-800,1800,0), FVector(800,1800,0), FVector(-800,-1600,0),
		FVector(1800,1800,0), FVector(2600,200,0), FVector(1800,-1800,0), FVector(200,-1800,0) };
	using K = EDeviceKind;
	Waves.SetNum(3);
	Waves[0].Spawns = { {0,K::Launcher}, {1,K::Launcher}, {2,K::Bomb}, {3,K::Emitter} };
	Waves[1].Spawns = { {1,K::Launcher}, {2,K::Bomb}, {4,K::Emitter}, {3,K::Bomb,1}, {5,K::Emitter,1}, {6,K::Emitter,1} };
	Waves[2].Spawns = { {0,K::Launcher}, {2,K::Bomb}, {4,K::Emitter}, {6,K::Emitter},
		{1,K::Launcher,1}, {3,K::Bomb,1}, {5,K::Bomb,1}, {7,K::Emitter,1} };
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Disc(TEXT("/Engine/BasicShapes/Cylinder"));
	for (int32 I = 0; I < 8; ++I)
	{
		auto* Circle = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("WarningCircle%d"), I));
		Circle->SetupAttachment(RootComponent);
		if (Disc.Succeeded()) Circle->SetStaticMesh(Disc.Object);
		Circle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Circle->SetCanEverAffectNavigation(false);
		Circle->SetVisibility(false);
		WarningCircles.Add(Circle);
		auto* Label = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("WarningLabel%d"), I));
		Label->SetupAttachment(RootComponent);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetWorldSize(45);
		Label->SetVisibility(false);
		Label->SetCanEverAffectNavigation(false);
		WarningLabels.Add(Label);
	}
}

void AMothWaveDirector::BeginPlay()
{
	Super::BeginPlay();
	// One director per map. A duplicate must not tear down the active director's world.
	for (TActorIterator<AMothWaveDirector> It(GetWorld()); It; ++It)
		if (*It != this && It->HasActorBegunPlay() && !It->IsActorBeingDestroyed())
		{ bEndingPlay = true; Destroy(); return; }
	for (auto Circle : WarningCircles)
		if (SpawnWarningMaterial) Circle->SetMaterial(0, SpawnWarningMaterial);
	GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()->SetSafetyBounds(SafetyCenter, SafetyHalfExtent);
}

bool AMothWaveDirector::SetPlayer(AMothEffectCharacter* Player)
{
	if (bEndingPlay || bChangingFlow || Phase != EMothFlowPhase::Ready || !IsValid(Player)
		|| Player->IsActorBeingDestroyed() || Player->GetWorld() != GetWorld()) return false;
	auto* Health = Player->FindComponentByClass<UHealthComponent>();
	if (!Health || !Health->IsAlive()) return false;
	UnbindPlayer();
	CombatPlayer = Player; PlayerHealth = Health;
	Health->OnDeathCommittedNative.AddUObject(this, &AMothWaveDirector::HandlePlayerDied);
	Player->OnEndPlay.AddUniqueDynamic(this, &AMothWaveDirector::HandlePlayerEndPlay);
	return true;
}

void AMothWaveDirector::UnbindPlayer()
{
	if (PlayerHealth.IsValid()) PlayerHealth->OnDeathCommittedNative.RemoveAll(this);
	if (CombatPlayer.IsValid()) CombatPlayer->OnEndPlay.RemoveDynamic(this, &AMothWaveDirector::HandlePlayerEndPlay);
	PlayerHealth.Reset();
}

bool AMothWaveDirector::StartEncounter()
{
	if (bEndingPlay || bChangingFlow || Phase != EMothFlowPhase::Ready || !CombatPlayer.IsValid()
		|| !PlayerHealth.IsValid() || !PlayerHealth->IsAlive() || EnemyClasses.Num() != 3
		|| SpawnPoints.Num() != 8 || GetWorld()->IsPaused()) return false;
	for (int32 I = 0; I < 3; ++I)
		if (!EnemyClasses[I] || EnemyClasses[I]->HasAnyClassFlags(CLASS_Abstract)
			|| EnemyClasses[I]->GetDefaultObject<AEnemyBase>()->GetDropKind() != static_cast<EDeviceKind>(I)) return false;
	if (SafetyCenter.ContainsNaN() || SafetyHalfExtent.ContainsNaN() || SafetyHalfExtent.GetMin() <= 0) return false;
	TGuardValue<bool> Guard(bChangingFlow, true);
	const uint64 Generation = ++RunGeneration;
	Phase = EMothFlowPhase::Preparing; // Commit before destruction callbacks.
	auto* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	Lifecycle->SetGameplayEnabled(false);
	CombatPlayer->SetGameplayEnabled(false);
	Lifecycle->ResetManagedObjects(); // Includes tutorial Held and active effects.
	TArray<TWeakObjectPtr<AEnemyBase>> OldEnemies;
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It) OldEnemies.Add(*It);
	for (auto E : OldEnemies) if (E.IsValid()) E->Destroy(); // Administrative removal has no drops.
	if (!IsCurrent(Generation) || !CombatPlayer.IsValid() || !PlayerHealth.IsValid() || !PlayerHealth->IsAlive()) return false;
	Lifecycle->SetSafetyBounds(SafetyCenter, SafetyHalfExtent);
	Lifecycle->SetGameplayEnabled(true);
	CombatPlayer->SetGameplayEnabled(true);
	PrepareWave(0);
	return IsCurrent(Generation);
}

int32 AMothWaveDirector::GetAliveCount() const
{
	int32 Count = 0;
	for (auto E : AliveEnemies)
		if (E.IsValid() && !E->IsActorBeingDestroyed() && E->GetHealthComponent()->IsAlive()) ++Count;
	return Count;
}

int32 AMothWaveDirector::GetPendingCount() const
{
	int32 Count = 0;
	for (const auto& Birth : PendingBirths) if (!Birth.bComplete) ++Count;
	return Count;
}

float AMothWaveDirector::GetPreparationRemaining() const
{
	return Phase == EMothFlowPhase::Preparing && GetWorld()
		? FMath::Max(0.0, PreparationEndsAt - GetWorld()->GetTimeSeconds()) : 0.0f;
}

void AMothWaveDirector::PrepareWave(int32 Index)
{
	WaveIndex = Index; Phase = EMothFlowPhase::Preparing;
	PendingBirths.Reset(); HideWarnings();
	PreparationEndsAt = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, PreparationSeconds);
	for (const auto& Spec : Waves[Index].Spawns)
	{
		FPendingBirth Birth; Birth.Spec = Spec;
		Birth.Due = PreparationEndsAt + Spec.Batch * FMath::Max(0.0f, BatchSpawnDelaySeconds);
		PendingBirths.Add(Birth);
	}
	PublishState();
}

bool AMothWaveDirector::IsOutside(const FVector& Position) const
{
	return Position.ContainsNaN() || !FBox(SafetyCenter - SafetyHalfExtent, SafetyCenter + SafetyHalfExtent).IsInsideOrOn(Position);
}

bool AMothWaveDirector::FindBirthplace(const FMothWaveSpawn& Spec, FVector& Position) const
{
	if (!CombatPlayer.IsValid() || !SpawnPoints.IsValidIndex(Spec.Point)) return false;
	const FVector Point = GetActorTransform().TransformPosition(SpawnPoints[Spec.Point]);
	const auto* Defaults = EnemyClasses[static_cast<int32>(Spec.Kind)]->GetDefaultObject<AEnemyBase>();
	const auto* Capsule = Defaults->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius(), HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	FHitResult Ground;
	FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothWaveBirth), false, this);
	if (!GetWorld()->LineTraceSingleByObjectType(Ground, Point + FVector(0,0,500), Point - FVector(0,0,1500), Objects, Params)
		|| Ground.ImpactNormal.Z < .65) return false;
	Position = Ground.ImpactPoint + FVector(0,0,HalfHeight + 2);
	if (IsOutside(Position) || FVector::Dist2D(Position, CombatPlayer->GetActorLocation()) < FMath::Max(0.0f, SpawnMinDistanceToPlayerCm)) return false;
	auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Projected;
	if (!Nav || !Nav->ProjectPointToNavigation(Ground.ImpactPoint, Projected, FVector(40,40,100))
		|| FVector::Dist2D(Projected.Location, Ground.ImpactPoint) > Radius) return false;
	// Enemy responses form a Custom profile; query the actual capsule response container.
	return !GetWorld()->OverlapBlockingTestByChannel(Position, FQuat::Identity, Capsule->GetCollisionObjectType(),
		FCollisionShape::MakeCapsule(Radius + 2, HalfHeight), Params, FCollisionResponseParams(Capsule->GetCollisionResponseToChannels()));
}

void AMothWaveDirector::ProcessBirths(double Now)
{
	const uint64 Generation = RunGeneration;
	for (int32 I = 0; I < PendingBirths.Num() && IsCurrent(Generation); ++I)
	{
		if (PendingBirths[I].bComplete || Now < PendingBirths[I].Due || Now < PendingBirths[I].RetryAt) continue;
		const FMothWaveSpawn Spec = PendingBirths[I].Spec;
		FVector Position;
		int32 WorldAlive = 0;
		for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
			if (!It->IsActorBeingDestroyed() && It->GetHealthComponent()->IsAlive()) ++WorldAlive;
		if (WorldAlive >= FMath::Clamp(MaxAliveEnemies, 1, 8) || !FindBirthplace(Spec, Position))
		{
			PendingBirths[I].WarningAt = -1;
			PendingBirths[I].RetryAt = Now + FMath::Max(.01f, SpawnRetrySeconds);
			ShowWarning(Spec.Point, GetActorTransform().TransformPosition(SpawnPoints[Spec.Point]), true, Spec.Kind);
			continue;
		}
		// Recheck throughout the entire warning. Leaving an occupied point starts a new full warning.
		if (PendingBirths[I].WarningAt < 0 || !Position.Equals(PendingBirths[I].WarnedPosition, 1.0))
		{
			PendingBirths[I].WarningAt = Now; PendingBirths[I].WarnedPosition = Position;
			ShowWarning(Spec.Point, Position - FVector(0,0,90), false, Spec.Kind);
		}
		if (Now - PendingBirths[I].WarningAt + UE_SMALL_NUMBER < FMath::Max(.01f, SpawnWarningSeconds)) continue;
		const FTransform Transform(FRotator(0, (CombatPlayer->GetActorLocation() - Position).Rotation().Yaw, 0), Position);
		auto* Enemy = GetWorld()->SpawnActorDeferred<AEnemyBase>(EnemyClasses[static_cast<int32>(Spec.Kind)], Transform,
			this, nullptr, ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
		// World spawn observers run even for deferred actors and may have ended this run.
		if (!IsCurrent(Generation)) { if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed()) Enemy->Destroy(); return; }
		if (!Enemy) { PendingBirths[I].WarningAt = -1; PendingBirths[I].RetryAt = Now + FMath::Max(.01f, SpawnRetrySeconds); continue; }
		// Reserve count and bind before construction/BeginPlay can die, destroy, or restart the encounter.
		PendingBirths[I].bComplete = true;
		AliveEnemies.Add(Enemy);
		Enemy->OnEnemyDiedNative.AddUObject(this, &AMothWaveDirector::HandleEnemyDied);
		Enemy->OnEndPlay.AddUniqueDynamic(this, &AMothWaveDirector::HandleEnemyEndPlay);
		WarningCircles[Spec.Point]->SetVisibility(false); WarningLabels[Spec.Point]->SetVisibility(false);
		Enemy->FinishSpawning(Transform);
		if (!IsCurrent(Generation)) { if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed()) Enemy->Destroy(); return; }
		if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed())
		{
			UnregisterEnemy(Enemy);
			// A failed construction/admission remains queued; an actual lethal birth is a completed spawn.
			if (!Enemy || Enemy->GetHealthComponent()->IsAlive())
			{
				PendingBirths[I].bComplete = false;
				PendingBirths[I].WarningAt = -1;
				PendingBirths[I].RetryAt = Now + FMath::Max(.01f, SpawnRetrySeconds);
			}
			continue;
		}
		Enemy->SetCombatTarget(CombatPlayer.Get());
		if (!IsCurrent(Generation)) return;
		OnEnemySpawnedNative.Broadcast(Enemy, Spec);
		if (!IsCurrent(Generation)) return;
		PublishState();
	}
}

void AMothWaveDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bEndingPlay || bChangingFlow || GetWorld()->IsPaused() || Phase == EMothFlowPhase::Restarting
		|| Phase == EMothFlowPhase::Victory || Phase == EMothFlowPhase::Defeat) return;
	if (!CombatPlayer.IsValid() && Phase == EMothFlowPhase::Ready)
		SetPlayer(Cast<AMothEffectCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)));
	if (!CombatPlayer.IsValid()) { if (IsEncounterRunning()) FinishEncounter(EMothFlowPhase::Defeat); return; }
	if (!PlayerHealth.IsValid() || !PlayerHealth->IsAlive() || IsOutside(CombatPlayer->GetActorLocation()))
	{ FinishEncounter(EMothFlowPhase::Defeat); return; }
	if (Phase == EMothFlowPhase::Ready) { if (bAutoStart) StartEncounter(); return; }
	const uint64 Generation = RunGeneration;
	const auto Snapshot = AliveEnemies.Array();
	for (auto E : Snapshot)
	{
		if (!E.IsValid()) AliveEnemies.Remove(E);
		else if (IsOutside(E->GetActorLocation())) { UnregisterEnemy(E.Get()); E->Destroy(); }
		if (!IsCurrent(Generation)) return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Phase == EMothFlowPhase::Preparing && Now >= PreparationEndsAt)
	{ Phase = EMothFlowPhase::Fighting; PublishState(); if (!IsCurrent(Generation)) return; }
	if (Phase != EMothFlowPhase::Fighting) return;
	ProcessBirths(Now);
	if (!IsCurrent(Generation)) return;
	// Resolve on tick after death callbacks/drop generation; never advance during a batch gap.
	if (GetAliveCount() == 0 && GetPendingCount() == 0)
	{
		if (WaveIndex == Waves.Num() - 1) FinishEncounter(EMothFlowPhase::Victory);
		else PrepareWave(WaveIndex + 1);
	}
}

void AMothWaveDirector::UnregisterEnemy(AEnemyBase* Enemy)
{
	if (!Enemy || !AliveEnemies.Remove(Enemy)) return;
	Enemy->OnEnemyDiedNative.RemoveAll(this);
	Enemy->OnEndPlay.RemoveDynamic(this, &AMothWaveDirector::HandleEnemyEndPlay);
}

void AMothWaveDirector::HandleEnemyDied(AEnemyBase* Enemy, AActor* Source)
{
	UnregisterEnemy(Enemy);
	if (IsEncounterRunning() && !bChangingFlow) PublishState();
}

void AMothWaveDirector::HandleEnemyEndPlay(AActor* Enemy, EEndPlayReason::Type Reason)
{
	UnregisterEnemy(Cast<AEnemyBase>(Enemy));
}

void AMothWaveDirector::HandlePlayerDied(AActor* Player, AActor* Source) { FinishEncounter(EMothFlowPhase::Defeat); }
void AMothWaveDirector::HandlePlayerEndPlay(AActor* Player, EEndPlayReason::Type Reason) { FinishEncounter(EMothFlowPhase::Defeat); }

void AMothWaveDirector::CloseAndClean(bool bResetWave)
{
	++RunGeneration;
	GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()->SetGameplayEnabled(false);
	PendingBirths.Reset(); PreparationEndsAt = 0; HideWarnings();
	const auto Snapshot = AliveEnemies.Array();
	for (auto E : Snapshot) if (E.IsValid()) UnregisterEnemy(E.Get());
	AliveEnemies.Reset(); UnbindPlayer();
	if (CombatPlayer.IsValid())
	{
		CombatPlayer->SetGameplayEnabled(false);
		CombatPlayer->GetCharacterMovement()->StopMovementImmediately();
		CombatPlayer->GetCharacterMovement()->DisableMovement();
	}
	GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()->ResetManagedObjects();
	// Also remove spent visuals/dead corpses and unregistered objects, not only capacity records.
	TArray<TWeakObjectPtr<AActor>> Objects;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		if (It->IsA<AEnemyBase>() || It->IsA<ADeviceBase>() || It->IsA<ARuleProjectile>() || It->IsA<ALaunchColumn>()) Objects.Add(*It);
	for (auto Object : Objects) if (Object.IsValid() && !Object->IsActorBeingDestroyed()) Object->Destroy();
	if (bResetWave) WaveIndex = -1;
}

void AMothWaveDirector::FinishEncounter(EMothFlowPhase Result)
{
	if (bEndingPlay || (Result != EMothFlowPhase::Victory && Result != EMothFlowPhase::Defeat)
		|| Phase == EMothFlowPhase::Victory || Phase == EMothFlowPhase::Defeat || Phase == EMothFlowPhase::Restarting) return;
	TGuardValue<bool> Guard(bChangingFlow, true);
	Phase = Result;
	CloseAndClean(false);
	PublishState();
}

bool AMothWaveDirector::RestartEncounter()
{
	if (bEndingPlay || bChangingFlow || Phase == EMothFlowPhase::Restarting) return false;
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	if (Map.IsEmpty() || !GetWorld()->GetOutermost()->HasAnyPackageFlags(PKG_ContainsMap)) return false;
	TGuardValue<bool> Guard(bChangingFlow, true);
	Phase = EMothFlowPhase::Restarting;
	CloseAndClean(true);
	UGameplayStatics::SetGamePaused(this, false);
	PublishState();
	UGameplayStatics::OpenLevel(this, FName(*Map));
	return true;
}

void AMothWaveDirector::ShowWarning(int32 Point, const FVector& Position, bool bBlocked, EDeviceKind Kind)
{
	WarningCircles[Point]->SetWorldLocation(Position + FVector(0,0,1));
	WarningCircles[Point]->SetWorldScale3D(FVector(SpawnMinDistanceToPlayerCm / 50, SpawnMinDistanceToPlayerCm / 50, .01));
	WarningCircles[Point]->SetVisibility(true);
	WarningLabels[Point]->SetWorldLocation(Position + FVector(0,0,160));
	WarningLabels[Point]->SetWorldRotation(FRotator(0, (CombatPlayer->GetActorLocation() - Position).Rotation().Yaw, 0));
	WarningLabels[Point]->SetTextRenderColor(bBlocked ? FColor::Red : FColor::Yellow);
	WarningLabels[Point]->SetText(FText::FromString(FString::Printf(TEXT("S%d / D0%d\n%s"), Point+1, static_cast<int32>(Kind)+1,
		bBlocked ? TEXT("BLOCKED - MOVE AWAY") : TEXT("INCOMING"))));
	WarningLabels[Point]->SetVisibility(true);
}

void AMothWaveDirector::HideWarnings()
{
	for (auto Circle : WarningCircles) Circle->SetVisibility(false);
	for (auto Label : WarningLabels) Label->SetVisibility(false);
}

void AMothWaveDirector::PublishState()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogMothEffect, Log, TEXT("Flow phase=%s wave=%d alive=%d pending=%d prepare=%.2f epoch=%llu"),
		*UEnum::GetValueAsString(Phase), GetWaveNumber(), GetAliveCount(), GetPendingCount(), GetPreparationRemaining(), RunGeneration);
#endif
	OnFlowChanged.Broadcast(Phase, GetWaveNumber(), GetAliveCount(), GetPendingCount());
}

void AMothWaveDirector::EndPlay(const EEndPlayReason::Type Reason)
{
	if (!bEndingPlay) { bEndingPlay = true; Phase = EMothFlowPhase::Restarting; CloseAndClean(true); }
	OnFlowChanged.Clear(); OnEnemySpawnedNative.Clear();
	Super::EndPlay(Reason);
}
