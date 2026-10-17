#if WITH_DEV_AUTOMATION_TESTS
#include "Framework/MothWaveDirector.h"
#include "Framework/MothEffectGameMode.h"
#include "AI/EnemyBase.h"
#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "TimerManager.h"
#include "Weapons/Rifle.h"
#include "Framework/MothEffectPlayerController.h"
#include "UI/MothHUDWidget.h"

namespace MothFlowRuntimeTest
{
	UWorld* GameWorld()
	{
		for (const auto& Context : GEngine->GetWorldContexts())
			if (Context.WorldType == EWorldType::Game) return Context.World();
		return nullptr;
	}
	AMothWaveDirector* Director(UWorld* World)
	{
		for (TActorIterator<AMothWaveDirector> It(World); It; ++It) if (!It->IsActorBeingDestroyed()) return *It;
		return nullptr;
	}
	FHitContext Hit(float Damage = 60) { FHitContext H; H.HitId = FGuid::NewGuid(); H.Damage = Damage; H.ShotDirection = FVector::ForwardVector; return H; }
	ADeviceBase* Device(UWorld* World, int32 Kind, FVector Position)
	{
		const TCHAR* Names[] { TEXT("Launcher"), TEXT("Bomb"), TEXT("Emitter") };
		auto* Class = LoadClass<ADeviceBase>(nullptr, *FString::Printf(TEXT("/Game/MothEffect/Devices/Drops/BP_Drop%s.BP_Drop%s_C"), Names[Kind], Names[Kind]));
		return World->GetSubsystem<UDeviceLifecycleSubsystem>()->SpawnDevice(Class, FTransform(Position));
	}
	AMothPlayerAutomationCharacter* ReplacePlayer(UWorld* World, AMothWaveDirector* D, FVector Position)
	{
		const FTransform Transform(Position);
		auto* P = World->SpawnActorDeferred<AMothPlayerAutomationCharacter>(AMothPlayerAutomationCharacter::StaticClass(), Transform,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		P->GetHealthComponent()->ConfigureMaxHealth(10000); P->FinishSpawning(Transform);
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		APawn* Old = PC ? PC->GetPawn() : nullptr;
		if (!D->SetPlayer(P)) { P->Destroy(); return nullptr; }
		// Keep the controller on its formal pawn for pause/travel. The transient fixture uses
		// native focus/button overrides, avoiding unconfigured Enhanced Input possession.
		if (auto* OldPlayer = Cast<AMothEffectCharacter>(Old))
		{
			OldPlayer->SetGameplayEnabled(false); OldPlayer->SetActorEnableCollision(false);
			OldPlayer->GetCharacterMovement()->DisableMovement();
		}
		return P;
	}
	void KillWave(UWorld* World)
	{
		TArray<TWeakObjectPtr<AEnemyBase>> Snapshot;
		for (TActorIterator<AEnemyBase> It(World); It; ++It)
			if (!It->IsActorBeingDestroyed() && It->GetHealthComponent()->IsAlive()) Snapshot.Add(*It);
		for (auto E : Snapshot) if (E.IsValid())
		{
			E->GetHealthComponent()->ApplyHit(Hit());
			// Death loot occupies its original spawn point. Remove only that loot for this
			// timing fixture; the separately placed cross-wave device must keep its idle clock.
			if (auto* Drop = E->GetDroppedDevice()) Drop->DestroyDevice();
		}
	}
	bool ValidateGame(FAutomationTestBase* T, UWorld* World)
	{
		return T->TestNotNull(TEXT("Run in an actual -game world"), World)
			&& T->TestTrue(TEXT("Actual saved flow range or main arena loaded"),
				World->GetOutermost()->GetName().Contains(TEXT("L_T16Flow"))
				|| World->GetOutermost()->GetName().Contains(TEXT("L_Arena_01")))
			&& T->TestNotNull(TEXT("Saved native director present"), Director(World));
	}

	class FThreeWaves final : public IAutomationLatentCommand
	{
	public:
		explicit FThreeWaves(FAutomationTestBase* InTest) : T(InTest) {}
		bool Update() override
		{
			if (!W) return Setup();
			if (FPlatformTime::Seconds() - StartedWall > 75) { T->AddError(TEXT("Three-wave runtime timed out")); Cleanup(); return true; }
			if (!D.IsValid() || !P.IsValid()) { T->AddError(TEXT("Flow actor disappeared")); Cleanup(); return true; }
			const double Now = W->GetTimeSeconds();
			if (D->GetPhase() == EMothFlowPhase::Defeat) { T->AddError(TEXT("Unexpected defeat during controlled three waves")); Cleanup(); return true; }
			if (Stage == 0 && D->GetAliveCount() == 3 && D->GetPendingCount() == 1)
			{
				T->TestTrue(TEXT("Player occupying S1 keeps one first-wave birth pending"), D->GetWaveNumber() == 1);
				KillWave(W); Stage = 1; WaitUntil = Now + .6;
			}
			else if (Stage == 1 && Now >= WaitUntil)
			{
				T->TestTrue(TEXT("No living enemies cannot clear an occupied pending birth"), D->GetWaveNumber() == 1 && D->GetAliveCount() == 0 && D->GetPendingCount() == 1);
				P->SetActorLocation(FVector(-2200,-1800,100)); ReleaseAt = Now; Stage = 2;
			}
			else if (Stage == 2)
			{
				if (Now - ReleaseAt < .79) T->TestEqual(TEXT("Move-away requires a full new .8s warning"), D->GetAliveCount(), 0);
				if (D->GetAliveCount() == 1)
				{
					T->TestTrue(TEXT("Occupied S1 eventually admits at its original warned position"), Now - ReleaseAt >= .79);
					Dormant = Device(W, 2, FVector(-2400,-1000,30));
					T->TestNotNull(TEXT("Cross-wave Dormant fixture"), Dormant.Get());
					if (Dormant.IsValid()) { Dormant->GetPhysicsBody()->SetSimulatePhysics(false); RemainingBefore = Dormant->GetRemainingIdleSeconds(); }
					KillWave(W); Stage = 3;
				}
			}
			else if (Stage == 3 && D->GetWaveNumber() == 2 && D->GetAliveCount() == 3)
			{
				T->TestEqual(TEXT("Second batch counts pending before first batch dies"), D->GetPendingCount(), 3);
				T->TestTrue(TEXT("Valid Dormant survives wave change without idle reset"), Dormant.IsValid() && Dormant->GetRemainingIdleSeconds() < RemainingBefore - 4);
				KillWave(W); Stage = 4; WaitUntil = Now + .6;
			}
			else if (Stage == 4)
			{
				if (Now >= WaitUntil && D->GetAliveCount() == 0)
					T->TestTrue(TEXT("Batch gap retains wave 2 and three pending births"), D->GetWaveNumber() == 2 && D->GetPendingCount() == 3);
				if (D->GetAliveCount() == 3 && D->GetPendingCount() == 0) { KillWave(W); Stage = 5; }
			}
			else if (Stage == 5 && D->GetWaveNumber() == 3 && D->GetAliveCount() == 4 && D->GetPendingCount() == 4)
			{
				Stage = 6; // Keep first batch alive to exercise the exact eight-enemy peak.
			}
			else if (Stage == 6 && D->GetAliveCount() == 8 && D->GetPendingCount() == 0)
			{
				T->TestEqual(TEXT("Formal wave never exceeds eight living enemies"), PeakAlive, 8);
				for (int32 I = 0; I < 3; ++I) T->TestEqual(*FString::Printf(TEXT("Actual wave %d births"), I+1), Counts[I], I == 0 ? 4 : I == 1 ? 6 : 8);
				const int32 ExpectedKinds[3][3] = { {2,1,1}, {1,2,3}, {2,3,3} };
				for (int32 Wave = 0; Wave < 3; ++Wave)
					for (int32 Kind = 0; Kind < 3; ++Kind)
						T->TestEqual(*FString::Printf(TEXT("Actual wave %d D0%d composition"), Wave+1, Kind+1), KindCounts[Wave][Kind], ExpectedKinds[Wave][Kind]);
				T->TestTrue(TEXT("Second batches begin at least three seconds after fighting starts"), SecondBatchAt[1] - FirstBatchAt[1] >= 2.95 && SecondBatchAt[2] - FirstBatchAt[2] >= 2.95);
				KillWave(W); Stage = 7;
			}
			else if (Stage == 7 && D->GetPhase() == EMothFlowPhase::Victory)
			{
				auto* M = W->GetSubsystem<UDeviceLifecycleSubsystem>();
				T->TestEqual(TEXT("Victory alive zero"), D->GetAliveCount(), 0);
				T->TestEqual(TEXT("Victory pending zero"), D->GetPendingCount(), 0);
				T->TestFalse(TEXT("Victory closes projectile generation"), M->CanSpawnProjectile());
				T->TestFalse(TEXT("Victory closes direct damage to living player"), P->GetHealthComponent()->ApplyHit(Hit(10)));
				T->TestNull(TEXT("Victory refuses device spawn"), Device(W, 2, FVector(0,0,100)));
				if (auto* PC = Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(W,0)))
				{
					if (auto* UI = PC->GetGameplayUI())
					{
						UI->RefreshState(); PC->SyncUIInput();
						T->TestTrue(TEXT("Three real waves end in the victory menu with cursor and blocked pawn input"),
							UI->GetHUDState().Page == EMothUIPage::Victory && PC->bShowMouseCursor && PC->IsMoveInputIgnored());
					}
					else T->AddError(TEXT("Main flow map must create its actual HUD"));
				}
				T->AddInfo(FString::Printf(TEXT("TC22 runtime: actual births=%d/%d/%d peak=%d; blocked pending and full warning; victory at %.3fs"), Counts[0], Counts[1], Counts[2], PeakAlive, Now));
				Cleanup(); return true;
			}
			return false;
		}
	private:
		bool Setup()
		{
			W = GameWorld(); if (!ValidateGame(T, W)) return true;
			D = Director(W); P = ReplacePlayer(W, D.Get(), FVector(-1800,1800,100));
			if (!T->TestNotNull(TEXT("Real controlled player"), P.Get())) return true;
			D->OnEnemySpawnedNative.AddRaw(this, &FThreeWaves::Spawned);
			StartedWall = FPlatformTime::Seconds();
			T->TestTrue(TEXT("Tutorial-ready map starts through native entry"), D->StartEncounter());
			T->TestTrue(TEXT("Formal game mode resolves this director"), Cast<AMothEffectGameMode>(W->GetAuthGameMode())->GetWaveDirector() == D.Get());
			return false;
		}
		void Spawned(AEnemyBase* E, const FMothWaveSpawn& Spec)
		{
			E->SetGameplayEnabled(false); E->GetCharacterMovement()->DisableMovement();
			const int32 Wave = D->GetWaveNumber()-1;
			// Wave 1 S1 is intentionally delayed; validate all actual identities against the definition.
			bool bConfigured = false;
			for (const auto& Entry : D->GetWaves()[Wave].Spawns)
				if (Entry.Point == Spec.Point && Entry.Kind == Spec.Kind && Entry.Batch == Spec.Batch) bConfigured = true;
			T->TestTrue(TEXT("Actual class and point match configured fixed-drop variant"), bConfigured && E->GetDropKind() == Spec.Kind);
			T->TestTrue(TEXT("Actual birth never relocates to an unannounced point"), FVector::Dist2D(E->GetActorLocation(), D->GetSpawnPoints()[Spec.Point]) < 1);
			++Counts[Wave]; PeakAlive = FMath::Max(PeakAlive, D->GetAliveCount());
			++KindCounts[Wave][static_cast<int32>(Spec.Kind)];
			if (Spec.Batch == 0 && FirstBatchAt[Wave] < 0) FirstBatchAt[Wave] = W->GetTimeSeconds();
			if (Spec.Batch == 1 && SecondBatchAt[Wave] < 0) SecondBatchAt[Wave] = W->GetTimeSeconds();
		}
		void Cleanup() { if (D.IsValid()) D->OnEnemySpawnedNative.RemoveAll(this); }
		FAutomationTestBase* T;
		UWorld* W = nullptr;
		TWeakObjectPtr<AMothWaveDirector> D;
		TWeakObjectPtr<AMothPlayerAutomationCharacter> P;
		TWeakObjectPtr<ADeviceBase> Dormant;
		int32 Stage = 0, PeakAlive = 0, Counts[3] = {0,0,0};
		int32 KindCounts[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
		double StartedWall = 0, WaitUntil = 0, ReleaseAt = 0, RemainingBefore = 0;
		double FirstBatchAt[3] = {-1,-1,-1}, SecondBatchAt[3] = {-1,-1,-1};
	};

	class FBoundsAndPause final : public IAutomationLatentCommand
	{
	public:
		explicit FBoundsAndPause(FAutomationTestBase* InTest) : T(InTest) {}
		bool Update() override
		{
			if (!W) return Setup();
			if (FPlatformTime::Seconds() - StartedWall > 25) { T->AddError(TEXT("Bounds/pause runtime timed out")); Cleanup(); return true; }
			if (!D.IsValid() || !P.IsValid()) { T->AddError(TEXT("Bounds/pause actors disappeared")); Cleanup(); return true; }
			if (Stage == 0 && FPlatformTime::Seconds() >= UnpauseWall)
			{
				T->TestEqual(TEXT("Paused game world preserves preparation deadline"), D->GetPreparationRemaining(), PausedRemaining);
				T->TestEqual(TEXT("Pause preserves four pending births"), D->GetPendingCount(), 4);
				T->TestEqual(TEXT("Pause cannot create an enemy"), D->GetAliveCount(), 0);
				UGameplayStatics::GetPlayerController(W, 0)->SetPause(false); Stage = 1;
			}
			else if (Stage == 1 && D->GetAliveCount() == 4 && D->GetPendingCount() == 0)
			{
				Escaped = Enemies[0]; Escaped->SetActorLocation(FVector(3200,0,100));
				OutsideDevice = Device(W, 2, FVector(3200,200,100));
				OutsideProjectile = W->SpawnActor<ARuleProjectile>(FVector(0,2700,400), FRotator::ZeroRotator);
				WaitUntil = W->GetTimeSeconds() + .15; Stage = 2;
			}
			else if (Stage == 2 && W->GetTimeSeconds() >= WaitUntil)
			{
				T->TestEqual(TEXT("Enemy outside unregisters exactly once"), D->GetAliveCount(), 3);
				T->TestTrue(TEXT("Bounds removal ends the actor without a death drop"), !Escaped.IsValid() || Escaped->IsActorBeingDestroyed());
				T->TestTrue(TEXT("Administrative removal never attempts a drop"), !Escaped.IsValid() || Escaped->GetDropAttempts() == 0);
				T->TestTrue(TEXT("Outside device removed"), !OutsideDevice.IsValid() || OutsideDevice->IsActorBeingDestroyed());
				T->TestTrue(TEXT("Outside projectile removed"), !OutsideProjectile.IsValid() || OutsideProjectile->IsActorBeingDestroyed());
				T->TestEqual(TEXT("Bounds removal releases device capacity"), W->GetSubsystem<UDeviceLifecycleSubsystem>()->GetDeviceCount(), 0);
				T->TestEqual(TEXT("Bounds removal releases P01 capacity"), W->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(), 0);
				KillWave(W); Stage = 3;
			}
			else if (Stage == 3 && D->GetWaveNumber() == 2)
			{
				T->TestTrue(TEXT("Escaped enemy cannot block clear-wave"), D->GetPhase() == EMothFlowPhase::Preparing && D->GetAliveCount() == 0 && D->GetPendingCount() == 6);
				P->SetActorLocation(FVector(0,2501,100)); Stage = 4;
			}
			else if (Stage == 4 && D->GetPhase() == EMothFlowPhase::Defeat)
			{
				T->TestTrue(TEXT("Player bounds produces defeat rather than waiting for health death"), P->GetHealthComponent()->IsAlive());
				T->AddInfo(TEXT("TC32 runtime: player defeat; escaped living enemy unregisters once without drop; outside D03/P01 release capacity; actual pause freezes preparation."));
				Cleanup(); return true;
			}
			return false;
		}
	private:
		bool Setup()
		{
			W = GameWorld(); if (!ValidateGame(T, W)) return true;
			D = Director(W); P = ReplacePlayer(W, D.Get(), FVector(-2200,-1800,100));
			if (!T->TestNotNull(TEXT("Bounds controlled player"), P.Get())) return true;
			D->OnEnemySpawnedNative.AddRaw(this, &FBoundsAndPause::Spawned);
			T->TestTrue(TEXT("Bounds flow starts"), D->StartEncounter());
			PausedRemaining = D->GetPreparationRemaining();
			UGameplayStatics::GetPlayerController(W, 0)->SetPause(true);
			T->TestTrue(TEXT("Actual engine world paused"), W->IsPaused());
			D->Tick(.5f); // Even an explicit caller cannot bypass the world's pause guard.
			StartedWall = FPlatformTime::Seconds(); UnpauseWall = StartedWall + .5;
			return false;
		}
		void Spawned(AEnemyBase* E, const FMothWaveSpawn&) { E->SetGameplayEnabled(false); E->GetCharacterMovement()->DisableMovement(); Enemies.Add(E); }
		void Cleanup() { if (D.IsValid()) D->OnEnemySpawnedNative.RemoveAll(this); }
		FAutomationTestBase* T; UWorld* W = nullptr;
		TWeakObjectPtr<AMothWaveDirector> D;
		TWeakObjectPtr<AMothPlayerAutomationCharacter> P;
		TArray<TWeakObjectPtr<AEnemyBase>> Enemies;
		TWeakObjectPtr<AEnemyBase> Escaped;
		TWeakObjectPtr<ADeviceBase> OutsideDevice;
		TWeakObjectPtr<ARuleProjectile> OutsideProjectile;
		int32 Stage = 0;
		double StartedWall = 0, UnpauseWall = 0, WaitUntil = 0;
		float PausedRemaining = 0;
	};

	class FReentrantBirthResult final : public IAutomationLatentCommand
	{
	public:
		explicit FReentrantBirthResult(FAutomationTestBase* InTest) : T(InTest) {}
		bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedWall > 25) { T->AddError(TEXT("Reentrant birth runtime timed out")); Cleanup(); return true; }
			auto* Current = GameWorld(); if (!Current || !Current->HasBegunPlay()) return false;
			if (Stage == 0)
			{
				if (!ValidateGame(T, Current)) return true;
				Previous = Current; Director(Current)->RestartEncounter(); Stage = 1; return false;
			}
			if (Stage == 1)
			{
				if (Current == Previous.Get()) return false;
				W = Current; D = Director(W); P = ReplacePlayer(W, D.Get(), FVector(-2200,-1800,100));
				if (!T->TestNotNull(TEXT("Reentrant controlled player"), P.Get())) return true;
				SpawnHandle = W->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateRaw(this, &FReentrantBirthResult::Spawned));
				T->TestTrue(TEXT("Reentrant flow starts"), D->StartEncounter()); Stage = 2;
			}
			else if (Stage == 2 && bCallback)
			{
				T->TestTrue(TEXT("Spawn observer actually commits player death"), bKilled);
				T->TestTrue(TEXT("Result during deferred allocation remains defeat"), D->GetPhase() == EMothFlowPhase::Defeat);
				T->TestEqual(TEXT("Result clears queue despite suspended spawn call"), D->GetPendingCount(), 0);
				T->TestEqual(TEXT("No deferred actor resurrects alive count"), D->GetAliveCount(), 0);
				int32 Alive = 0;
				for (TActorIterator<AEnemyBase> It(W); It; ++It) if (!It->IsActorBeingDestroyed()) ++Alive;
				T->TestEqual(TEXT("Unfinished birth ended before post-result BeginPlay"), Alive, 0);
				T->TestFalse(TEXT("Session still closed after returning from deferred spawn"), W->GetSubsystem<UDeviceLifecycleSubsystem>()->IsGameplayEnabled());
				T->AddInfo(TEXT("Result from actual World OnActorSpawned callback during deferred wave birth cancels the old generation before accessing its cleared queue."));
				Cleanup(); return true;
			}
			return false;
		}
	private:
		void Spawned(AActor* Actor)
		{
			if (bCallback || !Actor->IsA<AEnemyBase>() || D->GetPhase() != EMothFlowPhase::Fighting) return;
			bCallback = true; bKilled = P->GetHealthComponent()->ApplyHit(Hit(10000));
		}
		void Cleanup() { if (W && SpawnHandle.IsValid()) W->RemoveOnActorSpawnedHandler(SpawnHandle); SpawnHandle.Reset(); }
		FAutomationTestBase* T; UWorld* W = nullptr;
		TWeakObjectPtr<UWorld> Previous;
		TWeakObjectPtr<AMothWaveDirector> D;
		TWeakObjectPtr<AMothPlayerAutomationCharacter> P;
		FDelegateHandle SpawnHandle;
		int32 Stage = 0; bool bCallback = false, bKilled = false;
		double StartedWall = FPlatformTime::Seconds();
	};

	class FInterruptedWarning final : public IAutomationLatentCommand
	{
	public:
		explicit FInterruptedWarning(FAutomationTestBase* InTest) : T(InTest) {}
		bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedWall > 30) { T->AddError(TEXT("Interrupted warning runtime timed out")); Cleanup(); return true; }
			auto* Current = GameWorld(); if (!Current || !Current->HasBegunPlay()) return false;
			if (Stage == 0)
			{
				if (!ValidateGame(T, Current)) return true;
				Previous = Current; Director(Current)->RestartEncounter(); Stage = 1; return false;
			}
			if (Stage == 1)
			{
				if (Current == Previous.Get()) return false;
				W = Current; D = Director(W); P = ReplacePlayer(W, D.Get(), FVector(-2200,-1800,100));
				if (!T->TestNotNull(TEXT("Warning controlled player"), P.Get())) return true;
				D->OnEnemySpawnedNative.AddRaw(this, &FInterruptedWarning::Spawned);
				T->TestTrue(TEXT("Warning flow starts"), D->StartEncounter()); Stage = 2;
			}
			else if (Stage == 2 && D->GetPhase() == EMothFlowPhase::Fighting)
			{
				WarningStartedAt = W->GetTimeSeconds(); Stage = 3;
			}
			else if (Stage == 3 && W->GetTimeSeconds() - WarningStartedAt >= .2)
			{
				Obstacle = W->SpawnActor<AActor>(); auto* B = NewObject<UBoxComponent>(Obstacle.Get());
				Obstacle->AddInstanceComponent(B); Obstacle->SetRootComponent(B);
				B->SetBoxExtent(FVector(80,80,130)); B->SetCollisionObjectType(ECC_WorldDynamic);
				B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); B->SetCollisionResponseToAllChannels(ECR_Block);
				B->SetCanEverAffectNavigation(false); B->RegisterComponent(); B->SetWorldLocation(FVector(-800,1800,92));
				Stage = 4;
			}
			else if (Stage == 4 && D->GetAliveCount() == 3 && D->GetPendingCount() == 1)
			{
				T->TestFalse(TEXT("New physical obstacle interrupts S2 warning and preserves pending birth"), bS2Born);
				Obstacle->Destroy(); RemovedAt = W->GetTimeSeconds(); Stage = 5;
			}
			else if (Stage == 5)
			{
				if (W->GetTimeSeconds() - RemovedAt < .79) T->TestFalse(TEXT("Obstacle removal cannot reuse the old warning deadline"), bS2Born);
				if (bS2Born && D->GetAliveCount() == 4 && D->GetPendingCount() == 0)
				{
					T->AddInfo(TEXT("TC22 runtime: physical WorldDynamic capsule blocker appearing during warning retains S2 pending, removal requires a fresh .8s warning and births at original S2."));
					Cleanup(); P->SetActorLocation(FVector(3001,0,100)); return true;
				}
			}
			return false;
		}
	private:
		void Spawned(AEnemyBase* E, const FMothWaveSpawn& Spec)
		{
			E->SetGameplayEnabled(false); E->GetCharacterMovement()->DisableMovement();
			if (Spec.Point == 1)
			{
				bS2Born = true;
				T->TestTrue(TEXT("S2 only admits after obstacle removal and full new warning"), Stage == 5 && W->GetTimeSeconds() - RemovedAt >= .79);
				T->TestTrue(TEXT("No arbitrary relocation around collision"), FVector::Dist2D(E->GetActorLocation(), FVector(-800,1800,92)) < 1);
			}
		}
		void Cleanup() { if (D.IsValid()) D->OnEnemySpawnedNative.RemoveAll(this); if (Obstacle.IsValid()) Obstacle->Destroy(); }
		FAutomationTestBase* T; UWorld* W = nullptr;
		TWeakObjectPtr<UWorld> Previous;
		TWeakObjectPtr<AMothWaveDirector> D;
		TWeakObjectPtr<AMothPlayerAutomationCharacter> P;
		TWeakObjectPtr<AActor> Obstacle;
		int32 Stage = 0; bool bS2Born = false;
		double StartedWall = FPlatformTime::Seconds(), WarningStartedAt = 0, RemovedAt = 0;
	};

	class FTenRestarts final : public IAutomationLatentCommand
	{
	public:
		explicit FTenRestarts(FAutomationTestBase* InTest) : T(InTest) {}
		bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedWall > 120) { T->AddError(TEXT("Ten actual map restarts timed out")); return true; }
			UWorld* W = GameWorld();
			if (!W || !W->HasBegunPlay()) return false;
			auto* D = Director(W); auto* PC = UGameplayStatics::GetPlayerController(W, 0);
			auto* Player = PC ? Cast<AMothEffectCharacter>(PC->GetPawn()) : nullptr;
			if (!D || !Player || !Player->GetRifle()) return false;
			if (Stage == 0)
			{
				if (!ValidateGame(T, W)) return true;
				// Previous test may end in victory; reload before starting ten independent trials.
				OldWorld = W; T->TestTrue(TEXT("Initial clean-map restart accepted"), D->RestartEncounter()); Stage = 1; return false;
			}
			if (Stage == 1)
			{
				if (W == OldWorld.Get()) return false;
				T->TestTrue(TEXT("Fresh map has formal player class"), Player->GetClass()->GetPathName().Contains(TEXT("BP_MothPlayer")));
				T->TestEqual(TEXT("Reload restores default health"), Player->GetHealthComponent()->GetCurrentHealth(), 100.0f);
				T->TestEqual(TEXT("Reload restores magazine"), Player->GetRifle()->GetAmmoInMagazine(), 24);
				T->TestNull(TEXT("Reload has no held device"), Player->GetHeldDevice());
				T->TestTrue(TEXT("Reload has fresh ready director"), D->GetPhase() == EMothFlowPhase::Ready && D->GetWaveNumber() == 0 && D->GetAliveCount() == 0 && D->GetPendingCount() == 0);
				T->TestFalse(TEXT("Old actor tasks ended before fresh world"), OldPlayer.IsValid() && OldPlayer->HasActorBegunPlay());
				if (Iteration == 10) { T->AddInfo(TEXT("TC23 runtime: ten fresh-world OpenLevel restarts with Held, active D03/column, live P01 and rifle tasks; fresh health/ammo/counters verified each time.")); return true; }
				P = ReplacePlayer(W, D, FVector(-2200,-1800,100));
				if (!T->TestNotNull(TEXT("Restart controlled player"), P.Get())) return true;
				T->TestTrue(TEXT("Restart trial starts clean encounter"), D->StartEncounter());
				// Deplete one round with real W01 and begin a real pending reload.
				T->AddExpectedError(FString::Printf(TEXT("could not play on %s. Assign a Montage compatible"), *P->GetName()), EAutomationExpectedErrorFlags::Contains, 1);
				P->SetPrimaryDown(false); P->DoPrimaryEnd(); // Release the transition input gate first.
				P->SetPrimaryDown(true); P->DoPrimaryStart(); P->SetPrimaryDown(false); P->DoPrimaryEnd();
				P->GetRifle()->TryBeginReload();
				T->TestEqual(TEXT("Real rifle depleted before reload"), P->GetRifle()->GetAmmoInMagazine(), 23);
				T->TestTrue(TEXT("Actual pending reload before death/restart"), P->GetRifle()->IsReloading());
				auto* Held = Device(W, 2, P->GetActorLocation()+FVector(140,0,0));
				// Reload owns the action: cancel it briefly for pickup, then arm it after held cleanup on result.
				P->GetRifle()->CancelReload();
				T->TestTrue(TEXT("Actual held device before result"), P->TryPickupDevice(Held));
				RestartEmitter = Device(W, 2, FVector(0,0,100));
				RestartColumn = Device(W, 0, FVector(1000,0,30));
				WaitUntil = W->GetTimeSeconds()+FMath::Max(RestartEmitter->SpawnActivationProtectionSeconds, RestartColumn->SpawnActivationProtectionSeconds)+.01;
				Stage = 10; return false;
			}
			if (Stage == 10 && W->GetTimeSeconds() >= WaitUntil)
			{
				auto* Emitter = RestartEmitter.Get(); auto* Column = RestartColumn.Get();
				Emitter->TryActivate(Hit()); Column->TryActivate(Hit());
				T->TestTrue(TEXT("Saved cleanup hazards retain their default pending delay"), Emitter->IsActivationPending() && Column->IsActivationPending());
				WaitUntil = W->GetTimeSeconds()+FMath::Max(Emitter->ActivationDelaySeconds, Column->ActivationDelaySeconds)+.05;
				Stage = 11; return false;
			}
			if (Stage == 11 && W->GetTimeSeconds() >= WaitUntil)
			{
				T->TestTrue(TEXT("Actual delayed active column exists before result"), RestartColumn.IsValid() && RestartColumn->GetLaunchColumn()!=nullptr);
				W->SpawnActor<ARuleProjectile>(FVector(2000,0,400), FRotator::ZeroRotator);
				T->TestTrue(TEXT("Actual live projectile before result"), W->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount() > 0);
				T->TestTrue(TEXT("Lethal hit commits result"), P->GetHealthComponent()->ApplyHit(Hit(10000)));
				T->TestTrue(TEXT("Defeat closes session"), D->GetPhase() == EMothFlowPhase::Defeat && !W->GetSubsystem<UDeviceLifecycleSubsystem>()->IsGameplayEnabled());
				T->TestNull(TEXT("Defeat clears Held"), P->GetHeldDevice());
				T->TestFalse(TEXT("Defeat cancels rifle reload"), P->GetRifle()->IsReloading());
				T->TestEqual(TEXT("Defeat releases devices"), W->GetSubsystem<UDeviceLifecycleSubsystem>()->GetDeviceCount(), 0);
				T->TestEqual(TEXT("Defeat releases projectiles"), W->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(), 0);
				WaitUntil = W->GetTimeSeconds() + 3.1; OldWorld = W; OldPlayer = P; CurrentDirector = D; Stage = 2;
			}
			else if (Stage == 2 && W->GetTimeSeconds() >= WaitUntil)
			{
				T->TestEqual(TEXT("Old emitter/reload tasks do not generate while result is shown"), W->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(), 0);
				// Also restart from a legitimately paused result screen.
				if (Iteration % 2 == 0) { PC->SetPause(true); T->TestTrue(TEXT("Actual game pause before restart"), W->IsPaused()); }
				T->TestTrue(TEXT("Current-map travel accepted"), CurrentDirector->RestartEncounter());
				++Iteration; Stage = 1;
			}
			return false;
		}
	private:
		FAutomationTestBase* T;
		double StartedWall = FPlatformTime::Seconds(), WaitUntil = 0;
		int32 Stage = 0, Iteration = 0;
		TWeakObjectPtr<UWorld> OldWorld;
		TWeakObjectPtr<AMothEffectCharacter> OldPlayer;
		TWeakObjectPtr<AMothPlayerAutomationCharacter> P;
		TWeakObjectPtr<AMothWaveDirector> CurrentDirector;
		TWeakObjectPtr<ADeviceBase> RestartColumn;
		TWeakObjectPtr<ADeviceBase> RestartEmitter;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothThreeWavesRuntime, "MothEffect.WaveFlowRuntime.ThreeWavesBatchGapBlockedBirthVictory", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothThreeWavesRuntime::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothFlowRuntimeTest::FThreeWaves(this)); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothTenRestartsRuntime, "MothEffect.WaveFlowRuntime.TenActualMapRestartsWithHazards", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothTenRestartsRuntime::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothFlowRuntimeTest::FTenRestarts(this)); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothBoundsAndPauseRuntime, "MothEffect.WaveFlowRuntime.EnemyPlayerObjectBoundsAndPause", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothBoundsAndPauseRuntime::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothFlowRuntimeTest::FBoundsAndPause(this)); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothInterruptedWarningRuntime, "MothEffect.WaveFlowRuntime.SpawnWarningInterruptedByPhysicalBlocker", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothInterruptedWarningRuntime::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothFlowRuntimeTest::FInterruptedWarning(this)); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothReentrantBirthRuntime, "MothEffect.WaveFlowRuntime.ResultDuringDeferredWorldSpawnCallback", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothReentrantBirthRuntime::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothFlowRuntimeTest::FReentrantBirthResult(this)); return true; }
#endif
