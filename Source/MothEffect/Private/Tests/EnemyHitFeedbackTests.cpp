#include "Tests/EnemyHitFeedbackObserver.h"
#include "GameFramework/Pawn.h"

void UMothEnemyHitFeedbackTestObserver::HandleEnemyHitConfirmed(APawn* Instigator, const FMothEnemyHitFeedback& Result)
{
#if WITH_DEV_AUTOMATION_TESTS
	Instigators.Add(Instigator);
	Results.Add(Result);
#endif
}

void UMothEnemyHitFeedbackTestObserver::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
#if WITH_DEV_AUTOMATION_TESTS
	++HealthNotifications;
	NotifiedHealth = CurrentHealth;
#endif
}

void UMothEnemyHitFeedbackTestObserver::HandleDied(AActor* Victim, AActor* Source)
{
#if WITH_DEV_AUTOMATION_TESTS
	++DeathNotifications;
#endif
}

#if WITH_DEV_AUTOMATION_TESTS
#include "AI/EnemyBase.h"
#include "Components/HealthComponent.h"
#include "Devices/DeviceBase.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/BallisticReactive.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Tests/CombatTestWorld.h"
#include "Tutorial/MothTutorialTarget.h"
#include "UObject/StrongObjectPtr.h"

namespace MothEnemyHitFeedbackTest
{
	struct FFixture
	{
		FMothCombatTestWorld Combat;
		APawn* Player = nullptr;
		APlayerController* Controller = nullptr;
		UMothFeedbackSubsystem* Feedback = nullptr;
		UDeviceLifecycleSubsystem* Session = nullptr;
		TStrongObjectPtr<UMothEnemyHitFeedbackTestObserver> BlueprintObserver;
		TArray<FMothEnemyHitFeedback> NativeResults;
		TArray<TWeakObjectPtr<APawn>> NativeInstigators;
		FDelegateHandle NativeHandle;

		FFixture()
		{
			if (!Combat.World) return;
			Feedback = Combat.World->GetSubsystem<UMothFeedbackSubsystem>();
			Session = Combat.World->GetSubsystem<UDeviceLifecycleSubsystem>();
			Player = Combat.Spawn<APawn>();
			Controller = Combat.Spawn<APlayerController>();
			// This world has no GameMode login flow to mark its synthetic player local.
			if (Controller) Controller->SetAsLocalPlayerController();
			if (Controller) Controller->SetPlayerState(Combat.Spawn<APlayerState>());
			if (Controller && Player) Controller->Possess(Player);
			if (!Feedback) return;
			BlueprintObserver.Reset(NewObject<UMothEnemyHitFeedbackTestObserver>(Combat.World));
			Feedback->OnEnemyHitConfirmed.AddDynamic(BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleEnemyHitConfirmed);
			NativeHandle = Feedback->OnEnemyHitConfirmedNative.AddLambda([this](APawn* Instigator, const FMothEnemyHitFeedback& Result)
			{
				NativeInstigators.Add(Instigator);
				NativeResults.Add(Result);
			});
		}

		~FFixture()
		{
			if (!IsValid(Feedback)) return;
			Feedback->OnEnemyHitConfirmedNative.Remove(NativeHandle);
			if (BlueprintObserver.IsValid())
				Feedback->OnEnemyHitConfirmed.RemoveDynamic(BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleEnemyHitConfirmed);
		}

		bool CheckReady(FAutomationTestBase& Test) const
		{
			bool bReady = Test.TestNotNull(TEXT("Isolated combat world"), Combat.World);
			bReady &= Test.TestNotNull(TEXT("Local player pawn"), Player);
			bReady &= Test.TestNotNull(TEXT("Actual player controller"), Controller);
			bReady &= Test.TestNotNull(TEXT("World feedback subsystem"), Feedback);
			bReady &= Test.TestNotNull(TEXT("World lifecycle subsystem"), Session);
			bReady &= Test.TestNotNull(TEXT("Blueprint multicast receiver"), BlueprintObserver.Get());
			if (!bReady) return false;
			bReady &= Test.TestTrue(TEXT("Fixture uses standalone local-player attribution"), Combat.World->GetNetMode() == NM_Standalone && Player->IsPlayerControlled() && Player->IsLocallyControlled());
			return bReady;
		}

		AEnemyBase* SpawnEnemy(const FVector& Position = FVector(500, 0, 100))
		{
			auto* Enemy = Combat.Spawn<AEnemyBase>(Position);
			if (Enemy) Enemy->GetCharacterMovement()->DisableMovement();
			return Enemy;
		}

		FHitContext Hit(float Damage = 10.0f, AActor* Source = nullptr) const
		{
			FHitContext Context;
			Context.HitId = FGuid::NewGuid();
			Context.SourceActor = Source ? Source : Player;
			Context.InstigatorPawn = Player;
			Context.Damage = Damage;
			Context.ImpactPoint = FVector(500, 23, 100);
			Context.ShotDirection = FVector::ForwardVector;
			return Context;
		}

		void CheckCounts(FAutomationTestBase& Test, int32 NativeCount, int32 BlueprintCount) const
		{
			Test.TestEqual(TEXT("Native result count"), NativeResults.Num(), NativeCount);
			Test.TestEqual(TEXT("Blueprint result count"), BlueprintObserver->Results.Num(), BlueprintCount);
		}

		void CheckResult(FAutomationTestBase& Test, int32 Index, AEnemyBase* Victim, const FHitContext& Context,
			float AppliedDamage, float RemainingHealth, bool bKilled) const
		{
			if (!NativeResults.IsValidIndex(Index) || !BlueprintObserver->Results.IsValidIndex(Index)) return;
			const FMothEnemyHitFeedback* Results[] = { &NativeResults[Index], &BlueprintObserver->Results[Index] };
			for (const FMothEnemyHitFeedback* Result : Results)
			{
				Test.TestTrue(TEXT("Snapshot retains the accepted hit ID"), Result->HitId == Context.HitId);
				Test.TestEqual(TEXT("Snapshot identifies the actual damaged enemy"), Result->VictimId, static_cast<int32>(Victim->GetUniqueID()));
				Test.TestEqual(TEXT("Snapshot reports actual applied damage"), Result->AppliedDamage, AppliedDamage);
				Test.TestEqual(TEXT("Snapshot reports committed remaining health"), Result->RemainingHealth, RemainingHealth);
				Test.TestEqual(TEXT("Snapshot reports committed death"), Result->bKilled, bKilled);
				Test.TestTrue(TEXT("Snapshot retains the impact point"), Result->ImpactPoint.Equals(Context.ImpactPoint));
			}
			Test.TestTrue(TEXT("Native event names the local damage instigator"), NativeInstigators[Index].Get() == Player);
			Test.TestTrue(TEXT("Blueprint event names the same local instigator"), BlueprintObserver->Instigators[Index].Get() == Player);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitSnapshotTest, "MothEffect.Feedback.EnemyHit.AcceptedDamageAndLethalSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitSnapshotTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture F;
	if (!F.CheckReady(*this)) return false;
	auto* Enemy = F.SpawnEnemy();
	if (!TestNotNull(TEXT("Native enemy victim"), Enemy)) return false;
	const float InitialHealth = Enemy->GetHealthComponent()->GetCurrentHealth();
	const float NonFatalDamage = InitialHealth * 0.25f;
	const FHitContext FirstHit = F.Hit(NonFatalDamage);
	TestTrue(TEXT("Actual health accepts nonfatal player damage"), Enemy->GetHealthComponent()->ApplyHit(FirstHit));
	F.CheckCounts(*this, 1, 1);
	F.CheckResult(*this, 0, Enemy, FirstHit, NonFatalDamage, InitialHealth - NonFatalDamage, false);
	TestEqual(TEXT("Feedback reflects real health mutation"), Enemy->GetHealthComponent()->GetCurrentHealth(), InitialHealth - NonFatalDamage);

	const FHitContext FatalHit = F.Hit(InitialHealth + 500.0f);
	TestTrue(TEXT("Actual ballistic entry accepts lethal damage"), IBallisticReactive::Execute_ReceiveBallisticHit(Enemy, FatalHit));
	F.CheckCounts(*this, 2, 2);
	F.CheckResult(*this, 1, Enemy, FatalHit, InitialHealth - NonFatalDamage, 0.0f, true);
	TestFalse(TEXT("Committed victim is actually dead"), Enemy->GetHealthComponent()->IsAlive());
	TestFalse(TEXT("Further fatal damage cannot emit another result"), Enemy->GetHealthComponent()->ApplyHit(F.Hit(100.0f)));
	F.CheckCounts(*this, 2, 2);
	// A later death must not mutate the earlier value delivered to UI.
	F.CheckResult(*this, 0, Enemy, FirstHit, NonFatalDamage, InitialHealth - NonFatalDamage, false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitDeduplicationTest, "MothEffect.Feedback.EnemyHit.HitIdIsScopedToEachVictim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitDeduplicationTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture F;
	if (!F.CheckReady(*this)) return false;
	auto* First = F.SpawnEnemy();
	auto* Second = F.SpawnEnemy(FVector(1000, 0, 100));
	if (!TestNotNull(TEXT("First enemy"), First) || !TestNotNull(TEXT("Second enemy"), Second)) return false;
	const FHitContext Hit = F.Hit();
	TestTrue(TEXT("First enemy accepts the hit"), First->GetHealthComponent()->ApplyHit(Hit));
	TestFalse(TEXT("Repeated hit on first enemy is rejected"), First->GetHealthComponent()->ApplyHit(Hit));
	F.CheckCounts(*this, 1, 1);
	TestTrue(TEXT("The same hit ID can damage a second enemy"), IBallisticReactive::Execute_ReceiveBallisticHit(Second, Hit));
	TestFalse(TEXT("Repeated hit on second enemy is rejected"), Second->GetHealthComponent()->ApplyHit(Hit));
	F.CheckCounts(*this, 2, 2);
	F.CheckResult(*this, 0, First, Hit, Hit.Damage, First->GetHealthComponent()->GetCurrentHealth(), false);
	F.CheckResult(*this, 1, Second, Hit, Hit.Damage, Second->GetHealthComponent()->GetCurrentHealth(), false);
	if (F.NativeResults.Num() == 2)
		TestTrue(TEXT("Shared hit ID preserves distinct victim identities"), F.NativeResults[0].VictimId != F.NativeResults[1].VictimId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitAttributionTest, "MothEffect.Feedback.EnemyHit.ExcludesUnattributedDamageAndNonEnemies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitAttributionTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture F;
	if (!F.CheckReady(*this)) return false;
	auto* Enemy = F.SpawnEnemy();
	auto* NonPlayer = F.Combat.Spawn<APawn>();
	auto* NonEnemy = F.Combat.Spawn<AMothTutorialTarget>(FVector(1200, 0, 100));
	if (!TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("Nonplayer source"), NonPlayer)
		|| !TestNotNull(TEXT("Damageable tutorial target"), NonEnemy)) return false;
	FHitContext Unattributed = F.Hit();
	Unattributed.InstigatorPawn = nullptr; // A player SourceActor alone must not invent attribution.
	TestTrue(TEXT("Unattributed damage still changes actual enemy health"), Enemy->GetHealthComponent()->ApplyHit(Unattributed));
	FHitContext NonPlayerHit = F.Hit();
	NonPlayerHit.InstigatorPawn = NonPlayer;
	NonPlayerHit.SourceActor = NonPlayer;
	TestTrue(TEXT("Nonplayer damage still changes actual enemy health"), Enemy->GetHealthComponent()->ApplyHit(NonPlayerHit));
	TestTrue(TEXT("Local player may damage a non-enemy target"), NonEnemy->GetHealthComponent()->ApplyHit(F.Hit()));
	F.CheckCounts(*this, 0, 0);
	TestTrue(TEXT("A subsequent valid local enemy hit still emits"), Enemy->GetHealthComponent()->ApplyHit(F.Hit()));
	F.CheckCounts(*this, 1, 1);
	F.Controller->Possess(Enemy);
	TestTrue(TEXT("Self-damage case uses a real locally controlled enemy"), Enemy->IsPlayerControlled() && Enemy->IsLocallyControlled());
	FHitContext SelfHit = F.Hit();
	SelfHit.InstigatorPawn = Enemy;
	SelfHit.SourceActor = Enemy;
	TestTrue(TEXT("Real self-damage may commit without an enemy-hit confirmation"), Enemy->GetHealthComponent()->ApplyHit(SelfHit));
	F.CheckCounts(*this, 1, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitDeviceSourceTest, "MothEffect.Feedback.EnemyHit.PreservesPlayerAttributionThroughDeviceSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitDeviceSourceTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture F;
	if (!F.CheckReady(*this)) return false;
	auto* Enemy = F.SpawnEnemy();
	auto* Device = F.Combat.Spawn<ADeviceBase>(FVector(2000, 0, 300));
	if (!TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("Player-owned device source"), Device)) return false;
	Device->SetInstigator(F.Player);
	const FHitContext DeviceHit = F.Hit(10.0f, Device);
	TestTrue(TEXT("The source actor is the device, not the player"), DeviceHit.SourceActor.Get() == Device && DeviceHit.SourceActor.Get() != F.Player);
	TestTrue(TEXT("Real ballistic damage from the attributed device commits"), IBallisticReactive::Execute_ReceiveBallisticHit(Enemy, DeviceHit));
	F.CheckCounts(*this, 1, 1);
	F.CheckResult(*this, 0, Enemy, DeviceHit, DeviceHit.Damage, Enemy->GetHealthComponent()->GetCurrentHealth(), false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitGateTest, "MothEffect.Feedback.EnemyHit.PauseAndClosedGameplayRejectDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitGateTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture F;
	if (!F.CheckReady(*this)) return false;
	auto* Enemy = F.SpawnEnemy();
	auto* Pauser = F.Combat.Spawn<APlayerState>();
	if (!TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("Actual world pauser"), Pauser)) return false;
	const float InitialHealth = Enemy->GetHealthComponent()->GetCurrentHealth();
	const FHitContext Hit = F.Hit();
	F.Combat.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
	TestTrue(TEXT("World is actually paused"), F.Combat.World->IsPaused());
	TestFalse(TEXT("Paused direct health damage is rejected"), Enemy->GetHealthComponent()->ApplyHit(Hit));
	F.Combat.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
	F.Session->SetGameplayEnabled(false);
	TestFalse(TEXT("Session has closed gameplay"), F.Session->IsGameplayEnabled());
	TestFalse(TEXT("Closed-session direct health damage is rejected"), Enemy->GetHealthComponent()->ApplyHit(Hit));
	F.CheckCounts(*this, 0, 0);
	TestEqual(TEXT("Rejected damage leaves health intact"), Enemy->GetHealthComponent()->GetCurrentHealth(), InitialHealth);
	F.Session->SetGameplayEnabled(true);
	TestTrue(TEXT("Reopening admits the previously rejected hit ID"), Enemy->GetHealthComponent()->ApplyHit(Hit));
	F.CheckCounts(*this, 1, 1);
	F.CheckResult(*this, 0, Enemy, Hit, Hit.Damage, InitialHealth - Hit.Damage, false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitDeathResetTest, "MothEffect.Feedback.EnemyHit.DeathCommitResetSuppressesStaleResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitDeathResetTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture F;
	if (!F.CheckReady(*this)) return false;
	auto* Enemy = F.SpawnEnemy();
	if (!TestNotNull(TEXT("Enemy"), Enemy)) return false;
	Enemy->GetHealthComponent()->OnHealthChanged.AddDynamic(F.BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleHealthChanged);
	Enemy->GetHealthComponent()->OnDied.AddDynamic(F.BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleDied);
	const uint64 Generation = F.Session->GetLifecycleGeneration();
	int32 DeathCommits = 0;
	const FDelegateHandle Handle = Enemy->GetHealthComponent()->OnDeathCommittedNative.AddLambda([&](AActor* Victim, AActor*)
	{
		++DeathCommits;
		TestTrue(TEXT("Death commit names the actual victim"), Victim == Enemy);
		TestFalse(TEXT("Health death has already committed"), Enemy->GetHealthComponent()->IsAlive());
		F.Session->ResetManagedObjects();
	});
	TestTrue(TEXT("Lethal damage remains accepted despite a synchronous reset"), Enemy->GetHealthComponent()->ApplyHit(F.Hit(1000.0f)));
	Enemy->GetHealthComponent()->OnDeathCommittedNative.Remove(Handle);
	Enemy->GetHealthComponent()->OnHealthChanged.RemoveDynamic(F.BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleHealthChanged);
	Enemy->GetHealthComponent()->OnDied.RemoveDynamic(F.BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleDied);
	TestEqual(TEXT("Exactly one committed death callback"), DeathCommits, 1);
	TestTrue(TEXT("Death observer advanced the session generation"), F.Session->GetLifecycleGeneration() != Generation);
	F.CheckCounts(*this, 0, 0);
	TestEqual(TEXT("Session reset still permits committed health notification"), F.BlueprintObserver->HealthNotifications, 1);
	TestEqual(TEXT("Session reset still permits committed death notification"), F.BlueprintObserver->DeathNotifications, 1);
	auto* FreshEnemy = F.SpawnEnemy(FVector(1500, 0, 100));
	if (!TestNotNull(TEXT("Fresh-generation enemy"), FreshEnemy)) return false;
	TestTrue(TEXT("New-generation damage can notify normally"), FreshEnemy->GetHealthComponent()->ApplyHit(F.Hit()));
	F.CheckCounts(*this, 1, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitNativeResetTest, "MothEffect.Feedback.EnemyHit.NativeObserverResetSuppressesLateBlueprintDelivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitNativeResetTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture F;
	if (!F.CheckReady(*this)) return false;
	auto* Enemy = F.SpawnEnemy();
	if (!TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("Existing feedback profile"), F.Feedback->GetProfile())) return false;
	TGuardValue<bool> EnablePresentation(F.Feedback->GetProfile()->bEnabled, true);
	TGuardValue<bool> DisableWorldVisuals(F.Feedback->GetProfile()->bWorldVisuals, false);
	TGuardValue<bool> DisableAudio(F.Feedback->GetProfile()->bAudio, false);
	const uint64 HitCueCount = F.Feedback->GetCueCount(EMothFeedbackCue::Hit);
	Enemy->GetHealthComponent()->OnHealthChanged.AddDynamic(F.BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleHealthChanged);
	Enemy->GetHealthComponent()->OnDied.AddDynamic(F.BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleDied);
	const FDelegateHandle Handle = F.Feedback->OnEnemyHitConfirmedNative.AddLambda([&](APawn*, const FMothEnemyHitFeedback&)
	{
		F.Session->ResetManagedObjects();
	});
	TestTrue(TEXT("Real lethal damage commits before the native observer resets"), Enemy->GetHealthComponent()->ApplyHit(F.Hit(1000.0f)));
	F.Feedback->OnEnemyHitConfirmedNative.Remove(Handle);
	Enemy->GetHealthComponent()->OnHealthChanged.RemoveDynamic(F.BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleHealthChanged);
	Enemy->GetHealthComponent()->OnDied.RemoveDynamic(F.BlueprintObserver.Get(), &UMothEnemyHitFeedbackTestObserver::HandleDied);
	F.CheckCounts(*this, 1, 0);
	TestEqual(TEXT("Old-generation Hit cue cannot reappear after native reset"), F.Feedback->GetCueCount(EMothFeedbackCue::Hit), HitCueCount);
	TestEqual(TEXT("Old-generation local hit marker remains cleared"), F.Feedback->GetHitMarkerAlpha(), 0.0f);
	TestEqual(TEXT("Native reset does not lose committed health notification"), F.BlueprintObserver->HealthNotifications, 1);
	TestEqual(TEXT("Native reset does not lose committed death notification"), F.BlueprintObserver->DeathNotifications, 1);
	TestEqual(TEXT("Health notification reports committed zero health"), F.BlueprintObserver->NotifiedHealth, 0.0f);
	auto* FreshEnemy = F.SpawnEnemy(FVector(1500, 0, 100));
	if (!TestNotNull(TEXT("Fresh enemy after native reset"), FreshEnemy)) return false;
	TestTrue(TEXT("Fresh damage after reset emits normally"), FreshEnemy->GetHealthComponent()->ApplyHit(F.Hit()));
	F.CheckCounts(*this, 2, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitClearTest, "MothEffect.Feedback.EnemyHit.PresentationClearAndDisablePreserveResults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitClearTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture F;
	if (!F.CheckReady(*this)) return false;
	auto* Enemy = F.SpawnEnemy();
	if (!TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("Existing feedback profile"), F.Feedback->GetProfile())) return false;
	const FDelegateHandle Handle = F.Feedback->OnEnemyHitConfirmedNative.AddLambda([&](APawn*, const FMothEnemyHitFeedback&)
	{
		F.Feedback->ClearFeedback();
	});
	const FHitContext FirstHit = F.Hit();
	TestTrue(TEXT("Real first hit commits"), Enemy->GetHealthComponent()->ApplyHit(FirstHit));
	F.CheckCounts(*this, 1, 1);
	{
		TGuardValue<bool> DisablePresentation(F.Feedback->GetProfile()->bEnabled, false);
		TestTrue(TEXT("New accepted hit notifies with default presentation disabled"), Enemy->GetHealthComponent()->ApplyHit(F.Hit()));
		F.CheckCounts(*this, 2, 2);
		TestFalse(TEXT("Presentation clearing cannot erase combat hit deduplication"), Enemy->GetHealthComponent()->ApplyHit(FirstHit));
		F.CheckCounts(*this, 2, 2);
	}
	F.Feedback->ClearFeedback();
	TestTrue(TEXT("Explicit clearing does not disconnect later results"), Enemy->GetHealthComponent()->ApplyHit(F.Hit()));
	F.CheckCounts(*this, 3, 3);
	F.Feedback->OnEnemyHitConfirmedNative.Remove(Handle);
	if (F.NativeResults.Num() == 3)
		TestTrue(TEXT("Already delivered hit snapshot survives clears and later damage"), F.NativeResults[0].HitId == FirstHit.HitId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyHitWorldIsolationTest, "MothEffect.Feedback.EnemyHit.LocalAttributionCannotCrossWorlds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyHitWorldIsolationTest::RunTest(const FString&)
{
	MothEnemyHitFeedbackTest::FFixture First;
	MothEnemyHitFeedbackTest::FFixture Second;
	if (!First.CheckReady(*this) || !Second.CheckReady(*this)) return false;
	auto* FirstEnemy = First.SpawnEnemy();
	auto* SecondEnemy = Second.SpawnEnemy();
	if (!TestNotNull(TEXT("First-world enemy"), FirstEnemy) || !TestNotNull(TEXT("Second-world enemy"), SecondEnemy)) return false;
	TestTrue(TEXT("First-world local damage commits"), FirstEnemy->GetHealthComponent()->ApplyHit(First.Hit()));
	First.CheckCounts(*this, 1, 1);
	Second.CheckCounts(*this, 0, 0);
	TestTrue(TEXT("Health accepts a foreign attribution context without forwarding UI"), SecondEnemy->GetHealthComponent()->ApplyHit(First.Hit()));
	First.CheckCounts(*this, 1, 1);
	Second.CheckCounts(*this, 0, 0);
	TestTrue(TEXT("Second world's own local damage notifies its own observer"), SecondEnemy->GetHealthComponent()->ApplyHit(Second.Hit()));
	First.CheckCounts(*this, 1, 1);
	Second.CheckCounts(*this, 1, 1);
	return true;
}
#endif
