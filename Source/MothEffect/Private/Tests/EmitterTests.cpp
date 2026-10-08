#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/RuleProjectile.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Tests/CombatTestWorld.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	enum class EEmitterCase { DirectionAndLifetime, BlockedBirth, Cancellation };

	/** Advance only this world's gameplay clock/timers once per real automation frame.
	 * Physics and projectile flight are intentionally excluded from these schedule tests. */
	class FEmitterTimelineCommand final : public IAutomationLatentCommand
	{
	public:
		FEmitterTimelineCommand(FAutomationTestBase* InTest, EEmitterCase InCase)
			: Test(InTest), Case(InCase) {}

		virtual bool Update() override
		{
			if (!Fixture)
			{
				Fixture = MakeUnique<FMothCombatTestWorld>();
				if (!Test->TestNotNull(TEXT("Create isolated initialized world"), Fixture->World))
				{
					return true;
				}
				// Prime pending timers before activation; subsequent updates use new engine frames.
				Fixture->World->GetTimerManager().Tick(0.0f);
				Emitter = Fixture->Spawn<ADeviceBase>();
				if (!Test->TestNotNull(TEXT("Spawn native emitter"), Emitter))
				{
					return true;
				}
				FHitContext Hit;
				Hit.HitId = FGuid::NewGuid();
				Hit.ShotDirection = Case == EEmitterCase::BlockedBirth
					? FVector::ForwardVector : FVector(0.0, 3.0, 0.0);
				if (Case == EEmitterCase::BlockedBirth)
				{
					SetFloat(TEXT("EmitterDurationSeconds"), 1.0f);
					SetFloat(TEXT("EmitterFirstShotDelaySeconds"), 0.25f);
					Wall = Fixture->Spawn<AActor>();
					if (!Test->TestNotNull(TEXT("Spawn muzzle wall"), Wall))
					{
						return true;
					}
					UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
					Wall->SetRootComponent(Box);
					Box->SetBoxExtent(FVector(1.0, 100.0, 100.0));
					Box->SetCollisionObjectType(ECC_WorldStatic);
					Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
					Box->SetCollisionResponseToAllChannels(ECR_Block);
					Box->RegisterComponent();
					// Outside the 20cm body, inside the 26cm muzzle segment.
					Box->SetWorldLocation(FVector(23.0, 0.0, 0.0));
				}
				Test->TestTrue(TEXT("Activate D03 once"), Emitter->TryActivate(Hit));
				Test->TestTrue(TEXT("Active keeps gravity"), Emitter->GetPhysicsBody()->IsGravityEnabled());
				Test->TestTrue(TEXT("Active keeps physical simulation"), Emitter->GetPhysicsBody()->IsSimulatingPhysics());
				Test->TestTrue(TEXT("Direction cue is visible before the first shot"),
					Emitter->GetEmitterDirectionMarker()->IsVisible());
				Test->TestEqual(TEXT("Startup delay prevents an immediate shot"), Emitter->GetEmitterShotAttempts(), 0);
				if (Case == EEmitterCase::Cancellation)
				{
					SecondEmitter = Fixture->Spawn<ADeviceBase>(FVector(600.0, 0.0, 0.0));
					if (!Test->TestNotNull(TEXT("Spawn emitter for EndPlay cancellation"), SecondEmitter))
					{
						return true;
					}
					Hit.HitId = FGuid::NewGuid();
					Test->TestTrue(TEXT("Activate second emitter"), SecondEmitter->TryActivate(Hit));
				}
				return false;
			}

			UWorld* World = Fixture->World;
			if (PauseFrames > 0)
			{
				Test->TestTrue(TEXT("World remains paused"), World->IsPaused());
				Test->TestEqual(TEXT("Pause does not emit another projectile"), Emitter->GetEmitterShotAttempts(), PausedAttempts);
				Test->TestEqual(TEXT("Pause freezes the gameplay clock"), World->GetTimeSeconds(), PausedTime);
				if (--PauseFrames == 0)
				{
					World->GetWorldSettings()->SetPauserPlayerState(nullptr);
				}
				return false;
			}
			if (World->GetTimerManager().HasBeenTickedThisFrame())
			{
				return false;
			}
			// No global frame counter changes or ticks of the user's editor/PIE world.
			World->TimeSeconds += 0.01f;
			World->GetTimerManager().Tick(0.01f);
			const double Elapsed = World->GetTimeSeconds();

			if (Case == EEmitterCase::DirectionAndLifetime)
			{
				if (!bChanged && Elapsed >= 0.2)
				{
					bChanged = true;
					Test->TestEqual(TEXT("One first shot after delay"), Emitter->GetEmitterShotAttempts(), 1);
					Emitter->SetActorLocation(FVector(100.0, 200.0, 300.0), false, nullptr, ETeleportType::TeleportPhysics);
					Emitter->SetActorRotation(FRotator(20.0, 135.0, 45.0), ETeleportType::TeleportPhysics);
					Emitter->GetPhysicsBody()->SetPhysicsLinearVelocity(FVector(500.0, 0.0, 700.0));
					FHitContext Replay;
					Replay.HitId = FGuid::NewGuid();
					Replay.ShotDirection = FVector::ForwardVector;
					Test->TestFalse(TEXT("Later hit cannot restart or redirect the emitter"), Emitter->TryActivate(Replay));
					Test->TestTrue(TEXT("Spin does not rotate the direction cue"),
						Emitter->GetEmitterDirectionMarker()->GetForwardVector().Equals(FVector::RightVector));
					PausedAttempts = Emitter->GetEmitterShotAttempts();
					PausedTime = Elapsed;
					APlayerState* Pauser = Fixture->Spawn<APlayerState>();
					if (!Test->TestNotNull(TEXT("Create pause owner"), Pauser))
					{
						return true;
					}
					World->GetWorldSettings()->SetPauserPlayerState(Pauser);
					PauseFrames = 5;
				}
				if (Elapsed >= 3.05)
				{
					Test->TestTrue(TEXT("Original lifetime reaches Spent"), Emitter->GetDeviceState() == EDeviceState::Spent);
					Test->TestEqual(TEXT("Baseline schedule is .1 + n*.25, strictly before 3s"), Emitter->GetEmitterShotAttempts(), 12);
					Test->TestEqual(TEXT("Clear space spawns all scheduled shots"), Emitter->GetEmitterProjectilesSpawned(), 12);
					Test->TestFalse(TEXT("Spent hides the direction cue"), Emitter->GetEmitterDirectionMarker()->IsVisible());
					Test->TestFalse(TEXT("Spent stops physical simulation"), Emitter->GetPhysicsBody()->IsSimulatingPhysics());
					int32 Projectiles = 0;
					int32 MovedOrigins = 0;
					for (TActorIterator<ARuleProjectile> It(World); It; ++It)
					{
						++Projectiles;
						Test->TestTrue(TEXT("Every projectile uses the activating world direction"),
							It->GetFlightDirection().Equals(FVector::RightVector));
						MovedOrigins += It->GetActorLocation().Equals(FVector(100.0, 226.0, 300.0), 0.1);
					}
					Test->TestEqual(TEXT("Independent projectile actors remain after emitter stops"), Projectiles, 12);
					Test->TestEqual(TEXT("Later birth positions follow body translation"), MovedOrigins, 11);
					return true;
				}
			}
			else if (Case == EEmitterCase::BlockedBirth)
			{
				if (!bChanged && Elapsed >= 0.55)
				{
					bChanged = true;
					Test->TestEqual(TEXT("Blocked slots are consumed"), Emitter->GetEmitterShotAttempts(), 2);
					Test->TestEqual(TEXT("No projectile is born across the thin wall"), Emitter->GetEmitterProjectilesSpawned(), 0);
					Wall->Destroy();
				}
				if (Elapsed >= 1.05)
				{
					Test->TestTrue(TEXT("Blocking does not extend active lifetime"), Emitter->GetDeviceState() == EDeviceState::Spent);
					Test->TestEqual(TEXT("Only .25, .5 and .75 count; exactly 1s is excluded"), Emitter->GetEmitterShotAttempts(), 3);
					Test->TestEqual(TEXT("Removing wall permits only the next slot, with no refund"), Emitter->GetEmitterProjectilesSpawned(), 1);
					return true;
				}
			}
			else
			{
				if (!bChanged && Elapsed >= 0.2)
				{
					bChanged = true;
					Test->TestEqual(TEXT("First emitter has emitted one shot"), Emitter->GetEmitterProjectilesSpawned(), 1);
					Test->TestEqual(TEXT("Second emitter has emitted one shot"), SecondEmitter->GetEmitterProjectilesSpawned(), 1);
					Emitter->SetGameplayEnabled(false);
					Test->TestTrue(TEXT("Gameplay cancellation commits Spent"), Emitter->GetDeviceState() == EDeviceState::Spent);
					SecondEmitter->DestroyDevice();
				}
				if (Elapsed >= 1.05)
				{
					int32 Projectiles = 0;
					for (TActorIterator<ARuleProjectile> It(World); It; ++It) { ++Projectiles; }
					Test->TestEqual(TEXT("Cancellation and EndPlay stop future births, preserving existing flight"), Projectiles, 2);
					return true;
				}
			}
			return false;
		}

	private:
		void SetFloat(FName Name, float Value)
		{
			FFloatProperty* Property = FindFProperty<FFloatProperty>(ADeviceBase::StaticClass(), Name);
			if (Test->TestNotNull(TEXT("Find editable emitter parameter"), Property))
			{
				Property->SetPropertyValue_InContainer(Emitter, Value);
			}
		}

		FAutomationTestBase* Test;
		EEmitterCase Case;
		TUniquePtr<FMothCombatTestWorld> Fixture;
		ADeviceBase* Emitter = nullptr;
		ADeviceBase* SecondEmitter = nullptr;
		AActor* Wall = nullptr;
		bool bChanged = false;
		int32 PauseFrames = 0;
		int32 PausedAttempts = 0;
		double PausedTime = 0.0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEmitterDirectionTest,
	"MothEffect.Devices.Emitter.WorldDirectionAndLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothEmitterDirectionTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEmitterTimelineCommand(this, EEmitterCase::DirectionAndLifetime));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEmitterBlockedBirthTest,
	"MothEffect.Devices.Emitter.BlockedBirthAndExpiry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothEmitterBlockedBirthTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEmitterTimelineCommand(this, EEmitterCase::BlockedBirth));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEmitterCancellationTest,
	"MothEffect.Devices.Emitter.CancellationStopsTimers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothEmitterCancellationTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEmitterTimelineCommand(this, EEmitterCase::Cancellation));
	return true;
}

#endif
