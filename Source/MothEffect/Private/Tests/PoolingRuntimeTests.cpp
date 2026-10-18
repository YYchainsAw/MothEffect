#if WITH_DEV_AUTOMATION_TESTS

#include "AI/EnemyBase.h"
#include "Art/MothArenaVisualDirector.h"
#include "Animation/AnimInstance.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/HealthComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Devices/DeviceBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/MothWaveDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/EnemyPoolSubsystem.h"

namespace MothPoolingRuntimeTest
{
	UWorld* GameWorld()
	{
		if (GEngine)
			for (const auto& Context : GEngine->GetWorldContexts())
				if (Context.WorldType == EWorldType::Game) return Context.World();
		return nullptr;
	}

	class FHighDensityEncounter final : public IAutomationLatentCommand
	{
	public:
		explicit FHighDensityEncounter(FAutomationTestBase* InTest) : Test(InTest) {}

		bool Update() override
		{
			if (FPlatformTime::Seconds() - StartedWall > 120)
			{
				Test->AddError(FString::Printf(TEXT("High-density encounter timed out: wave=%d phase=%s births=%d/%d/%d alive=%d pending=%d kills=%d"),
					Director.IsValid() ? Director->GetWaveNumber() : -1,
					Director.IsValid() ? *UEnum::GetValueAsString(Director->GetPhase()) : TEXT("missing"),
					Births[0], Births[1], Births[2], Director.IsValid() ? Director->GetAliveCount() : -1,
					Director.IsValid() ? Director->GetPendingCount() : -1, AcceptedKills));
				Cleanup();
				return true;
			}
			if (!bSetup) return Setup();
			if (bFailed) { Cleanup(); return true; }
			if (!World.IsValid() || GameWorld() != World.Get() || !Director.IsValid() || !Player.IsValid())
			{
				Test->AddError(TEXT("High-density encounter lost its original world, director or actual player"));
				Cleanup();
				return true;
			}
			if (Director->GetPhase() == EMothFlowPhase::Defeat)
			{
				Test->AddError(TEXT("High-density encounter unexpectedly ended in defeat"));
				Cleanup();
				return true;
			}
			PeakAlive = FMath::Max(PeakAlive, Director->GetAliveCount());
			if (Director->GetPhase() == EMothFlowPhase::Victory)
			{
				ValidateVictory();
				Cleanup();
				return true;
			}
			const int32 Wave = Director->GetWaveNumber() - 1;
			if (Wave >= 0 && Wave < 3 && !bKilledWave[Wave]
				&& Director->GetPhase() == EMothFlowPhase::Fighting && Director->GetPendingCount() == 0)
			{
				if (Births[Wave] != ExpectedBirths[Wave] || Director->GetAliveCount() != ExpectedBirths[Wave])
				{
					Test->AddError(FString::Printf(TEXT("Wave %d completed admission with births=%d alive=%d, expected %d"),
						Wave + 1, Births[Wave], Director->GetAliveCount(), ExpectedBirths[Wave]));
					Cleanup();
					return true;
				}
				bKilledWave[Wave] = true;
				KillWave(Wave);
			}
			return false;
		}

	private:
		bool Setup()
		{
			UWorld* Current = GameWorld();
			if (!Current || !Current->HasBegunPlay()) return false;
			if (!Test->TestTrue(TEXT("High-density runtime runs on the actual saved SoulCity arena"),
				Current->GetOutermost()->GetName().Contains(TEXT("L_Arena_SoulCity")))) return true;
			World = Current;
			int32 VisualDirectorCount = 0;
			AMothArenaVisualDirector* VisualDirector = nullptr;
			for (TActorIterator<AMothArenaVisualDirector> It(Current); It; ++It)
				if (!It->IsActorBeingDestroyed()) { ++VisualDirectorCount; VisualDirector = *It; }
			if (!Test->TestEqual(TEXT("Saved arena receives exactly one runtime visual director"), VisualDirectorCount, 1)
				|| !Test->TestTrue(TEXT("Runtime arena look is applied with enabled unbound post process"),
					VisualDirector && VisualDirector->WasLookApplied()
					&& VisualDirector->GetPostProcessComponent()->bEnabled
					&& VisualDirector->GetPostProcessComponent()->bUnbound)) return true;
			const IConsoleVariable* AAMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"));
			if (!Test->TestTrue(TEXT("Saved arena uses the configured TSR method"), AAMethod && AAMethod->GetInt() == 4)) return true;
			for (TActorIterator<AMothWaveDirector> It(Current); It; ++It)
				if (!It->IsActorBeingDestroyed()) { Director = *It; break; }
			if (!Test->TestNotNull(TEXT("Saved arena wave director"), Director.Get())) return true;
			// The automation worker can attach just after preparation, during the first warning.
			// Subscribe before the first actual birth; the fixture never starts or
			// changes the encounter, wave definitions, NavMesh or director safety checks.
			SpawnHandle = Director->OnEnemySpawnedNative.AddRaw(this, &FHighDensityEncounter::Spawned);
			if (!Test->TestTrue(TEXT("Runtime attaches before any first-wave births"),
				Director->IsEncounterRunning() && Director->GetWaveNumber() == 1
				&& Director->GetAliveCount() == 0 && Director->GetPendingCount() == ExpectedBirths[0]))
			{
				Cleanup();
				return true;
			}
			Player = Cast<AMothEffectCharacter>(UGameplayStatics::GetPlayerPawn(Current, 0));
			if (!Test->TestNotNull(TEXT("Actual possessed arena player"), Player.Get())
				|| !Test->TestTrue(TEXT("Director observes the actual arena player"), Director->GetPlayer() == Player.Get())
				|| !Test->TestEqual(TEXT("Actual configured arena density is three"), Director->GetWaveEnemyMultiplier(), 3))
			{
				Cleanup();
				return true;
			}
			bool bTooClose = false;
			for (const FVector& Point : Director->GetSpawnPoints())
				bTooClose |= FVector::Dist2D(Point, Player->GetActorLocation()) < 600;
			if (bTooClose)
			{
				// This is the saved combat-layout PlayerStart, clear of all eight points.
				const FVector SafePlayerStart(-2200, -2400, 100);
				for (const FVector& Point : Director->GetSpawnPoints())
					if (!Test->TestTrue(TEXT("Known PlayerStart leaves every native distance check open"), FVector::Dist2D(Point, SafePlayerStart) >= 600))
					{ Cleanup(); return true; }
				Player->SetActorLocation(SafePlayerStart, false, nullptr, ETeleportType::TeleportPhysics);
				Player->GetCharacterMovement()->StopMovementImmediately();
			}
			bSetup = true;
			StartedGame = Current->GetTimeSeconds();
			return false;
		}

		void Spawned(AEnemyBase* Enemy, const FMothWaveSpawn& Spec)
		{
			const int32 Wave = Director->GetWaveNumber() - 1;
			if (!IsValid(Enemy) || Wave < 0 || Wave >= 3)
			{
				Test->AddError(TEXT("Director reported an invalid high-density birth"));
				bFailed = true;
				return;
			}
			++Births[Wave];
			const int32 Kind = static_cast<int32>(Spec.Kind);
			if (Kind < 0 || Kind >= 3 || Enemy->GetDropKind() != Spec.Kind)
			{
				Test->AddError(TEXT("Actual pooled class does not match its fixed-drop wave entry"));
				bFailed = true;
				return;
			}
			++KindCounts[Wave][Kind];
			WaveEnemies[Wave].Add(Enemy);
			const uint32 ActorId = Enemy->GetUniqueID();
			int32& Lives = LivesByActor.FindOrAdd(ActorId);
			UAnimInstance* Animation = Enemy->GetMesh()->GetAnimInstance();
			if (!Test->TestNotNull(TEXT("Saved enemy retains a working skeletal animation instance"), Animation)
				|| !Test->TestTrue(TEXT("Checkout restores Blueprint animation after a previous death animation"),
					Enemy->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationBlueprint)) bFailed = true;
			if (Lives > 0)
			{
				++ReusedLives;
				if (!Test->TestTrue(TEXT("Reused actor restores the same animation Blueprint class"),
					Animation && AnimationClasses.FindRef(ActorId).Get() == Animation->GetClass())) bFailed = true;
			}
			else AnimationClasses.Add(ActorId, Animation ? Animation->GetClass() : nullptr);
			++Lives;
			PeakAlive = FMath::Max(PeakAlive, Director->GetAliveCount());
			// Frozen fixture actors would otherwise occupy repeated spawn points forever.
			// Only these admitted enemies lose AI/occupancy; map ground, NavMesh and all
			// native birth validation still run for every later scheduled admission.
			Enemy->SetGameplayEnabled(false);
			Enemy->GetCharacterMovement()->DisableMovement();
			Enemy->SetActorEnableCollision(false);
		}

		void KillWave(int32 Wave)
		{
			for (const TWeakObjectPtr<AEnemyBase>& Entry : WaveEnemies[Wave])
			{
				AEnemyBase* Enemy = Entry.Get();
				if (!IsValid(Enemy) || Enemy->IsPoolInactive() || !Enemy->GetHealthComponent()->IsAlive())
				{
					Test->AddError(TEXT("An admitted enemy disappeared before its controlled real health death"));
					bFailed = true;
					continue;
				}
				FHitContext Hit;
				Hit.HitId = FGuid::NewGuid();
				Hit.SourceActor = Player.Get();
				Hit.InstigatorPawn = Player.Get();
				Hit.Damage = 60;
				Hit.ImpactPoint = Enemy->GetActorLocation();
				Hit.ShotDirection = (Enemy->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
				// Gameplay is disabled solely to stop AI. Apply through the same health
				// component used by weapons, including attribution, death, BP and drops.
				if (!Test->TestTrue(TEXT("Player-attributed 60-damage health hit commits a real enemy death"), Enemy->GetHealthComponent()->ApplyHit(Hit))
					|| !Test->TestFalse(TEXT("The committed health death is lethal"), Enemy->GetHealthComponent()->IsAlive())) bFailed = true;
				else ++AcceptedKills;
				if (!Test->TestTrue(TEXT("Normal death retains its own visual lifetime before returning to the pool"), Enemy->GetLifeSpan() > 0)) bFailed = true;
				if (!Test->TestEqual(TEXT("Each actual life attempts its fixed drop exactly once"), Enemy->GetDropAttempts(), 1)) bFailed = true;
				if (ADeviceBase* Drop = Enemy->GetDroppedDevice()) Drop->DestroyDevice();
			}
			Test->AddInfo(FString::Printf(TEXT("High-density wave=%d births=%d kinds=%d/%d/%d kills=%d peakAlive=%d uniqueActors=%d reusedLives=%d"),
				Wave + 1, Births[Wave], KindCounts[Wave][0], KindCounts[Wave][1], KindCounts[Wave][2],
				AcceptedKills, PeakAlive, LivesByActor.Num(), ReusedLives));
		}

		void ValidateVictory()
		{
			const int32 ExpectedKinds[3][3] = { {6, 3, 3}, {3, 6, 9}, {6, 9, 9} };
			for (int32 Wave = 0; Wave < 3; ++Wave)
			{
				Test->TestEqual(*FString::Printf(TEXT("Actual high-density wave %d births"), Wave + 1), Births[Wave], ExpectedBirths[Wave]);
				for (int32 Kind = 0; Kind < 3; ++Kind)
					Test->TestEqual(*FString::Printf(TEXT("Actual wave %d D0%d composition"), Wave + 1, Kind + 1), KindCounts[Wave][Kind], ExpectedKinds[Wave][Kind]);
			}
			Test->TestEqual(TEXT("All 54 scheduled lives were actually spawned"), Births[0] + Births[1] + Births[2], 54);
			Test->TestEqual(TEXT("All 54 actual health deaths were accepted"), AcceptedKills, 54);
			Test->TestEqual(TEXT("Player kills count every life of reused actors exactly once"), Director->GetKillCount(), 54);
			Test->TestEqual(TEXT("Actual saved arena admits a peak of 24 living enemies"), PeakAlive, 24);
			Test->TestEqual(TEXT("Victory has no living enemies"), Director->GetAliveCount(), 0);
			Test->TestEqual(TEXT("Victory has no pending births"), Director->GetPendingCount(), 0);
			auto* Pool = World->GetSubsystem<UEnemyPoolSubsystem>();
			if (!Test->TestNotNull(TEXT("Actual arena world owns its enemy pool"), Pool)) return;
			Test->TestTrue(TEXT("Real waves reuse previously dead actors"), Pool->GetReuseCount() > 0 && ReusedLives > 0);
			Test->TestTrue(TEXT("High density allocates fewer actors than its 54 total lives"), Pool->GetCreatedCount() < 54 && LivesByActor.Num() < 54);
			Test->AddInfo(FString::Printf(TEXT("High-density SoulCity victory: births=%d/%d/%d kills=%d peak=%d created=%d poolReuses=%d uniqueActors=%d reusedLives=%d elapsedGame=%.3fs"),
				Births[0], Births[1], Births[2], Director->GetKillCount(), PeakAlive, Pool->GetCreatedCount(),
				Pool->GetReuseCount(), LivesByActor.Num(), ReusedLives, World->GetTimeSeconds() - StartedGame));
		}

		void Cleanup()
		{
			if (Director.IsValid() && SpawnHandle.IsValid()) Director->OnEnemySpawnedNative.Remove(SpawnHandle);
			SpawnHandle.Reset();
		}

		FAutomationTestBase* Test;
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AMothWaveDirector> Director;
		TWeakObjectPtr<AMothEffectCharacter> Player;
		FDelegateHandle SpawnHandle;
		TArray<TWeakObjectPtr<AEnemyBase>> WaveEnemies[3];
		TMap<uint32, int32> LivesByActor;
		TMap<uint32, TWeakObjectPtr<UClass>> AnimationClasses;
		const int32 ExpectedBirths[3] = { 12, 18, 24 };
		int32 Births[3] = { 0, 0, 0 };
		int32 KindCounts[3][3] = {};
		bool bKilledWave[3] = {};
		int32 PeakAlive = 0;
		int32 AcceptedKills = 0;
		int32 ReusedLives = 0;
		bool bSetup = false;
		bool bFailed = false;
		double StartedWall = FPlatformTime::Seconds();
		double StartedGame = 0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothHighDensityEncounterRuntime,
	"MothEffect.PoolingRuntime.HighDensityEncounter",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothHighDensityEncounterRuntime::RunTest(const FString&)
{
	ADD_LATENT_AUTOMATION_COMMAND(MothPoolingRuntimeTest::FHighDensityEncounter(this));
	return true;
}

#endif
