#if WITH_DEV_AUTOMATION_TESTS

#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "Framework/MothEffectPlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	FHitContext LifecycleHit()
	{
		FHitContext Hit;
		Hit.HitId = FGuid::NewGuid();
		Hit.ShotDirection = FVector::ForwardVector;
		return Hit;
	}
	UDeviceLifecycleSubsystem* Manager(FMothCombatTestWorld& F) { return F.World->GetSubsystem<UDeviceLifecycleSubsystem>(); }
	ADeviceBase* NewDevice(FMothCombatTestWorld& F, FVector Position, EDeviceKind Kind = EDeviceKind::Emitter)
	{
		const FTransform Transform(FRotator::ZeroRotator, Position);
		ADeviceBase* D = F.World->SpawnActorDeferred<ADeviceBase>(ADeviceBase::StaticClass(), Transform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!D) return nullptr;
		D->ActivationDelaySeconds = 0.0f; // Isolate capacity and idle clocks from effect startup delay.
		FEnumProperty* P = FindFProperty<FEnumProperty>(ADeviceBase::StaticClass(), TEXT("DeviceKind"));
		P->GetUnderlyingProperty()->SetIntPropertyValue(P->ContainerPtrToValuePtr<void>(D), static_cast<int64>(Kind));
		D->FinishSpawning(Transform);
		F.AgeDeviceForActivation(D);
		D->GetPhysicsBody()->SetEnableGravity(false); // Isolate clocks/admission from unrelated free fall.
		return D;
	}
	AMothPlayerAutomationCharacter* NewHolder(FMothCombatTestWorld& F, FVector Position)
	{
		auto* P = F.Spawn<AMothPlayerAutomationCharacter>(Position);
		if (P) P->GetCharacterMovement()->GravityScale = 0.0f;
		return P;
	}
	ARuleProjectile* NewProjectile(FMothCombatTestWorld& F, FVector Position)
	{
		const FTransform T(FRotator::ZeroRotator, Position);
		auto* P = F.World->SpawnActorDeferred<ARuleProjectile>(ARuleProjectile::StaticClass(), T, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!P) return nullptr;
		if (!P->InitializeProjectile(FVector::UpVector, nullptr, nullptr)) { P->Destroy(); return nullptr; }
		P->FinishSpawning(T);
		return P;
	}
	void LifecycleFloor(FMothCombatTestWorld& F)
	{
		AActor* A = F.Spawn<AActor>();
		auto* B = NewObject<UBoxComponent>(A);
		A->AddInstanceComponent(B); A->SetRootComponent(B);
		B->SetBoxExtent(FVector(7000, 2000, 10));
		B->SetCollisionObjectType(ECC_WorldStatic);
		B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		B->SetCollisionResponseToAllChannels(ECR_Block);
		B->ComponentTags.Add(TEXT("MothColumnSurface"));
		B->RegisterComponent(); B->SetWorldLocation(FVector(3000, 0, -10));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothTotalRecycling, "MothEffect.Lifecycle.TotalRecyclesOldestDormant", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothTotalRecycling::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	auto* M = Manager(F);
	if (!TestNotNull(TEXT("Game world creates lifecycle subsystem"), M)) return false;
	auto* Holder = NewHolder(F, FVector(0, 0, 100));
	auto* Held = NewDevice(F, FVector(140, 0, 100));
	TestTrue(TEXT("Oldest device is actually picked up"), Holder->TryPickupDevice(Held));
	auto* Dormant = NewDevice(F, FVector(1000, 0, 100));
	auto* Active = NewDevice(F, FVector(1400, 0, 100));
	Active->TryActivate(LifecycleHit());
	for (int32 I = 3; I < 24; ++I) NewDevice(F, FVector(2000 + I * 100, 0, 100));
	TestEqual(TEXT("All states share 24 total slots"), M->GetDeviceCount(), 24);
	auto* Replacement = M->SpawnDevice(ADeviceBase::StaticClass(), FTransform(FVector(5000, 0, 100)));
	TestNotNull(TEXT("Drop entry makes room before spawning"), Replacement);
	TestTrue(TEXT("Earliest Dormant is reclaimed"), Dormant->GetDeviceState() == EDeviceState::Destroyed);
	TestTrue(TEXT("Earlier Held remains held"), Held->GetDeviceState() == EDeviceState::Held && Holder->GetHeldDevice() == Held);
	TestTrue(TEXT("Active remains active"), Active->GetDeviceState() == EDeviceState::Active);
	TestEqual(TEXT("No overshoot after replacement"), M->GetDeviceCount(), 24);
	// Direct placement/spawn cannot bypass registration either.
	F.Spawn<ADeviceBase>(FVector(6000, 0, 100));
	TestEqual(TEXT("Direct spawn also enforces the cap"), M->GetDeviceCount(), 24);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothProtectedFull, "MothEffect.Lifecycle.FullCapacityProtectsHeld", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothProtectedFull::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	for (int32 I = 0; I < 24; ++I)
	{
		const FVector P(I * 600, 0, 100);
		auto* Holder = NewHolder(F, P);
		auto* Device = NewDevice(F, P + FVector(140, 0, 0));
		TestTrue(TEXT("Each protected slot uses actual Held ownership"), Holder->TryPickupDevice(Device));
	}
	auto* M = Manager(F);
	TestEqual(TEXT("Twenty-four Held occupy the full budget"), M->GetDeviceCount(), 24);
	TestNull(TEXT("No Dormant means drop rejected"), M->SpawnDevice(ADeviceBase::StaticClass(), FTransform(FVector(20000, 0, 100))));
	TestEqual(TEXT("Rejection does not remove Held"), M->GetDeviceCount(), 24);
	M->ResetManagedObjects();
	TestEqual(TEXT("Reset frees all protected slots"), M->GetDeviceCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothActiveOrder, "MothEffect.Lifecycle.EmitterActivationOrderAndReentrancy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothActiveOrder::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	TArray<ADeviceBase*> D;
	for (int32 I = 0; I < 8; ++I) D.Add(NewDevice(F, FVector(I * 200, 0, 500)));
	// The oldest generated device is activated last. Retirement uses activation order.
	for (int32 I = 5; I >= 0; --I) TestTrue(TEXT("First six emitters activate"), D[I]->TryActivate(LifecycleHit()));
	bool bNestedActivation = true;
	D[5]->OnDeviceStateChangedNative.AddLambda([&](ADeviceBase*, EDeviceState, EDeviceState NewState)
	{
		if (NewState == EDeviceState::Spent) bNestedActivation = D[7]->TryActivate(LifecycleHit());
	});
	TestTrue(TEXT("Seventh safely replaces earliest activation"), D[6]->TryActivate(LifecycleHit()));
	TestFalse(TEXT("Retirement callback cannot steal the reserved slot"), bNestedActivation);
	TestTrue(TEXT("Earliest activation reaches Spent"), D[5]->GetDeviceState() == EDeviceState::Spent);
	TestTrue(TEXT("Earliest generation remains Active"), D[0]->GetDeviceState() == EDeviceState::Active);
	TestEqual(TEXT("Active cap never exceeds six"), Manager(F)->GetActiveCount(EDeviceKind::Emitter), 6);
	D[5]->OnDeviceStateChangedNative.Clear();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothProjectileAdmission, "MothEffect.Lifecycle.ProjectileAdmissionAndImmediateImpactRelease", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothProjectileAdmission::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	TArray<ARuleProjectile*> P;
	const int32 ProjectileLimit = Manager(F)->GetProjectileLimit();
	for (int32 I = 0; I < ProjectileLimit; ++I) P.Add(NewProjectile(F, FVector(I * 40, 0, 5000)));
	TestEqual(TEXT("Unified Pawn/device-independent budget admits the configured limit"), Manager(F)->GetProjectileCount(), ProjectileLimit);
	TestNull(TEXT("The projectile beyond the configured limit is rejected"), NewProjectile(F, FVector(0, 500, 5000)));
	AActor* Wall = F.Spawn<AActor>();
	FHitResult Hit(Wall, nullptr, FVector::ZeroVector, FVector::UpVector); Hit.bBlockingHit = true;
	int32 CountInImpactCallback = -1;
	P[0]->OnImpactNative.AddLambda([&](ARuleProjectile*, AActor*, const FHitContext&) { CountInImpactCallback = Manager(F)->GetProjectileCount(); });
	TestTrue(TEXT("Blocking consumption succeeds"), P[0]->ProcessBlockingHit(Hit));
	TestEqual(TEXT("Quota freed before impact observer runs"), CountInImpactCallback, ProjectileLimit - 1);
	TestNotNull(TEXT("Freed slot can be reused immediately"), NewProjectile(F, FVector(0, 500, 5000)));
	Manager(F)->ResetManagedObjects();
	TestEqual(TEXT("Cleanup is idempotent"), Manager(F)->GetProjectileCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothAdmissionReset, "MothEffect.Lifecycle.ResetInvalidatesInFlightDropAdmission", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothAdmissionReset::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	auto* M = Manager(F);
	auto* Oldest = NewDevice(F, FVector(0, 0, 500));
	for (int32 I = 1; I < 24; ++I) NewDevice(F, FVector(I * 100, 0, 500));
	Oldest->OnDeviceStateChangedNative.AddLambda([&](ADeviceBase*, EDeviceState, EDeviceState S)
	{
		if (S == EDeviceState::Destroyed) M->ResetManagedObjects();
	});
	TestNull(TEXT("A reset during retirement cancels the old pending drop"), M->SpawnDevice(ADeviceBase::StaticClass(), FTransform(FVector(3000, 0, 500))));
	TestEqual(TEXT("No drop survives the reset boundary"), M->GetDeviceCount(), 0);
	Oldest->OnDeviceStateChangedNative.Clear();
	TestNotNull(TEXT("A fresh request still succeeds"), M->SpawnDevice(ADeviceBase::StaticClass(), FTransform(FVector(3000, 0, 500))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDeferredSlot, "MothEffect.Lifecycle.DeferredProjectileCancellationReleasesQuota", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDeferredSlot::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	const FTransform Transform(FVector(0, 0, 500));
	auto* P = F.World->SpawnActorDeferred<ARuleProjectile>(ARuleProjectile::StaticClass(), Transform);
	TestTrue(TEXT("Deferred initialization reserves before BeginPlay"), P->InitializeProjectile(FVector::UpVector, nullptr, nullptr));
	TestEqual(TEXT("Reservation is already included"), Manager(F)->GetProjectileCount(), 1);
	P->Destroy();
	TestEqual(TEXT("Cancelling before FinishSpawning does not leak quota"), Manager(F)->GetProjectileCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothSavedLifecycle, "MothEffect.Lifecycle.SavedBlueprintsAndDebugController", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothSavedLifecycle::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	UClass* ModeClass = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/MothEffect/Framework/BP_MothGameMode.BP_MothGameMode_C"));
	if (!TestNotNull(TEXT("Saved game mode"), ModeClass)) return false;
	const auto* Mode = ModeClass->GetDefaultObject<AGameModeBase>();
	TestTrue(TEXT("Current player controller supports lifecycle debug commands"), Mode->PlayerControllerClass
		&& Mode->PlayerControllerClass->IsChildOf(AMothEffectPlayerController::StaticClass()));
	const TCHAR* Paths[] = {
		TEXT("/Game/MothEffect/Devices/BP_DeviceLauncher.BP_DeviceLauncher_C"),
		TEXT("/Game/MothEffect/Devices/BP_DeviceBomb.BP_DeviceBomb_C"),
		TEXT("/Game/MothEffect/Devices/BP_DeviceEmitter.BP_DeviceEmitter_C") };
	for (int32 I = 0; I < 3; ++I)
	{
		UClass* Class = LoadClass<ADeviceBase>(nullptr, Paths[I]);
		if (!TestNotNull(TEXT("Saved device class"), Class)) continue;
		auto* Device = Manager(F)->SpawnDevice(Class, FTransform(FVector(I * 500, 0, 500)));
		if (!TestNotNull(TEXT("Saved class uses admitted spawn"), Device)) continue;
		TestEqual(TEXT("Saved kind preserved"), static_cast<int32>(Device->GetDeviceKind()), I);
		TestNotNull(TEXT("Existing Blueprint inherits new warning component"), Device->GetDefaultSubobjectByName(TEXT("LifecycleFeedback")));
	}
	TestEqual(TEXT("Three saved device actors registered once"), Manager(F)->GetDeviceCount(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothResetIsolation, "MothEffect.Lifecycle.ResetRejectsCallbacksAndIsolatesWorlds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothResetIsolation::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	FMothCombatTestWorld Other;
	auto* M = Manager(F);
	auto* Active = NewDevice(F, FVector(1000, 0, 500)); Active->TryActivate(LifecycleHit());
	auto* Holder = NewHolder(F, FVector(0, 0, 100));
	auto* Held = NewDevice(F, FVector(140, 0, 100)); Holder->TryPickupDevice(Held);
	NewProjectile(F, FVector(0, 0, 1000));
	NewDevice(Other, FVector(0, 0, 100));
	bool bReplacementSpawned = true;
	Held->OnDeviceStateChangedNative.AddLambda([&](ADeviceBase*, EDeviceState, EDeviceState S)
	{
		if (S == EDeviceState::Destroyed) bReplacementSpawned = M->SpawnDevice(ADeviceBase::StaticClass(), FTransform::Identity) != nullptr;
	});
	M->ResetManagedObjects(); M->ResetManagedObjects();
	TestFalse(TEXT("Reset callbacks cannot create leftover objects"), bReplacementSpawned);
	TestEqual(TEXT("Total count reset"), M->GetDeviceCount(), 0);
	TestEqual(TEXT("Active count reset"), M->GetActiveCount(EDeviceKind::Emitter), 0);
	TestEqual(TEXT("Projectile count reset"), M->GetProjectileCount(), 0);
	TestNull(TEXT("Held owner reference cleared"), Holder->GetHeldDevice());
	TestEqual(TEXT("Other world unaffected"), Manager(Other)->GetDeviceCount(), 1);
	TestNotNull(TEXT("Fresh round admits objects after reset"), M->SpawnDevice(ADeviceBase::StaticClass(), FTransform(FVector(1000, 0, 100))));
	Held->OnDeviceStateChangedNative.Clear();
	return true;
}

namespace
{
	enum class ECase { Idle, Column, Saturation, Bounds, Disabled };
	class FLifecycleWorldTest final : public IAutomationLatentCommand
	{
	public:
		FLifecycleWorldTest(FAutomationTestBase* InTest, ECase InCase) : T(InTest), Case(InCase) {}
		bool Update() override
		{
			if (!F) return Setup();
			if (F->World->GetTimerManager().HasBeenTickedThisFrame()) return false;
			F->World->Tick(LEVELTICK_All, Case == ECase::Idle || Case == ECase::Disabled ? 0.1f : 0.01f);
			const double Now = F->GetElapsedTimeSeconds();
			if (Case == ECase::Idle) return Idle(Now);
			if (Case == ECase::Disabled)
			{
				if (Stage == 0 && Now >= 5.0) { D->SetGameplayEnabled(false); IdleBefore = D->GetDormantIdleSeconds(); Stage = 1; }
				else if (Stage == 1 && Now >= 40.0)
				{
					T->TestTrue(TEXT("Round-disabled Dormant clock freezes"), FMath::IsNearlyEqual(D->GetDormantIdleSeconds(), IdleBefore, 0.01f));
					D->SetGameplayEnabled(true); Stage = 2;
				}
				else if (Stage == 2 && Now >= 41.0)
				{
					T->TestTrue(TEXT("Re-enabled idle continues original accumulated age"), D->GetDormantIdleSeconds() > IdleBefore + 0.9f);
					return true;
				}
			}
			if (Case == ECase::Column && Now >= 0.25)
			{
				auto* OldColumn = Seeds[0]->GetLaunchColumn();
				T->TestTrue(TEXT("Old column is actual solid support"), OldColumn && OldColumn->IsSolidSupportEnabled());
				T->TestTrue(TEXT("Seventh launcher admitted"), Seeds[6]->TryActivate(LifecycleHit()));
				T->TestTrue(TEXT("Old column immediately retracts without solid support"), OldColumn && OldColumn->GetPhase() == ELaunchColumnPhase::Retracting && !OldColumn->IsSolidSupportEnabled());
				T->TestEqual(TEXT("Exactly six active seeds, column visual is not double-counted"), M->GetActiveCount(EDeviceKind::Launcher), 6);
				T->TestEqual(TEXT("Spent seed releases total slot immediately"), M->GetDeviceCount(), 6);
				M->ResetManagedObjects();
				T->TestTrue(TEXT("Reset destroys even already-retracting old column"), OldColumn->IsActorBeingDestroyed());
				return true;
			}
			if (Case == ECase::Saturation)
			{
				T->TestTrue(TEXT("Every actual frame obeys projectile cap"), M->GetProjectileCount() <= M->GetProjectileLimit());
				if (Stage == 0 && Now >= 0.4)
				{
					T->TestEqual(TEXT("Full cap consumes first two schedule slots"), D->GetEmitterShotAttempts(), 2);
					T->TestEqual(TEXT("No P01 born while full"), D->GetEmitterProjectilesSpawned(), 0);
					Projectiles[0]->Destroy(); Stage = 1;
				}
				else if (Stage == 1 && Now >= 0.65)
				{
					T->TestEqual(TEXT("Next normal slot resumes after quota release"), D->GetEmitterProjectilesSpawned(), 1); Stage = 2;
				}
				else if (Stage == 2 && Now >= 3.05)
				{
					T->TestTrue(TEXT("Saturation does not extend source duration"), D->GetDeviceState() == EDeviceState::Spent);
					T->TestEqual(TEXT("Original twelve scheduled attempts only"), D->GetEmitterShotAttempts(), 12); Stage = 3;
				}
				else if (Stage == 3 && Now >= 6.2)
				{
					T->TestEqual(TEXT("All projectile slots expire independently"), M->GetProjectileCount(), 0);
					T->TestEqual(TEXT("Spent visual finally destroyed"), M->GetDeviceCount(), 0); return true;
				}
			}
			if (Case == ECase::Bounds && Now >= 0.05)
			{
				T->TestEqual(TEXT("Out-of-arena objects release every slot"), M->GetDeviceCount(), 0);
				T->TestEqual(TEXT("Outside projectiles removed"), M->GetProjectileCount(), 0);
				T->TestNull(TEXT("Outside Held cleans holder reference"), Holder->GetHeldDevice());
				T->TestTrue(TEXT("Bounds teardown stops active effect"), D->GetDeviceState() == EDeviceState::Destroyed); return true;
			}
			if (Now > 100.0) { T->AddError(TEXT("Lifecycle fixture timed out")); return true; }
			return false;
		}
	private:
		FAutomationTestBase* T;
		ECase Case;
		TUniquePtr<FMothCombatTestWorld> F;
		UDeviceLifecycleSubsystem* M = nullptr;
		ADeviceBase* D = nullptr;
		AMothPlayerAutomationCharacter* Holder = nullptr;
		TArray<ADeviceBase*> Seeds;
		TArray<ARuleProjectile*> Projectiles;
		int32 Stage = 0;
		int32 PausedFrames = 0;
		float IdleBefore = 0.0f;
		double ChangedAt = 0.0;
		bool Setup()
		{
			F = MakeUnique<FMothCombatTestWorld>();
			M = Manager(*F);
			if (!T->TestNotNull(TEXT("Lifecycle subsystem"), M)) return true;
			if (Case == ECase::Column)
			{
				LifecycleFloor(*F);
				for (int32 I = 0; I < 7; ++I)
				{
					Seeds.Add(NewDevice(*F, FVector(I * 700, 0, 20), EDeviceKind::Launcher));
					if (I < 6) Seeds.Last()->TryActivate(LifecycleHit());
				}
			}
			else if (Case == ECase::Saturation)
			{
				for (int32 I = 0; I < M->GetProjectileLimit(); ++I) Projectiles.Add(NewProjectile(*F, FVector(-9000, I * 40, 5000)));
				D = NewDevice(*F, FVector(0, 0, 500)); D->TryActivate(LifecycleHit());
			}
			else
			{
				Holder = NewHolder(*F, FVector(0, 0, 100));
				D = NewDevice(*F, FVector(140, 0, 100));
				if (Case == ECase::Idle)
					D->OnDeviceStateChangedNative.AddLambda([this](ADeviceBase* Device, EDeviceState, EDeviceState NewState)
					{
						if (NewState == EDeviceState::Spent)
						{
							T->TestEqual(TEXT("Dormant expiry frees quota before visual cleanup/event"), M->GetDeviceCount(), 0);
							T->TestTrue(TEXT("Expiry callback sees collision already disabled"), Device->GetPhysicsBody()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
						}
					});
				if (Case == ECase::Bounds)
				{
					Holder->TryPickupDevice(D);
					D = NewDevice(*F, FVector(1000, 0, 500)); D->TryActivate(LifecycleHit());
					NewProjectile(*F, FVector(1200, 0, 500));
					T->TestFalse(TEXT("Invalid arena does not install"), M->SetSafetyBounds(FVector::ZeroVector, FVector(-1)));
					M->SetSafetyBounds(FVector(5000, 0, 5000), FVector(100));
				}
			}
			return false;
		}
		bool Idle(double Now)
		{
			if (Stage == 0 && Now >= 25.1)
			{
				T->TestTrue(TEXT("Last five idle seconds show warning"), D->IsExpiryWarningActive());
				T->TestTrue(TEXT("Actual pickup pauses original clock"), Holder->TryPickupDevice(D));
				T->TestFalse(TEXT("Warning is hidden while Held"), D->IsExpiryWarningActive());
				IdleBefore = D->GetDormantIdleSeconds(); ChangedAt = Now; Stage = 1;
			}
			else if (Stage == 1 && Now - ChangedAt >= 40.0)
			{
				T->TestTrue(TEXT("Held survives beyond full loose lifetime"), D->GetDeviceState() == EDeviceState::Held);
				T->TestTrue(TEXT("Forty held seconds do not add idle age"), FMath::IsNearlyEqual(D->GetDormantIdleSeconds(), IdleBefore, 0.01f));
				auto* Pauser = F->Spawn<APlayerState>(); F->World->GetWorldSettings()->SetPauserPlayerState(Pauser);
				ChangedAt = Now; PausedFrames = 10; Stage = 2;
			}
			else if (Stage == 2)
			{
				T->TestTrue(TEXT("Actual world pause freezes clock"), FMath::IsNearlyEqual(Now, ChangedAt, 0.001));
				if (--PausedFrames == 0)
				{
					F->World->GetWorldSettings()->SetPauserPlayerState(nullptr);
					T->TestTrue(TEXT("Actual drop resumes instead of refreshing"), Holder->TryReleaseHeldDevice(false));
					D->GetPhysicsBody()->SetEnableGravity(false); ChangedAt = Now; Stage = 3;
				}
			}
			else if (Stage == 3 && Now - ChangedAt >= 1.0)
			{
				T->TestTrue(TEXT("Drop continued accumulated age"), D->GetDormantIdleSeconds() > IdleBefore + 0.9f);
				T->TestTrue(TEXT("Warning remains after drop"), D->IsExpiryWarningActive());
				T->TestTrue(TEXT("Warning-stage pickup succeeds again"), Holder->TryPickupDevice(D));
				ChangedAt = Now; Stage = 4;
			}
			else if (Stage == 4 && Now - ChangedAt >= 1.0)
			{
				T->TestTrue(TEXT("Actual throw keeps accumulated idle budget"), Holder->TryReleaseHeldDevice(true));
				D->GetPhysicsBody()->SetEnableGravity(false); ChangedAt = Now; Stage = 5;
			}
			else if (Stage == 5 && Now - ChangedAt >= 4.4)
			{
				T->TestTrue(TEXT("Thirty cumulative idle seconds ultimately destroy loose device"), D->GetDeviceState() == EDeviceState::Destroyed);
				T->TestEqual(TEXT("Expired device releases total quota"), M->GetDeviceCount(), 0); return true;
			}
			return false;
		}
	};
}

#define MOTH_LIFECYCLE_LATENT(ClassName, TestName, CaseName) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool ClassName::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(FLifecycleWorldTest(this, ECase::CaseName)); return true; }

MOTH_LIFECYCLE_LATENT(FMothIdleLifecycle, "MothEffect.Lifecycle.IdleWarningHeldDropThrowAndPause", Idle)
MOTH_LIFECYCLE_LATENT(FMothColumnLimit, "MothEffect.Lifecycle.ColumnLimitRetractsAndClosesSupport", Column)
MOTH_LIFECYCLE_LATENT(FMothProjectileSaturation, "MothEffect.Lifecycle.SaturatedEmitterConsumesSlotsAndResumes", Saturation)
MOTH_LIFECYCLE_LATENT(FMothOutsideCleanup, "MothEffect.Lifecycle.BoundsCleansHeldActiveAndProjectile", Bounds)
MOTH_LIFECYCLE_LATENT(FMothDisabledClock, "MothEffect.Lifecycle.DisabledIdleClockResumes", Disabled)

#undef MOTH_LIFECYCLE_LATENT
#endif
