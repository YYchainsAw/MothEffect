#if WITH_DEV_AUTOMATION_TESTS
#include "AI/EnemyBase.h"
#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Devices/DeviceBase.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "Weapons/Rifle.h"

namespace MothEnemyTest
{
	AEnemyBase* Shooter(FMothCombatTestWorld& F, FVector Position = FVector(0, 0, 90))
	{
		auto* E = F.Spawn<AEnemyBase>(Position);
		E->GetCharacterMovement()->DisableMovement(); // Only isolate timing/sweeps from floor/navigation here.
		return E;
	}
	AMothPlayerAutomationCharacter* Target(FMothCombatTestWorld& F, FVector Position = FVector(1000, 0, 90))
	{
		const FTransform T(Position);
		auto* P = F.World->SpawnActorDeferred<AMothPlayerAutomationCharacter>(AMothPlayerAutomationCharacter::StaticClass(), T, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		P->FindComponentByClass<UHealthComponent>()->ConfigureMaxHealth(1000);
		P->FinishSpawning(T);
		return P;
	}
	AActor* Cover(FMothCombatTestWorld& F, FVector Position, FVector Extent)
	{
		auto* A = F.Spawn<AActor>();
		auto* B = NewObject<UBoxComponent>(A);
		A->AddInstanceComponent(B); A->SetRootComponent(B);
		B->SetBoxExtent(Extent); B->SetCollisionObjectType(ECC_WorldStatic);
		B->SetCollisionResponseToAllChannels(ECR_Block); B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		B->RegisterComponent(); B->SetWorldLocation(Position);
		return A;
	}
	FHitContext Damage(float Amount)
	{
		FHitContext H; H.HitId = FGuid::NewGuid(); H.Damage = Amount; H.ShotDirection = FVector::ForwardVector; return H;
	}
	enum class ECase { Cadence, Occlusion, Cancel, Saturation, Muzzle, Device, Friendly, Rifle, Pause };
	class FEnemyFlight final : public IAutomationLatentCommand
	{
	public:
		FEnemyFlight(FAutomationTestBase* InTest, ECase InCase) : T(InTest), Case(InCase) {}
		bool Update() override
		{
			if (!F) return Setup();
			if (F->World->GetTimerManager().HasBeenTickedThisFrame()) return false;
			F->World->Tick(LEVELTICK_All, 0.01f);
			const double Now = F->World->GetTimeSeconds();
			if (Case == ECase::Friendly)
			{
				if (Now < 0.6) return false;
				T->TestEqual(TEXT("Actual swept P01 damages another enemy without allegiance immunity"), Victim->GetHealthComponent()->GetCurrentHealth(), 50.0f);
				T->TestEqual(TEXT("Only spawning enemy is immune"), E->GetHealthComponent()->GetCurrentHealth(), 60.0f);
				return true;
			}
			if (Case == ECase::Rifle)
			{
				if (Now < 0.65) return false;
				P->GetRifle()->StopFire();
				T->TestEqual(TEXT("Three real W01 hits kill the 60HP enemy"), E->GetHealthComponent()->GetCurrentHealth(), 0.0f);
				T->TestEqual(TEXT("Death event commits once"), Deaths, 1);
				T->TestTrue(TEXT("Death immediately closes gameplay/collision"), !E->IsCombatEnabled() && E->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
				E->OnEnemyDiedNative.Clear(); return true;
			}
			if (Case == ECase::Cadence)
			{
				if (E->GetProjectilesSpawned() != SeenShots) { T->TestEqual(TEXT("No catch-up burst per tick"), E->GetProjectilesSpawned() - SeenShots, 1); SeenShots = E->GetProjectilesSpawned(); ShotTimes.Add(Now); }
				if (Now < 0.45) T->TestEqual(TEXT("Full first telegraph precedes any shot"), SeenShots, 0);
				if (Now < 3.2) return false;
				T->TestEqual(TEXT("Plan fires three shots by 3.2s"), SeenShots, 3);
				for (int32 I = 0; I < ShotTimes.Num(); ++I) T->TestTrue(TEXT("Configured 0.5s warning / >=1.2s shot cadence"), I == 0 ? ShotTimes[I] >= 0.5 : ShotTimes[I] - ShotTimes[I - 1] >= 1.19);
				T->TestTrue(TEXT("AI-born P01 actually sweeps into player's health"), P->FindComponentByClass<UHealthComponent>()->GetCurrentHealth() <= 980.0f);
				return true;
			}
			if (Case == ECase::Occlusion)
			{
				if (Stage == 0 && Now >= 0.25) { Wall = Cover(*F, FVector(500, 0, 150), FVector(20, 150, 200)); Stage = 1; }
				else if (Stage == 1 && Now >= 1.2)
				{ T->TestFalse(TEXT("Cover interrupts target sight"), E->HasTargetSight()); T->TestEqual(TEXT("Interrupted warning never shoots through wall"), E->GetProjectilesSpawned(), 0); Wall->Destroy(); ReleasedAt = Now; Stage = 2; }
				else if (Stage == 2)
				{
					if (Now < ReleasedAt + 0.49) T->TestEqual(TEXT("Reacquisition requires a fresh complete warning"), E->GetShotAttempts(), 0);
					if (Now >= ReleasedAt + 0.65) { T->TestEqual(TEXT("Fresh warning restores exactly one normal shot"), E->GetProjectilesSpawned(), 1); P->SetActorLocation(FVector(3500, 0, 90)); Stage = 3; ReleasedAt = Now; }
				}
				else if (Stage == 3 && Now >= ReleasedAt + 1.5) { T->TestEqual(TEXT("Out of sight/attack range prevents further shots"), E->GetProjectilesSpawned(), 1); return true; }
				return false;
			}
			if (Case == ECase::Cancel)
			{
				if (Stage == 0 && Now >= 0.7) { T->TestEqual(TEXT("Telegraph callback disable prevents firing"), E->GetShotAttempts(), 0); E->OnTelegraphChangedNative.Clear(); E->SetGameplayEnabled(true); ReleasedAt = Now; Stage = 1; }
				else if (Stage == 1 && Now >= ReleasedAt + 0.2) { E->SetCombatTarget(nullptr); Stage = 2; }
				else if (Stage == 2 && Now >= ReleasedAt + 1.0) { T->TestEqual(TEXT("Target cleared mid-warning cancels pending fire"), E->GetShotAttempts(), 0); E->SetCombatTarget(P); ReleasedAt = Now; Stage = 3; }
				else if (Stage == 3 && Now >= ReleasedAt + 0.2) { IBallisticReactive::Execute_ReceiveBallisticHit(E, Damage(60)); Stage = 4; }
				else if (Stage == 4 && Now >= ReleasedAt + 0.7) { T->TestEqual(TEXT("Death mid-warning cancels all future fire"), E->GetShotAttempts(), 0); return true; }
				return false;
			}
			if (Case == ECase::Saturation)
			{
				if (Stage == 0 && Now >= 0.65) { T->TestEqual(TEXT("Full P01 quota consumes one planned attempt"), E->GetShotAttempts(), 1); T->TestEqual(TEXT("No projectile born while full"), E->GetProjectilesSpawned(), 0); Budget[0]->Destroy(); Stage = 1; }
				else if (Stage == 1 && Now >= 1.9) { T->TestEqual(TEXT("Only next normal slot recovers, no make-up shot"), E->GetShotAttempts(), 2); T->TestEqual(TEXT("Freed global quota admits one shot"), E->GetProjectilesSpawned(), 1); return true; }
				return false;
			}
			if (Case == ECase::Muzzle)
			{
				if (Now < 0.7) return false;
				T->TestTrue(TEXT("Eye ray remains clear beside small muzzle cover"), E->HasTargetSight());
				T->TestEqual(TEXT("Muzzle sweep consumes the planned attempt"), E->GetShotAttempts(), 1);
				T->TestEqual(TEXT("Sphere birth cannot cross/overlap adjacent cover"), E->GetProjectilesSpawned(), 0); return true;
			}
			if (Case == ECase::Device)
			{
				// A normal device enters the already-fired projectile's path. While in front
				// of the target it also blocks sight, so it must not be present before firing.
				if (!D && E->GetProjectilesSpawned() > 0) { D = F->Spawn<ADeviceBase>(FVector(450, 0, 151)); D->GetPhysicsBody()->SetEnableGravity(false); }
				if (Now < 1.0) return false;
				if (!T->TestNotNull(TEXT("Normal AI shot permits a device to enter its path"), D)) return true;
				T->TestTrue(TEXT("Real enemy P01 activates ordinary Dormant D03"), D->GetDeviceState() == EDeviceState::Active);
				T->TestTrue(TEXT("Activation carries a real projectile hit ID and flight direction"), D->GetActivationHitId().IsValid() && D->GetActivationDirection().X > 0.99); return true;
			}
			if (Case == ECase::Pause)
			{
				if (Stage == 0 && Now >= 0.25) { F->World->GetWorldSettings()->SetPauserPlayerState(F->Spawn<APlayerState>()); PausedAt = Now; Stage = 1; }
				else if (Stage == 1)
				{
					T->TestEqual(TEXT("Real World pause freezes gameplay time"), Now, PausedAt);
					T->TestEqual(TEXT("No shot during paused warning"), E->GetShotAttempts(), 0);
					if (++PausedFrames >= 20) { F->World->GetWorldSettings()->SetPauserPlayerState(nullptr); Stage = 2; }
				}
				else if (Stage == 2 && Now >= 0.7) { T->TestEqual(TEXT("Warning continues after unpause"), E->GetProjectilesSpawned(), 1); return true; }
				return false;
			}
			return true;
		}
	private:
		bool Setup()
		{
			F = MakeUnique<FMothCombatTestWorld>();
			if (!T->TestNotNull(TEXT("Create enemy test world"), F->World)) return true;
			E = Shooter(*F); P = Target(*F);
			E->SetCombatTarget(P);
			if (Case == ECase::Cancel) E->OnTelegraphChangedNative.AddLambda([](AEnemyBase* Enemy, bool bVisible) { if (bVisible) Enemy->SetGameplayEnabled(false); });
			if (Case == ECase::Muzzle) Cover(*F, FVector(20, 6, 150), FVector(5, 3, 40));
			if (Case == ECase::Saturation)
			{
				for (int32 I = 0; I < 64; ++I)
				{
					const FTransform Transform(FVector(I * 50, 5000, 5000));
					auto* Shot = F->World->SpawnActorDeferred<ARuleProjectile>(ARuleProjectile::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					T->TestTrue(TEXT("Reserve filler in actual shared subsystem"), Shot->InitializeProjectile(FVector::UpVector, nullptr, nullptr)); Shot->FinishSpawning(Transform); Budget.Add(Shot);
				}
			}
			if (Case == ECase::Friendly)
			{
				E->SetGameplayEnabled(false); Victim = Shooter(*F, FVector(500, 0, 90)); Victim->SetCombatTarget(nullptr);
				const FTransform Transform(FVector(0, 0, 150));
				auto* Shot = F->World->SpawnActorDeferred<ARuleProjectile>(ARuleProjectile::StaticClass(), Transform, E, E, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				Shot->InitializeProjectile(FVector::ForwardVector, E, E); Shot->FinishSpawning(Transform);
			}
			if (Case == ECase::Rifle)
			{
				E->SetActorLocation(FVector(1000, 0, 90)); E->SetGameplayEnabled(true); E->SetCombatTarget(nullptr);
				P->SetActorLocation(FVector(0, 0, 90)); P->SetActorRotation(FRotator::ZeroRotator);
				E->OnEnemyDiedNative.AddLambda([this](AEnemyBase*, AActor*) { ++Deaths; });
				auto* Source = NewObject<USceneComponent>(P); P->AddInstanceComponent(Source); Source->RegisterComponent(); Source->SetWorldLocation(FVector(60, 0, 150));
				P->GetRifle()->SetMuzzleSource(Source, NAME_None);
				P->SetPrimaryDown(true); P->DoPrimaryStart();
				T->TestEqual(TEXT("Normal player press fires integrated W01"), P->GetRifle()->GetAmmoInMagazine(), 23);
			}
			return false;
		}
		FAutomationTestBase* T; ECase Case;
		TUniquePtr<FMothCombatTestWorld> F;
		AEnemyBase* E = nullptr; AEnemyBase* Victim = nullptr;
		AMothPlayerAutomationCharacter* P = nullptr;
		AActor* Wall = nullptr; ADeviceBase* D = nullptr;
		TArray<ARuleProjectile*> Budget;
		TArray<double> ShotTimes;
		int32 Stage = 0, SeenShots = 0, Deaths = 0, PausedFrames = 0;
		double ReleasedAt = 0, PausedAt = 0;
	};
}

#define MOTH_ENEMY_TEST(Class, Suffix, Case) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(Class, "MothEffect.Enemies." Suffix, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool Class::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothEnemyTest::FEnemyFlight(this, MothEnemyTest::ECase::Case)); return true; }
MOTH_ENEMY_TEST(FMothEnemyCadence, "TelegraphCadenceAndRealPlayerImpact", Cadence)
MOTH_ENEMY_TEST(FMothEnemyOcclusion, "CoverCancelsWarningAndReacquires", Occlusion)
MOTH_ENEMY_TEST(FMothEnemyCancellation, "CallbackDisableTargetChangeAndDeathCancel", Cancel)
MOTH_ENEMY_TEST(FMothEnemyQuota, "SharedProjectileQuotaAndNormalSlotRecovery", Saturation)
MOTH_ENEMY_TEST(FMothEnemyMuzzle, "EyeSightDoesNotBypassMuzzleCover", Muzzle)
MOTH_ENEMY_TEST(FMothEnemyDevice, "RealP01ActivatesDormantDevice", Device)
MOTH_ENEMY_TEST(FMothEnemyFriendly, "RealP01FriendlyDamageAndSourceExclusion", Friendly)
MOTH_ENEMY_TEST(FMothEnemyRifle, "NormalRifleKillsOnce", Rifle)
MOTH_ENEMY_TEST(FMothEnemyPause, "ActualWorldPauseFreezesWarning", Pause)
#undef MOTH_ENEMY_TEST

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyDeathAndDefaults, "MothEffect.Enemies.DeathIdempotenceConfigAndBomb", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyDeathAndDefaults::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	auto* E = MothEnemyTest::Shooter(F); auto* P = MothEnemyTest::Target(F);
	TestEqual(TEXT("Enemy archetype uses baseline 60HP"), E->GetHealthComponent()->GetMaxHealth(), 60.0f);
	TestFalse(TEXT("Health cannot be reset by configuring a begun actor"), E->GetHealthComponent()->ConfigureMaxHealth(999));
	TestFalse(TEXT("Self cannot become a combat target"), E->SetCombatTarget(E));
	FMothCombatTestWorld Other;
	TestFalse(TEXT("Cross-world target is rejected"), E->SetCombatTarget(MothEnemyTest::Target(Other)));
	int32 Deaths = 0;
	E->OnEnemyDiedNative.AddLambda([&](AEnemyBase* Victim, AActor*)
	{
		++Deaths; TestTrue(TEXT("Death state and collision commit before callbacks"), Victim->GetCombatPhase() == EEnemyCombatPhase::Dead && !Victim->IsCombatEnabled());
		TestFalse(TEXT("Reentrant damage cannot kill twice"), IBallisticReactive::Execute_ReceiveBallisticHit(Victim, MothEnemyTest::Damage(60)));
		Victim->SetGameplayEnabled(true); TestFalse(TEXT("Death cannot be enabled again"), Victim->IsCombatEnabled());
	});
	const FTransform Transform(FVector(100, 0, 50));
	auto* Bomb = F.World->SpawnActorDeferred<ADeviceBase>(ADeviceBase::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	auto* Kind = FindFProperty<FEnumProperty>(ADeviceBase::StaticClass(), TEXT("DeviceKind"));
	Kind->GetUnderlyingProperty()->SetIntPropertyValue(Kind->ContainerPtrToValuePtr<void>(Bomb), static_cast<int64>(EDeviceKind::Bomb));
	Bomb->FinishSpawning(Transform);
	TestTrue(TEXT("Actual D02 effect can kill the enemy through shared health"), Bomb->TryActivate(MothEnemyTest::Damage(0)));
	TestEqual(TEXT("Bomb causes one death"), Deaths, 1);
	E->OnEnemyDiedNative.Clear();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemySavedBlueprint, "MothEffect.Enemies.SavedBlueprintAndRange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemySavedBlueprint::RunTest(const FString&)
{
	UClass* Class = LoadClass<AEnemyBase>(nullptr, TEXT("/Game/MothEffect/Enemies/BP_EnemyBase.BP_EnemyBase_C"));
	if (!TestNotNull(TEXT("Reload saved enemy Blueprint"), Class)) return false;
	FMothCombatTestWorld F;
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* E = F.World->SpawnActor<AEnemyBase>(Class, FVector(0, 0, 90), FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Spawn actual saved enemy class"), E)) return false;
	TestEqual(TEXT("Saved blueprint still initializes 60HP"), E->GetHealthComponent()->GetMaxHealth(), 60.0f);
	auto* Property = FindFProperty<FObjectPropertyBase>(AEnemyBase::StaticClass(), TEXT("ProjectileClass"));
	TestEqual(TEXT("Saved enemy references formal visible P01 Blueprint"), Property->GetObjectPropertyValue_InContainer(E),
		static_cast<UObject*>(LoadClass<ARuleProjectile>(nullptr, TEXT("/Game/MothEffect/Combat/BP_RuleProjectile.BP_RuleProjectile_C"))));
	TestNotNull(TEXT("Saved enemy inherits telegraph component"), E->FindComponentByClass<UTextRenderComponent>());
	TestNotNull(TEXT("Saved T14 test map reloads"), LoadObject<UWorld>(nullptr, TEXT("/Game/MothEffect/Tests/L_T14AI.L_T14AI")));
	return true;
}
#endif
