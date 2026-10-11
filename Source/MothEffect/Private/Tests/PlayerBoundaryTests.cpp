#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PlayerAutomationCharacter.h"
#include "Tests/CombatTestWorld.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "Devices/DeviceBase.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Weapons/Rifle.h"

namespace
{
	enum class EPlayerBoundaryCase
	{
		ThrowPress, BlockedRelease, FocusAndFlush, PauseTimers, ReloadCompetition,
		GameplayCancellation, DeathAndEndPlay, HeldInvalidation, Unpossessed
	};

	/** Exercise real actors, traces and world ticks, replacing only hardware input and visual assets. */
	class FPlayerBoundaryCommand final : public IAutomationLatentCommand
	{
	public:
		FPlayerBoundaryCommand(FAutomationTestBase* InTest, EPlayerBoundaryCase InCase)
			: Test(InTest), Case(InCase) {}

		virtual bool Update() override
		{
			if (!Fixture)
			{
				Fixture = MakeUnique<FMothCombatTestWorld>();
				if (!Test->TestNotNull(TEXT("Create isolated gameplay world"), Fixture->World)) return true;
				Player = Fixture->Spawn<AMothPlayerAutomationCharacter>(FVector(0.0, 0.0, 500.0));
				if (!Test->TestNotNull(TEXT("Spawn native player without animation assets"), Player)) return true;
				Rifle = Player->GetRifle();
				if (!Test->TestNotNull(TEXT("Player binds its gameplay rifle"), Rifle)) return true;
				Fixture->World->GetTimerManager().Tick(0.0f);
				InitialAmmo = Rifle->GetAmmoInMagazine();
				if (Case != EPlayerBoundaryCase::BlockedRelease && Case != EPlayerBoundaryCase::HeldInvalidation)
				{
					Test->AddExpectedError(TEXT("could not play on"), EAutomationExpectedErrorFlags::Contains, 1);
				}
				return !BeginCase();
			}
			UWorld* World = Fixture->World;
			if (World->GetTimerManager().HasBeenTickedThisFrame()) return false;
			// Tick only this isolated world, never the user's editor/PIE world or global frame counter.
			World->Tick(LEVELTICK_All, 0.05f);
			if (++Frames > 160)
			{
				Test->AddError(TEXT("Player boundary case exceeded its bounded timeline"));
				return true;
			}
			const double Elapsed = World->GetTimeSeconds() - StageStart;
			switch (Case)
			{
			case EPlayerBoundaryCase::ThrowPress: return CheckThrow(Elapsed);
			case EPlayerBoundaryCase::BlockedRelease:
				if (Elapsed < 0.5) return false;
				Test->TestTrue(TEXT("Successful retry completes recovery without a Montage"), IsState(EPlayerActionState::Ready));
				Test->TestEqual(TEXT("Blocked and successful throws consume no rifle ammunition"), Rifle->GetAmmoInMagazine(), InitialAmmo);
				return true;
			case EPlayerBoundaryCase::FocusAndFlush: return CheckFocus(Elapsed);
			case EPlayerBoundaryCase::PauseTimers: return CheckPause(Elapsed);
			case EPlayerBoundaryCase::ReloadCompetition: return CheckReload(Elapsed);
			case EPlayerBoundaryCase::GameplayCancellation:
				if (Stage == 0 && Elapsed >= 0.1)
				{
					Player->SetGameplayEnabled(false);
					Test->TestTrue(TEXT("Round cancellation immediately clears recovery"), IsState(EPlayerActionState::Ready));
					Player->SetGameplayEnabled(true);
					ReleasePrimary();
					Player->DoReloadStart();
					Test->TestTrue(TEXT("Start a new legal task after enabling gameplay"), IsState(EPlayerActionState::Reloading));
					NextStage();
				}
				else if (Stage == 1 && Elapsed >= 0.5)
				{
					Test->TestTrue(TEXT("Cancelled throw callback cannot overwrite the new reload"), IsState(EPlayerActionState::Reloading));
					Test->TestEqual(TEXT("Cancelled recovery cannot refill ammunition"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
					NextStage();
				}
				else if (Stage == 2 && Elapsed >= 0.9)
				{
					Test->TestTrue(TEXT("The new reload finishes normally without animation"), IsState(EPlayerActionState::Ready));
					Test->TestEqual(TEXT("Only the new reload refills ammunition"), Rifle->GetAmmoInMagazine(), InitialAmmo);
					return true;
				}
				return false;
			case EPlayerBoundaryCase::DeathAndEndPlay:
				if (Elapsed < 1.5) return false;
				Test->TestTrue(TEXT("Death stays terminal after old reload deadline"), IsState(EPlayerActionState::Dead));
				Test->TestEqual(TEXT("Death cancels refill"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				Test->TestTrue(TEXT("Fresh player does not inherit destroyed pawn tasks"),
					NewPlayer->GetActionState() == EPlayerActionState::Ready);
				Test->TestEqual(TEXT("Fresh player keeps its own full magazine"), NewPlayer->GetRifle()->GetAmmoInMagazine(), InitialAmmo);
				return true;
			case EPlayerBoundaryCase::HeldInvalidation:
				if (Elapsed < 0.5) return false;
				Test->TestNull(TEXT("Round cleanup removes the held reference"), Player->GetHeldDevice());
				Test->TestTrue(TEXT("Round cleanup leaves no carrying/recovery lock"), IsState(EPlayerActionState::Ready));
				return true;
			case EPlayerBoundaryCase::Unpossessed:
				if (Stage == 0 && Elapsed >= 0.5)
				{
					Test->TestEqual(TEXT("Unpossessed pawn does not keep firing"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
					Test->TestFalse(TEXT("Unpossessed pawn requires a new press"), Player->CanFireRifle());
					ReleasePrimary();
					Player->DoReloadStart();
					Test->TestTrue(TEXT("Start a reload before releasing control again"), Rifle->IsReloading());
					Player->UnPossessed();
					Test->TestFalse(TEXT("Unpossessed cancels the reload"), Rifle->IsReloading());
					if (!Pickup()) return true;
					PressPrimary();
					Test->TestTrue(TEXT("Recovery is pending before releasing control"), IsState(EPlayerActionState::ThrowRecovery));
					Player->UnPossessed();
					Test->TestTrue(TEXT("Unpossessed clears the recovery immediately"), IsState(EPlayerActionState::Ready));
					NextStage();
				}
				else if (Stage == 1 && Elapsed >= 1.4)
				{
					Test->TestEqual(TEXT("Old unpossessed tasks cannot fire or refill later"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
					return true;
				}
				return false;
			}
			return true;
		}

	private:
		bool IsState(EPlayerActionState Expected) const { return Player->GetActionState() == Expected; }
		void NextStage() { ++Stage; StageStart = Fixture->World->GetTimeSeconds(); }
		void PressPrimary() { Player->SetPrimaryDown(true); Player->DoPrimaryStart(); }
		void ReleasePrimary() { Player->SetPrimaryDown(false); Player->DoPrimaryEnd(); Player->Tick(0.0f); }
		bool Pickup()
		{
			Device = Fixture->Spawn<ADeviceBase>(Player->GetActorLocation() + FVector(130.0, 0.0, 0.0));
			if (!Test->TestNotNull(TEXT("Spawn dormant device"), Device)) return false;
			if (!Test->TestTrue(TEXT("Pick up through gameplay validation"), Player->TryPickupDevice(Device))) return false;
			ReleasePrimary();
			return true;
		}
		void ApplyDamage(float Damage)
		{
			FHitContext Hit;
			Hit.HitId = FGuid::NewGuid();
			Hit.ShotDirection = FVector::ForwardVector;
			Hit.Damage = Damage;
			Test->TestTrue(TEXT("Player accepts a fresh damage hit"), IBallisticReactive::Execute_ReceiveBallisticHit(Player, Hit));
		}
		bool Pause()
		{
			APlayerState* Pauser = Fixture->Spawn<APlayerState>();
			if (!Test->TestNotNull(TEXT("Create pause owner"), Pauser)) return false;
			Fixture->World->GetWorldSettings()->SetPauserPlayerState(Pauser);
			Player->CancelCombatInput(); // Same pawn hook used by the gameplay controller's SetPause.
			PausedTime = Fixture->World->GetTimeSeconds();
			PausedProgress = Rifle->GetReloadProgress();
			PauseFrames = 12;
			return true;
		}
		bool BeginCase()
		{
			StageStart = Fixture->World->GetTimeSeconds();
			switch (Case)
			{
			case EPlayerBoundaryCase::ThrowPress:
			case EPlayerBoundaryCase::BlockedRelease:
				if (!Pickup()) return false;
				if (Case == EPlayerBoundaryCase::BlockedRelease)
				{
					AActor* Wall = Fixture->Spawn<AActor>();
					if (!Test->TestNotNull(TEXT("Create release obstruction"), Wall)) return false;
					UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
					Wall->SetRootComponent(Box);
					Box->SetBoxExtent(FVector(1.0, 100.0, 100.0));
					Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
					Box->SetCollisionResponseToAllChannels(ECR_Block);
					Box->RegisterComponent();
					Box->SetWorldLocation(Player->GetActorLocation() + FVector(75.0, 0.0, 20.0));
					PressPrimary();
					Test->TestTrue(TEXT("Obstructed throw keeps Carrying"), IsState(EPlayerActionState::Carrying));
					Test->TestTrue(TEXT("Obstructed throw retains the same device"), Player->GetHeldDevice() == Device);
					Wall->Destroy();
					Player->DoPrimaryStart();
					Test->TestTrue(TEXT("Repeated Started in one held press cannot retry release"), Player->IsCarryingDevice());
					ReleasePrimary();
				}
				PressPrimary();
				return Test->TestTrue(TEXT("Only a successful physical throw enters recovery"), IsState(EPlayerActionState::ThrowRecovery));
			case EPlayerBoundaryCase::FocusAndFlush:
				PressPrimary();
				Test->TestEqual(TEXT("First press fires one shot"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				Player->SetInputFocused(false);
				Player->Tick(0.0f);
				return true;
			case EPlayerBoundaryCase::Unpossessed:
				PressPrimary();
				Player->UnPossessed();
				return true;
			case EPlayerBoundaryCase::PauseTimers:
			case EPlayerBoundaryCase::ReloadCompetition:
				PressPrimary();
				ReleasePrimary();
				Player->DoReloadStart();
				Test->TestTrue(TEXT("Start legal reload without a Montage or AnimInstance"), Rifle->IsReloading());
				if (Case == EPlayerBoundaryCase::ReloadCompetition)
				{
					Test->TestFalse(TEXT("Invalid candidate does not pick up"), Player->TryPickupDevice(nullptr));
					Test->TestTrue(TEXT("Failed pickup preserves reload"), Rifle->IsReloading());
					ApplyDamage(10.0f);
					Test->TestTrue(TEXT("Ordinary damage does not cancel reload"), Rifle->IsReloading());
				}
				return true;
			case EPlayerBoundaryCase::GameplayCancellation:
				PressPrimary();
				ReleasePrimary();
				if (!Pickup()) return false;
				PressPrimary();
				return Test->TestTrue(TEXT("Enter recovery before ending round"), IsState(EPlayerActionState::ThrowRecovery));
			case EPlayerBoundaryCase::DeathAndEndPlay:
				PressPrimary();
				ReleasePrimary();
				Player->DoReloadStart();
				ApplyDamage(1000.0f);
				Test->TestFalse(TEXT("Death cancels weapon reload immediately"), Rifle->IsReloading());
				{
					AMothPlayerAutomationCharacter* EndingPlayer = Fixture->Spawn<AMothPlayerAutomationCharacter>(FVector(1000.0, 0.0, 500.0));
					if (!Test->TestNotNull(TEXT("Spawn pawn to destroy during recovery"), EndingPlayer)) return false;
					ADeviceBase* EndingDevice = Fixture->Spawn<ADeviceBase>(FVector(1130.0, 0.0, 500.0));
					if (!Test->TestTrue(TEXT("Ending pawn picks up"), EndingPlayer->TryPickupDevice(EndingDevice))) return false;
					EndingPlayer->Tick(0.0f);
					EndingPlayer->SetPrimaryDown(true);
					EndingPlayer->DoPrimaryStart();
					Test->TestTrue(TEXT("Ending pawn has a pending recovery"), EndingPlayer->GetActionState() == EPlayerActionState::ThrowRecovery);
					EndingPlayer->Destroy();
				}
				NewPlayer = Fixture->Spawn<AMothPlayerAutomationCharacter>(FVector(2000.0, 0.0, 500.0));
				return Test->TestNotNull(TEXT("Spawn fresh pawn after EndPlay"), NewPlayer);
			case EPlayerBoundaryCase::HeldInvalidation:
				if (!Pickup()) return false;
				{
					FHitContext Hit;
					Hit.HitId = FGuid::NewGuid();
					Hit.ShotDirection = FVector::ForwardVector;
					Test->TestFalse(TEXT("Held is immune to activation"), Device->TryActivate(Hit));
					ApplyDamage(10.0f);
					Test->TestTrue(TEXT("Nonfatal damage preserves Carrying"), IsState(EPlayerActionState::Carrying));
				}
				Device->DestroyDevice();
				Test->TestNull(TEXT("Unexpected device destruction clears held reference"), Player->GetHeldDevice());
				Test->TestTrue(TEXT("Unexpected device destruction unlocks living player"), IsState(EPlayerActionState::Ready));
				Test->TestFalse(TEXT("Separate rifle visibility is restored"), Rifle->IsHidden());
				if (!Pickup()) return false;
				Player->SetGameplayEnabled(false);
				Test->TestTrue(TEXT("Round cleanup destroys Held instead of dropping it"), Device->GetDeviceState() == EDeviceState::Destroyed);
				return true;
			}
			return false;
		}
		bool CheckThrow(double Elapsed)
		{
			if (Stage == 0 && Elapsed >= 0.5)
			{
				Test->TestTrue(TEXT("No animation still completes timed recovery"), IsState(EPlayerActionState::Ready));
				Player->DoPrimaryStart();
				Test->TestEqual(TEXT("Holding the throw press across recovery never fires"), Rifle->GetAmmoInMagazine(), InitialAmmo);
				ReleasePrimary();
				Device->DestroyDevice();
				if (!Pickup()) return true;
				PressPrimary();
				ReleasePrimary();
				PressPrimary();
				Test->TestTrue(TEXT("Premature new press cannot fire during recovery"), IsState(EPlayerActionState::ThrowRecovery));
				NextStage();
			}
			else if (Stage == 1 && Elapsed >= 0.5)
			{
				Test->TestTrue(TEXT("Second recovery completes independently of premature input"), IsState(EPlayerActionState::Ready));
				Player->DoPrimaryStart();
				Test->TestEqual(TEXT("Premature press is not buffered across recovery"), Rifle->GetAmmoInMagazine(), InitialAmmo);
				ReleasePrimary();
				PressPrimary();
				Test->TestEqual(TEXT("Release plus a fresh post-recovery press fires once"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				Player->DoPrimaryStart();
				Test->TestEqual(TEXT("Duplicate Started cannot add a second immediate shot"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				ReleasePrimary();
				return true;
			}
			return false;
		}
		bool CheckFocus(double Elapsed)
		{
			if (Stage == 0 && Elapsed >= 0.5)
			{
				Test->TestEqual(TEXT("Focus loss stops fire even without a viewport Flush callback"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				Player->SetInputFocused(true);
				Player->DoPrimaryStart();
				Test->TestFalse(TEXT("Returning while physically held cannot resume fire"), Player->CanFireRifle());
				ReleasePrimary();
				PressPrimary();
				const int32 Ammo = Rifle->GetAmmoInMagazine();
				Player->DoPrimaryCanceled();
				Player->DoPrimaryEnd(); // A synthetic Completed from flushed keys is not a hardware release.
				Player->Tick(0.0f);
				Player->DoPrimaryStart();
				Test->TestFalse(TEXT("Canceled and synthetic Completed preserve the physical release gate"), Player->CanFireRifle());
				Test->TestEqual(TEXT("Flush does not fire or refill ammunition"), Rifle->GetAmmoInMagazine(), Ammo);
				InitialAmmo = Ammo;
				NextStage();
			}
			else if (Stage == 1 && Elapsed >= 0.5)
			{
				Test->TestEqual(TEXT("No fire timer survives cancellation"), Rifle->GetAmmoInMagazine(), InitialAmmo);
				ReleasePrimary();
				PressPrimary();
				Test->TestEqual(TEXT("Actual release restores the next fresh shot"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				ReleasePrimary();
				return true;
			}
			return false;
		}
		bool CheckPause(double Elapsed)
		{
			if (PauseFrames > 0)
			{
				Test->TestEqual(TEXT("Actual paused World::Tick freezes gameplay time"), Fixture->World->GetTimeSeconds(), PausedTime);
				Test->TestEqual(TEXT("Paused reload progress is preserved"), Rifle->GetReloadProgress(), PausedProgress);
				if (--PauseFrames == 0)
				{
					Fixture->World->GetWorldSettings()->SetPauserPlayerState(nullptr);
					NextStage();
				}
				return false;
			}
			if (Stage == 0 && Elapsed >= 0.2)
			{
				Test->TestTrue(TEXT("Reload is partially complete before pause"), Rifle->GetReloadProgress() > 0.0f);
				return !Pause();
			}
			if (Stage == 1 && Elapsed >= 1.2)
			{
				Test->TestTrue(TEXT("Legal reload resumes and completes without animation"), IsState(EPlayerActionState::Ready));
				Test->TestEqual(TEXT("Reload fills once after remaining time"), Rifle->GetAmmoInMagazine(), InitialAmmo);
				if (!Pickup()) return true;
				PressPrimary();
				NextStage();
			}
			else if (Stage == 2 && Elapsed >= 0.1)
			{
				Test->TestTrue(TEXT("Recovery is pending before pause"), IsState(EPlayerActionState::ThrowRecovery));
				return !Pause();
			}
			else if (Stage == 3 && Elapsed >= 0.35)
			{
				Test->TestTrue(TEXT("Throw recovery resumes for its remaining time"), IsState(EPlayerActionState::Ready));
				Player->DoPrimaryStart();
				Test->TestEqual(TEXT("Pause/resume does not release a physically held throw press"), Rifle->GetAmmoInMagazine(), InitialAmmo);
				ReleasePrimary();
				PressPrimary();
				Test->TestEqual(TEXT("Fresh post-pause press fires"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				ReleasePrimary();
				return true;
			}
			return false;
		}
		bool CheckReload(double Elapsed)
		{
			if (Stage == 0 && Elapsed >= 0.2)
			{
				if (!Pickup()) return true;
				Test->TestFalse(TEXT("Successful pickup cancels the unfinished reload"), Rifle->IsReloading());
				Test->TestEqual(TEXT("Cancelled reload does not refill"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				Test->TestTrue(TEXT("Safe drop returns directly to Ready"), Player->TryReleaseHeldDevice(false) && IsState(EPlayerActionState::Ready));
				Device->DestroyDevice();
				Player->DoReloadStart();
				NextStage();
			}
			else if (Stage == 1 && Elapsed >= 1.15)
			{
				Test->TestTrue(TEXT("Old completion deadline cannot finish the newer reload"), Rifle->IsReloading());
				Test->TestEqual(TEXT("Old task cannot refill ammunition"), Rifle->GetAmmoInMagazine(), InitialAmmo - 1);
				NextStage();
			}
			else if (Stage == 2 && Elapsed >= 0.25)
			{
				Test->TestTrue(TEXT("New reload completes exactly through its own task"), IsState(EPlayerActionState::Ready));
				Test->TestEqual(TEXT("Current reload fills ammunition"), Rifle->GetAmmoInMagazine(), InitialAmmo);
				return true;
			}
			return false;
		}

		FAutomationTestBase* Test;
		EPlayerBoundaryCase Case;
		TUniquePtr<FMothCombatTestWorld> Fixture;
		AMothPlayerAutomationCharacter* Player = nullptr;
		AMothPlayerAutomationCharacter* NewPlayer = nullptr;
		ARifle* Rifle = nullptr;
		ADeviceBase* Device = nullptr;
		int32 InitialAmmo = 0;
		int32 Stage = 0;
		int32 Frames = 0;
		int32 PauseFrames = 0;
		float PausedProgress = 0.0f;
		double StageStart = 0.0;
		double PausedTime = 0.0;
	};
}

#define MOTH_PLAYER_BOUNDARY_TEST(ClassName, TestName, CaseName) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, "MothEffect.Player.T09." TestName, \
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool ClassName::RunTest(const FString& Parameters) \
	{ \
		ADD_LATENT_AUTOMATION_COMMAND(FPlayerBoundaryCommand(this, EPlayerBoundaryCase::CaseName)); \
		return true; \
	}

MOTH_PLAYER_BOUNDARY_TEST(FMothThrowPressTest, "SinglePressAndRecovery", ThrowPress)
MOTH_PLAYER_BOUNDARY_TEST(FMothBlockedThrowTest, "BlockedReleaseRetainsHeld", BlockedRelease)
MOTH_PLAYER_BOUNDARY_TEST(FMothPlayerFocusTest, "FocusAndPhysicalRelease", FocusAndFlush)
MOTH_PLAYER_BOUNDARY_TEST(FMothPlayerPauseTest, "PausePreservesTimers", PauseTimers)
MOTH_PLAYER_BOUNDARY_TEST(FMothPickupReloadTest, "PickupReloadCompetition", ReloadCompetition)
MOTH_PLAYER_BOUNDARY_TEST(FMothRoundCancellationTest, "RoundCancellationRejectsOldTasks", GameplayCancellation)
MOTH_PLAYER_BOUNDARY_TEST(FMothPlayerEndPlayTest, "DeathAndEndPlayCleanup", DeathAndEndPlay)
MOTH_PLAYER_BOUNDARY_TEST(FMothHeldInvalidationTest, "HeldInvalidationAndDamage", HeldInvalidation)
MOTH_PLAYER_BOUNDARY_TEST(FMothUnpossessedFireTest, "UnpossessedStopsFire", Unpossessed)

#undef MOTH_PLAYER_BOUNDARY_TEST

#endif
