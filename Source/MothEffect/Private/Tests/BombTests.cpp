#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/ExternalImpulseComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Devices/DeviceBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace
{
	FHitContext BombHit(APawn* Instigator = nullptr)
	{
		FHitContext Hit;
		Hit.HitId = FGuid::NewGuid();
		Hit.ShotDirection = FVector::ForwardVector;
		Hit.ImpactPoint = FVector(9000.0, 9000.0, 9000.0);
		Hit.InstigatorPawn = Instigator;
		return Hit;
	}

	ADeviceBase* SpawnDevice(FMothCombatTestWorld& Fixture, const FVector& Position, EDeviceKind Kind)
	{
		const FTransform Transform(FRotator::ZeroRotator, Position);
		ADeviceBase* Device = Fixture.World->SpawnActorDeferred<ADeviceBase>(ADeviceBase::StaticClass(),
			Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Device) return nullptr;
		FEnumProperty* Property = FindFProperty<FEnumProperty>(ADeviceBase::StaticClass(), TEXT("DeviceKind"));
		check(Property);
		Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Device),
			static_cast<int64>(Kind));
		Device->FinishSpawning(Transform);
		Device->GetPhysicsBody()->SetEnableGravity(false);
		Device->GetPhysicsBody()->SetLinearDamping(0.0f);
		return Device;
	}

	AMothPlayerAutomationCharacter* SpawnPawn(FMothCombatTestWorld& Fixture, const FVector& Position)
	{
		AMothPlayerAutomationCharacter* Pawn = Fixture.Spawn<AMothPlayerAutomationCharacter>(Position);
		if (Pawn)
		{
			UCharacterMovementComponent* Movement = Pawn->GetCharacterMovement();
			Movement->GravityScale = 0.0f;
			Movement->BrakingDecelerationFalling = 0.0f;
			Movement->FallingLateralFriction = 0.0f;
			Movement->bRunPhysicsWithNoController = true;
			Movement->SetMovementMode(MOVE_Falling);
		}
		return Pawn;
	}

	UBoxComponent* MakeBox(FMothCombatTestWorld& Fixture, const FVector& Position,
		const FVector& Extent, ECollisionChannel ObjectType, ECollisionResponse Response = ECR_Block,
		AActor* Owner = nullptr)
	{
		if (!Owner) Owner = Fixture.Spawn<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Owner);
		Owner->AddInstanceComponent(Box);
		if (Owner->GetRootComponent()) Box->SetupAttachment(Owner->GetRootComponent());
		else Owner->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionObjectType(ObjectType);
		Box->SetCollisionResponseToAllChannels(Response);
		Box->RegisterComponent();
		Box->SetWorldLocation(Position);
		return Box;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothBombSingleUseTest, "MothEffect.Devices.Bomb.SingleUseAndFrozenCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothBombSingleUseTest::RunTest(const FString& Parameters)
{
	for (double Height : {0.0, 900.0})
	{
		FMothCombatTestWorld Fixture;
		if (!TestNotNull(TEXT("Create gameplay world"), Fixture.World)) return false;
		const FVector Center(0.0, 0.0, Height);
		ADeviceBase* Bomb = SpawnDevice(Fixture, Center, EDeviceKind::Bomb);
		AMothPlayerAutomationCharacter* Shooter = SpawnPawn(Fixture, Center + FVector(180.0, 0.0, 0.0));
		AMothPlayerAutomationCharacter* Other = SpawnPawn(Fixture, Center + FVector(-180.0, 0.0, 0.0));
		if (!TestNotNull(TEXT("Bomb"), Bomb) || !TestNotNull(TEXT("Shooter"), Shooter)
			|| !TestNotNull(TEXT("Other pawn"), Other)) return false;
		MakeBox(Fixture, Shooter->GetActorLocation(), FVector(20.0), ECC_Pawn, ECR_Block, Shooter);
		const FHitContext Hit = BombHit(Shooter);
		int32 Activations = 0;
		Bomb->OnDeviceStateChangedNative.AddLambda([this, &Hit, &Activations, Center]
			(ADeviceBase* Device, EDeviceState OldState, EDeviceState NewState)
			{
				if (NewState != EDeviceState::Active) return;
				++Activations;
				TestFalse(TEXT("Reentrant activation is rejected"), Device->TryActivate(Hit));
				Device->SetActorLocation(Center + FVector(1500.0, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
			});
		TestTrue(TEXT("Ground and air bombs accept a ballistic activation"), Bomb->TryActivate(Hit));
		TestTrue(TEXT("Center is original device position, not impact point or callback movement"),
			Bomb->GetExplosionCenter().Equals(Center));
		TestEqual(TEXT("Instigator is not immune; multiple colliders deal damage once"), Shooter->GetHealthComponent()->GetCurrentHealth(), 40.0f);
		TestEqual(TEXT("Other pawn uses the same fixed damage"), Other->GetHealthComponent()->GetCurrentHealth(), 40.0f);
		TestTrue(TEXT("Explosion immediately commits Spent"), Bomb->GetDeviceState() == EDeviceState::Spent);
		TestFalse(TEXT("Another hit cannot repeat the explosion"), Bomb->TryActivate(BombHit()));
		TestEqual(TEXT("One activation transition"), Activations, 1);
		TestTrue(TEXT("Spent has finite visual cleanup"), Bomb->GetLifeSpan() > 0.0f);
		Bomb->OnDeviceStateChangedNative.Clear();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothBombCoverTest, "MothEffect.Devices.Bomb.StaticCoverAndRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothBombCoverTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Create gameplay world"), Fixture.World)) return false;
	const FVector Center(0.0, 0.0, 500.0);
	ADeviceBase* Bomb = SpawnDevice(Fixture, Center, EDeviceKind::Bomb);
	AMothPlayerAutomationCharacter* Covered = SpawnPawn(Fixture, Center + FVector(200.0, -70.0, 0.0));
	AMothPlayerAutomationCharacter* Exposed = SpawnPawn(Fixture, Center + FVector(-200.0, 0.0, 0.0));
	AMothPlayerAutomationCharacter* Outside = SpawnPawn(Fixture, Center + FVector(450.0, 0.0, 0.0));
	ADeviceBase* CoveredDevice = SpawnDevice(Fixture, Center + FVector(200.0, 70.0, 0.0), EDeviceKind::Emitter);
	if (!Bomb || !Covered || !Exposed || !Outside || !CoveredDevice) { AddError(TEXT("Spawn cover targets")); return false; }
	// A large extra collider overlaps the blast, but the root representative remains outside it.
	MakeBox(Fixture, Outside->GetActorLocation(), FVector(100.0), ECC_Pawn, ECR_Block, Outside);
	MakeBox(Fixture, Center + FVector(100.0, 0.0, 0.0), FVector(5.0, 90.0, 150.0), ECC_WorldStatic);
	MakeBox(Fixture, Center + FVector(-100.0, 0.0, 0.0), FVector(5.0, 90.0, 150.0), ECC_WorldStatic, ECR_Overlap);
	MakeBox(Fixture, Center + FVector(-150.0, 0.0, 0.0), FVector(5.0, 90.0, 150.0), ECC_WorldDynamic);
	TestTrue(TEXT("Activate cover scene"), Bomb->TryActivate(BombHit()));
	TestEqual(TEXT("Static solid cover shields health"), Covered->GetHealthComponent()->GetCurrentHealth(), 100.0f);
	TestEqual(TEXT("Dynamic blockers and static overlap triggers do not shield health"), Exposed->GetHealthComponent()->GetCurrentHealth(), 40.0f);
	TestEqual(TEXT("Root outside configured radius receives no damage"), Outside->GetHealthComponent()->GetCurrentHealth(), 100.0f);
	// Settle actual queued events without advancing physics; covered bodies must have no queued impulse.
	FWorldDelegates::OnWorldPostActorTick.Broadcast(Fixture.World, LEVELTICK_All, 0.0f);
	TestTrue(TEXT("Covered device gets neither movement nor activation"),
		CoveredDevice->GetPhysicsBody()->GetPhysicsLinearVelocity().IsNearlyZero()
		&& CoveredDevice->GetDeviceState() == EDeviceState::Dormant);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothBombStatesTest, "MothEffect.Devices.Bomb.HeldAndNonBallisticStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothBombStatesTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Create gameplay world"), Fixture.World)) return false;
	const FVector Center(0.0, 0.0, 700.0);
	AMothPlayerAutomationCharacter* Holder = SpawnPawn(Fixture, Center + FVector(180.0, 0.0, 0.0));
	ADeviceBase* Held = SpawnDevice(Fixture, Center + FVector(200.0, 50.0, 0.0), EDeviceKind::Emitter);
	ADeviceBase* NeighborBomb = SpawnDevice(Fixture, Center + FVector(-160.0, 0.0, 0.0), EDeviceKind::Bomb);
	ADeviceBase* Emitter = SpawnDevice(Fixture, Center + FVector(0.0, 180.0, 0.0), EDeviceKind::Emitter);
	ADeviceBase* Launcher = SpawnDevice(Fixture, Center + FVector(0.0, -180.0, 0.0), EDeviceKind::Launcher);
	ADeviceBase* Bomb = SpawnDevice(Fixture, Center, EDeviceKind::Bomb);
	if (!Holder || !Held || !NeighborBomb || !Emitter || !Launcher || !Bomb) { AddError(TEXT("Spawn state targets")); return false; }
	TestTrue(TEXT("Pickup held target"), Holder->TryPickupDevice(Held));
	TestTrue(TEXT("Activate emitter"), Emitter->TryActivate(BombHit()));
	TestTrue(TEXT("Activate fixed launcher shell"), Launcher->TryActivate(BombHit()));
	TestTrue(TEXT("Activate nearby bomb"), Bomb->TryActivate(BombHit(Holder)));
	TestEqual(TEXT("Holding does not shield the character"), Holder->GetHealthComponent()->GetCurrentHealth(), 40.0f);
	TestTrue(TEXT("Held remains attached and has no independent physics"), Held->GetHolder() == Holder && !Held->GetPhysicsBody()->IsSimulatingPhysics());
	TestTrue(TEXT("Dormant neighbor bomb is not triggered"), NeighborBomb->GetDeviceState() == EDeviceState::Dormant);
	TestTrue(TEXT("Active emitter is neither restarted nor redirected"), Emitter->GetDeviceState() == EDeviceState::Active
		&& Emitter->GetActivationDirection().Equals(FVector::ForwardVector));
	TestFalse(TEXT("An active launcher cannot receive external motion"), Launcher->CanReceiveExternalImpulse());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothBombCancelTest, "MothEffect.Devices.Bomb.CancelBeforeEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothBombCancelTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Create gameplay world"), Fixture.World)) return false;
	ADeviceBase* Bomb = SpawnDevice(Fixture, FVector(0.0, 0.0, 500.0), EDeviceKind::Bomb);
	AMothPlayerAutomationCharacter* Target = SpawnPawn(Fixture, FVector(150.0, 0.0, 500.0));
	if (!Bomb || !Target) { AddError(TEXT("Spawn cancellation scene")); return false; }
	Bomb->OnDeviceStateChangedNative.AddLambda([](ADeviceBase* Device, EDeviceState OldState, EDeviceState NewState)
		{ if (NewState == EDeviceState::Active) Device->SetGameplayEnabled(false); });
	TestTrue(TEXT("Activation is committed before the cancelling observer"), Bomb->TryActivate(BombHit()));
	TestEqual(TEXT("Cancellation before effect prevents damage"), Target->GetHealthComponent()->GetCurrentHealth(), 100.0f);
	TestTrue(TEXT("Cancelled bomb cannot return to Dormant"), Bomb->GetDeviceState() == EDeviceState::Spent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothBombBlueprintTest, "MothEffect.Devices.Bomb.BlueprintDefaultsAndActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothBombBlueprintTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Create asset integration world"), Fixture.World)) return false;
	UClass* BombClass = LoadClass<ADeviceBase>(nullptr,
		TEXT("/Game/MothEffect/Devices/BP_DeviceBomb.BP_DeviceBomb_C"));
	if (!TestNotNull(TEXT("Load saved bomb Blueprint from disk"), BombClass)) return false;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADeviceBase* Bomb = Fixture.World->SpawnActor<ADeviceBase>(BombClass, FVector(0.0, 0.0, 500.0), FRotator::ZeroRotator, Spawn);
	AMothPlayerAutomationCharacter* Pawn = SpawnPawn(Fixture, FVector(180.0, 0.0, 500.0));
	if (!Bomb || !Pawn) { AddError(TEXT("Spawn saved Blueprint and native target")); return false; }
	TestTrue(TEXT("Saved Blueprint retains Bomb kind"), Bomb->GetDeviceKind() == EDeviceKind::Bomb);
	const UStaticMeshComponent* Mesh = Bomb->FindComponentByClass<UStaticMeshComponent>();
	if (!TestNotNull(TEXT("Saved device has its visual mesh"), Mesh)) return false;
	if (!TestNotNull(TEXT("Resolved visual persists across editor processes"), Mesh->GetStaticMesh().Get())) return false;
	const FVector VisualExtent=Mesh->GetStaticMesh()->GetBoundingBox().GetExtent()*Mesh->GetRelativeScale3D();
	TestTrue(TEXT("Replaceable visual stays within the shot envelope"),VisualExtent.GetMin()>1&&VisualExtent.GetMax()<=25);
	TestEqual(TEXT("Visual replacement retains the native physical radius"),Bomb->GetPhysicsBody()->GetUnscaledSphereRadius(),20.f);
	TestTrue(TEXT("Saved asset uses native ballistic activation"), IBallisticReactive::Execute_ReceiveBallisticHit(Bomb, BombHit(Pawn)));
	TestEqual(TEXT("Saved asset applies baseline fixed damage"), Pawn->GetHealthComponent()->GetCurrentHealth(), 40.0f);
	TestTrue(TEXT("Saved asset reaches Spent once"), Bomb->GetDeviceState() == EDeviceState::Spent);
	return true;
}

namespace
{
	enum class EImpulseCase { BlastMovement, Merge, PauseCleanup };
	class FImpulseTimeline final : public IAutomationLatentCommand
	{
	public:
		FImpulseTimeline(FAutomationTestBase* InTest, EImpulseCase InCase) : Test(InTest), Case(InCase) {}
		bool Update() override
		{
			if (!Fixture)
			{
				Fixture = MakeUnique<FMothCombatTestWorld>();
				if (!Test->TestNotNull(TEXT("Create physics world"), Fixture->World)) return true;
				const FVector Center(0.0, 0.0, 800.0);
				Pawn = SpawnPawn(*Fixture, Center + FVector(200.0, 0.0, 0.0));
				Other = SpawnPawn(*Fixture, Center + FVector(-200.0, 0.0, 0.0));
				Device = SpawnDevice(*Fixture, Center + FVector(0.0, 200.0, 0.0), EDeviceKind::Emitter);
				if (!Pawn || !Other || !Device) { Test->AddError(TEXT("Spawn motion targets")); return true; }
				Fixture->World->GetTimerManager().Tick(0.0f);
				if (Case == EImpulseCase::BlastMovement)
				{
					ADeviceBase* Bomb = SpawnDevice(*Fixture, Center, EDeviceKind::Bomb);
					Test->TestTrue(TEXT("Blast schedules native movement"), Bomb->TryActivate(BombHit(Pawn)));
				}
				else if (Case == EImpulseCase::Merge)
				{
					const FVector Current(100.0, 200.0, -100.0);
					Pawn->GetCharacterMovement()->Velocity = Current;
					Other->GetCharacterMovement()->Velocity = Current;
					Device->GetPhysicsBody()->SetPhysicsLinearVelocity(Current);
					for (AActor* Target : {static_cast<AActor*>(Pawn), static_cast<AActor*>(Other), static_cast<AActor*>(Device)})
					{
						const FGuid Id = FGuid::NewGuid();
						if (Target == Other) IImpulseReceiver::Execute_ReceiveImpulse(Target, FVector(0.0, 0.0, 1000.0), true, FGuid::NewGuid());
						IImpulseReceiver::Execute_ReceiveImpulse(Target, FVector(650.0, 0.0, 450.0), false, Id);
						IImpulseReceiver::Execute_ReceiveImpulse(Target, FVector(650.0, 0.0, 450.0), false, Id);
						IImpulseReceiver::Execute_ReceiveImpulse(Target, FVector(-100.0, 100.0, 50.0), false, FGuid::NewGuid());
						if (Target != Other) IImpulseReceiver::Execute_ReceiveImpulse(Target, FVector(0.0, 0.0, 1000.0), true, FGuid::NewGuid());
					}
				}
				else
				{
					IImpulseReceiver::Execute_ReceiveImpulse(Pawn, FVector(300.0, 0.0, 400.0), false, FGuid::NewGuid());
					Fixture->World->GetWorldSettings()->SetPauserPlayerState(Fixture->Spawn<APlayerState>());
				}
				return false;
			}
			UWorld* World = Fixture->World;
			if (World->GetTimerManager().HasBeenTickedThisFrame()) return false;
			World->Tick(LEVELTICK_All, 0.01f);
			++Frames;
			if (Frames > 20) { Test->AddError(TEXT("Impulse timeline exceeded bound")); return true; }
			if (Case == EImpulseCase::PauseCleanup && Frames <= 3)
			{
				Test->TestTrue(TEXT("Pause leaves queued launch unapplied"), Pawn->GetCharacterMovement()->Velocity.IsNearlyZero());
				if (Frames == 3) World->GetWorldSettings()->SetPauserPlayerState(nullptr);
				return false;
			}
			const int32 ReadyFrame = Case == EImpulseCase::PauseCleanup ? 5 : 2;
			if (Frames < ReadyFrame) return false;
			if (Case == EImpulseCase::BlastMovement)
			{
				Test->TestTrue(TEXT("Native CharacterMovement receives additive radial/up velocity"), Pawn->GetVelocity().Equals(FVector(650.0, 0.0, 450.0), 1.0));
				Test->TestTrue(TEXT("Opposite pawn uses opposite horizontal direction"), Other->GetVelocity().Equals(FVector(-650.0, 0.0, 450.0), 1.0));
				Test->TestTrue(TEXT("Physics root receives the 1100cm/s radial velocity"), Device->GetPhysicsBody()->GetPhysicsLinearVelocity().Equals(FVector(0.0, 1100.0, 0.0), 1.0));
				Test->TestTrue(TEXT("Motion cannot activate the Dormant device"), Device->GetDeviceState() == EDeviceState::Dormant);
				return true;
			}
			if (Case == EImpulseCase::Merge)
			{
				if (Frames == 2)
				{
					const FVector Expected(650.0, 300.0, 1000.0);
					Test->TestTrue(TEXT("Merge adds explosions once then replaces Z"), Pawn->GetVelocity().Equals(Expected, 1.0));
					Test->TestTrue(TEXT("Changing callback order gives the same character velocity"), Other->GetVelocity().Equals(Expected, 1.0));
					Test->TestTrue(TEXT("Physics roots use the same order and preserve initial velocity"), Device->GetPhysicsBody()->GetPhysicsLinearVelocity().Equals(Expected, 1.0));
					for (AActor* Target : {static_cast<AActor*>(Pawn), static_cast<AActor*>(Device)})
						IImpulseReceiver::Execute_ReceiveImpulse(Target, FVector(5000.0, 0.0, 6000.0), false, FGuid::NewGuid());
					return false;
				}
				if (Frames < 4) return false;
				Test->TestTrue(TEXT("Character velocity is limited after merged events"), FMath::IsNearlyEqual(Pawn->GetVelocity().Size(), 1600.0, 1.0));
				Test->TestTrue(TEXT("Physics root has the same 1600cm/s limit"), FMath::IsNearlyEqual(Device->GetPhysicsBody()->GetPhysicsLinearVelocity().Size(), 1600.0, 1.0));
				return true;
			}
			if (Frames == 5)
			{
				Test->TestTrue(TEXT("Resume applies preserved pending impulse once"), Pawn->GetVelocity().Equals(FVector(300.0, 0.0, 400.0), 1.0));
				// Queue and settle another launch, then disable before CharacterMovement consumes it.
				IImpulseReceiver::Execute_ReceiveImpulse(Pawn, FVector(1000.0, 0.0, 0.0), false, FGuid::NewGuid());
				FWorldDelegates::OnWorldPostActorTick.Broadcast(World, LEVELTICK_All, 0.0f);
				Pawn->SetGameplayEnabled(false);
				Pawn->GetCharacterMovement()->Velocity = FVector::ZeroVector;
				IImpulseReceiver::Execute_ReceiveImpulse(Other, FVector(1000.0, 0.0, 0.0), false, FGuid::NewGuid());
				FHitContext Fatal = BombHit(); Fatal.Damage = 1000.0f;
				Other->GetHealthComponent()->ApplyHit(Fatal);
				return false;
			}
			if (Frames < 7) return false;
			Test->TestTrue(TEXT("Round disable cancels both accumulator and pending native launch"), Pawn->GetVelocity().IsNearlyZero());
			Test->TestTrue(TEXT("Death cancels queued external movement"), Other->GetVelocity().IsNearlyZero()
				&& Other->GetActionState() == EPlayerActionState::Dead);
			return true;
		}
	private:
		FAutomationTestBase* Test;
		EImpulseCase Case;
		TUniquePtr<FMothCombatTestWorld> Fixture;
		AMothPlayerAutomationCharacter* Pawn = nullptr;
		AMothPlayerAutomationCharacter* Other = nullptr;
		ADeviceBase* Device = nullptr;
		int32 Frames = 0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothBombMotionTest, "MothEffect.Devices.Bomb.PhysicalMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothBombMotionTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FImpulseTimeline(this, EImpulseCase::BlastMovement)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothImpulseMergeTest, "MothEffect.Combat.Impulse.FrameMergeAndClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothImpulseMergeTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FImpulseTimeline(this, EImpulseCase::Merge)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothImpulsePauseTest, "MothEffect.Combat.Impulse.PauseAndCancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothImpulsePauseTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FImpulseTimeline(this, EImpulseCase::PauseCleanup)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothImpulseInvalidTest, "MothEffect.Combat.Impulse.InvalidAndDuplicateEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothImpulseInvalidTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Create world"), Fixture.World)) return false;
	AMothPlayerAutomationCharacter* Pawn = SpawnPawn(Fixture, FVector(0.0, 0.0, 500.0));
	if (!TestNotNull(TEXT("Create receiver"), Pawn)) return false;
	UExternalImpulseComponent* Receiver = Pawn->FindComponentByClass<UExternalImpulseComponent>();
	if (!TestNotNull(TEXT("Native receiver component"), Receiver)) return false;
	TestFalse(TEXT("Invalid event ID is rejected"), Receiver->QueueImpulse(FVector::ForwardVector, false, FGuid()));
	TestFalse(TEXT("Nonfinite velocity is rejected"), Receiver->QueueImpulse(FVector(std::numeric_limits<double>::infinity(), 0.0, 0.0), false, FGuid::NewGuid()));
	const FGuid Event = FGuid::NewGuid();
	TestTrue(TEXT("Valid event is accepted"), Receiver->QueueImpulse(FVector::ForwardVector, false, Event));
	TestFalse(TEXT("Duplicate event is rejected"), Receiver->QueueImpulse(FVector::ForwardVector, false, Event));
	Pawn->SetGameplayEnabled(false);
	TestFalse(TEXT("Disabled receiver rejects new events"), Receiver->QueueImpulse(FVector::ForwardVector, false, FGuid::NewGuid()));
	return true;
}

#endif
