#if WITH_DEV_AUTOMATION_TESTS

#include "Devices/DeviceBase.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/URL.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	struct FDeviceTestWorld
	{
		UWorld* World = nullptr;

		FDeviceTestWorld()
		{
			if (!GEngine)
			{
				return;
			}
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (World)
			{
				GEngine->CreateNewWorldContext(World->WorldType).SetCurrentWorld(World);
				World->SetShouldTick(false);
				// Interface Execute calls go through Actor::ProcessEvent, which requires
				// world-level actor initialization as well as the actor's own BeginPlay.
				World->InitializeActorsForPlay(FURL());
			}
		}

		~FDeviceTestWorld()
		{
			if (World)
			{
				// Actors begin play individually in these tests, without a game mode.
				World->BeginTearingDown();
				for (FActorIterator It(World); It; ++It)
				{
					It->RouteEndPlay(EEndPlayReason::Quit);
				}
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};

	FHitContext MakeDeviceHit()
	{
		FHitContext Hit;
		Hit.HitId = FGuid::NewGuid();
		Hit.ImpactPoint = FVector(10.0, 20.0, 30.0);
		Hit.ShotDirection = FVector(0.0, 2.0, 0.0);
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDeviceActivationTest,
	"MothEffect.Devices.ActivationIsSingleUse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothDeviceActivationTest::RunTest(const FString& Parameters)
{
	FDeviceTestWorld Fixture;
	ADeviceBase* Device = Fixture.World ? Fixture.World->SpawnActor<ADeviceBase>() : nullptr;
	if (!TestNotNull(TEXT("Create a device in an isolated world"), Device))
	{
		return false;
	}
	if (!TestTrue(TEXT("Test world initializes actors for reflected interface calls"),
		Fixture.World->AreActorsInitialized()))
	{
		return false;
	}
	const FHitContext FirstHit = MakeDeviceHit();
	TestFalse(TEXT("No activation before BeginPlay"), Device->TryActivate(FirstHit));
	Device->DispatchBeginPlay();
	TestTrue(TEXT("Dormant exposes device-side pickup eligibility"), Device->CanBePickedUp());
	int32 ActiveTransitions = 0;
	int32 SpentTransitions = 0;
	int32 DestroyedTransitions = 0;
	const FDelegateHandle StateObserver = Device->OnDeviceStateChangedNative.AddLambda(
		[this, &FirstHit, &ActiveTransitions, &SpentTransitions, &DestroyedTransitions]
		(ADeviceBase* ChangedDevice, EDeviceState OldState, EDeviceState NewState)
		{
			if (NewState == EDeviceState::Active)
			{
				++ActiveTransitions;
				TestFalse(TEXT("A hit re-entering the state callback cannot activate twice"),
					ChangedDevice->TryActivate(FirstHit));
			}
			SpentTransitions += NewState == EDeviceState::Spent;
			DestroyedTransitions += NewState == EDeviceState::Destroyed;
		});
	if (!TestTrue(TEXT("Ballistic interface accepts a zero-damage activation"),
		IBallisticReactive::Execute_ReceiveBallisticHit(Device, FirstHit)))
	{
		Device->OnDeviceStateChangedNative.Remove(StateObserver);
		return false;
	}
	TestTrue(TEXT("First hit commits Active"), Device->GetDeviceState() == EDeviceState::Active);
	TestFalse(TEXT("Active cannot be picked up"), Device->CanBePickedUp());
	TestTrue(TEXT("Direction is normalized in world space"),
		Device->GetActivationDirection().Equals(FVector::RightVector));
	TestFalse(TEXT("Replay of the same hit is rejected"), Device->TryActivate(FirstHit));
	FHitContext LaterHit = MakeDeviceHit();
	LaterHit.ShotDirection = FVector::ForwardVector;
	TestFalse(TEXT("A different hit cannot restart Active"), Device->TryActivate(LaterHit));
	TestTrue(TEXT("Original hit ID stays locked"), Device->GetActivationHitId() == FirstHit.HitId);
	TestTrue(TEXT("Original direction stays locked"),
		Device->GetActivationDirection().Equals(FVector::RightVector));
	TestTrue(TEXT("Original impact point stays locked"),
		Device->GetActivationImpactPoint().Equals(FirstHit.ImpactPoint));
	TestEqual(TEXT("Repeated and re-entrant hits emit one activation"), ActiveTransitions, 1);
	TestTrue(TEXT("Active device keeps simulating physics"), Device->GetPhysicsBody()->IsSimulatingPhysics());
	TestTrue(TEXT("First completion commits Spent"), Device->FinishActivation());
	TestFalse(TEXT("Completion is idempotent"), Device->FinishActivation());
	TestFalse(TEXT("Spent cannot activate"), Device->TryActivate(LaterHit));
	TestFalse(TEXT("Spent cannot be picked up"), Device->CanBePickedUp());
	TestTrue(TEXT("Spent has no physical collision"),
		Device->GetPhysicsBody()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	Device->DestroyDevice();
	Device->DestroyDevice();
	TestTrue(TEXT("Destruction commits the terminal state"), Device->GetDeviceState() == EDeviceState::Destroyed);
	TestFalse(TEXT("Destroyed cannot activate"), Device->TryActivate(FirstHit));
	TestEqual(TEXT("Completion emits one Spent transition"), SpentTransitions, 1);
	TestEqual(TEXT("Repeated destruction emits one terminal transition"), DestroyedTransitions, 1);
	Device->OnDeviceStateChangedNative.Remove(StateObserver);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDeviceInvalidHitTest,
	"MothEffect.Devices.RejectInvalidAndDisabledHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothDeviceInvalidHitTest::RunTest(const FString& Parameters)
{
	FDeviceTestWorld Fixture;
	ADeviceBase* Device = Fixture.World ? Fixture.World->SpawnActor<ADeviceBase>() : nullptr;
	if (!TestNotNull(TEXT("Create a device in an isolated world"), Device))
	{
		return false;
	}
	Device->DispatchBeginPlay();
	FHitContext InvalidHit = MakeDeviceHit();
	InvalidHit.HitId.Invalidate();
	TestFalse(TEXT("Invalid hit ID cannot activate"), Device->TryActivate(InvalidHit));
	InvalidHit = MakeDeviceHit();
	InvalidHit.ShotDirection = FVector::ZeroVector;
	TestFalse(TEXT("Zero trajectory cannot activate"), Device->TryActivate(InvalidHit));
	InvalidHit = MakeDeviceHit();
	InvalidHit.ImpactPoint.X = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("Non-finite impact point cannot activate"), Device->TryActivate(InvalidHit));
	InvalidHit = MakeDeviceHit();
	InvalidHit.ShotDirection.Y = std::numeric_limits<double>::infinity();
	TestFalse(TEXT("Non-finite trajectory cannot activate"), Device->TryActivate(InvalidHit));
	InvalidHit = MakeDeviceHit();
	InvalidHit.Damage = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Non-finite damage cannot activate"), Device->TryActivate(InvalidHit));
	InvalidHit = MakeDeviceHit();
	InvalidHit.Damage = -1.0f;
	TestFalse(TEXT("Negative damage cannot activate"), Device->TryActivate(InvalidHit));
	TestTrue(TEXT("Rejected hits leave Dormant unchanged"), Device->GetDeviceState() == EDeviceState::Dormant);
	const FHitContext ValidHit = MakeDeviceHit();
	APlayerState* Pauser = Fixture.World->SpawnActor<APlayerState>();
	if (!TestNotNull(TEXT("Create a pause owner"), Pauser))
	{
		return false;
	}
	Fixture.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
	TestFalse(TEXT("Paused world rejects activation"), Device->TryActivate(ValidHit));
	TestFalse(TEXT("Paused world rejects pickup eligibility"), Device->CanBePickedUp());
	Fixture.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
	Device->SetGameplayEnabled(false);
	TestFalse(TEXT("Disabled gameplay rejects an otherwise valid hit"), Device->TryActivate(ValidHit));
	Device->SetGameplayEnabled(true);
	TestTrue(TEXT("Rejected attempts did not consume activation"), Device->TryActivate(ValidHit));
	Device->SetGameplayEnabled(false);
	TestTrue(TEXT("Disabling an active effect leaves Spent"), Device->GetDeviceState() == EDeviceState::Spent);
	Device->SetGameplayEnabled(true);
	TestFalse(TEXT("Re-enabling gameplay does not reset a spent device"), Device->TryActivate(ValidHit));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDeviceCancelledActivationTest,
	"MothEffect.Devices.CancelDuringActivationCallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothDeviceCancelledActivationTest::RunTest(const FString& Parameters)
{
	FDeviceTestWorld Fixture;
	ADeviceBase* Device = Fixture.World ? Fixture.World->SpawnActor<ADeviceBase>() : nullptr;
	if (!TestNotNull(TEXT("Create a device in an isolated world"), Device))
	{
		return false;
	}
	Device->DispatchBeginPlay();
	const FDelegateHandle StateObserver = Device->OnDeviceStateChangedNative.AddLambda(
		[](ADeviceBase* ChangedDevice, EDeviceState OldState, EDeviceState NewState)
		{
			if (NewState == EDeviceState::Active)
			{
				ChangedDevice->SetGameplayEnabled(false);
			}
		});
	const FHitContext Hit = MakeDeviceHit();
	TestTrue(TEXT("A committed activation is accepted even if its callback cancels gameplay"),
		Device->TryActivate(Hit));
	TestTrue(TEXT("Callback cancellation reaches Spent before returning"),
		Device->GetDeviceState() == EDeviceState::Spent);
	TestFalse(TEXT("Cancelled device is no longer simulating physics"),
		Device->GetPhysicsBody()->IsSimulatingPhysics());
	Device->SetGameplayEnabled(true);
	TestFalse(TEXT("Callback cancellation cannot restore activation eligibility"), Device->TryActivate(Hit));
	Device->OnDeviceStateChangedNative.Remove(StateObserver);
	return true;
}

#endif
