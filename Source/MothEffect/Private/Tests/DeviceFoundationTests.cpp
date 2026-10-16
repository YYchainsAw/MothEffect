#if WITH_DEV_AUTOMATION_TESTS

#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HealthComponent.h"
#include "Components/ExternalImpulseComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "TimerManager.h"
#include "Types/MothCollisionChannels.h"
#include "UObject/UnrealType.h"

namespace MothFoundationTest
{
	FHitContext Hit(FVector Direction = FVector(1, 2, 3))
	{
		FHitContext H; H.HitId = FGuid::NewGuid(); H.ShotDirection = Direction; return H;
	}
	ADeviceBase* Device(FMothCombatTestWorld& F, EDeviceKind Kind, FVector Position)
	{
		const FTransform Transform(Position);
		auto* D = F.World->SpawnActorDeferred<ADeviceBase>(ADeviceBase::StaticClass(), Transform,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!D) return nullptr;
		auto* P = FindFProperty<FEnumProperty>(ADeviceBase::StaticClass(), TEXT("DeviceKind"));
		P->GetUnderlyingProperty()->SetIntPropertyValue(P->ContainerPtrToValuePtr<void>(D), static_cast<int64>(Kind));
		D->FinishSpawning(Transform);
		return D;
	}
	void Floor(FMothCombatTestWorld& F)
	{
		auto* A = F.Spawn<AActor>(); auto* B = NewObject<UBoxComponent>(A);
		A->AddInstanceComponent(B); A->SetRootComponent(B);
		B->SetBoxExtent(FVector(6000, 2500, 10));
		B->SetCollisionObjectType(ECC_WorldStatic);
		B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		B->SetCollisionResponseToAllChannels(ECR_Block);
		B->ComponentTags.Add(TEXT("MothColumnSurface"));
		B->RegisterComponent(); B->SetWorldLocation(FVector(2000, 0, -10));
	}
	bool Tick(FMothCombatTestWorld& F)
	{
		if (F.World->GetTimerManager().HasBeenTickedThisFrame()) return false;
		F.World->Tick(LEVELTICK_All, .01f);
		return true;
	}

	class FGroundFlight final : public IAutomationLatentCommand
	{
	public:
		explicit FGroundFlight(FAutomationTestBase* Test) : T(Test) {}
		bool Update() override
		{
			if (!F)
			{
				F = MakeUnique<FMothCombatTestWorld>(); Floor(*F);
				D = Device(*F, EDeviceKind::Emitter, FVector(0, 0, 150));
				return !T->TestNotNull(TEXT("Native falling device"), D.Get());
			}
			if (!Tick(*F)) return false;
			const double Now = F->World->GetTimeSeconds();
			if (Now > 8) { T->AddError(TEXT("Ground/flight/landing exceeded the bounded real-world timeline")); return true; }
			if (!D.IsValid()) { T->AddError(TEXT("Grounded device vanished before its idle lifetime")); return true; }
			if (Stage == 0)
			{
				if (!D->IsGrounded()) return false;
				Landed = D->GetActorTransform();
				T->TestFalse(TEXT("Landing stops Chaos simulation"), D->GetPhysicsBody()->IsSimulatingPhysics());
				T->TestTrue(TEXT("Ground body has no entity collision"), D->GetPhysicsBody()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
				auto* Query = FindObject<UPrimitiveComponent>(D.Get(), TEXT("DroppedCollider"));
				if (!T->TestNotNull(TEXT("Separate ballistic query shape"), Query)) return true;
				T->TestTrue(TEXT("Grounded ballistic shape remains query-only"), Query->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
				for (ECollisionChannel Channel : {ECC_Visibility, MothCollision::Projectile})
				{
					FHitResult H;
					T->TestTrue(TEXT("Both W01 and P01 queries hit the fixed ground device"),
						F->World->LineTraceSingleByChannel(H, Landed.GetLocation()+FVector(-100,0,0), Landed.GetLocation()+FVector(100,0,0), Channel)
						&& H.GetActor() == D.Get());
				}
				Pawn = F->Spawn<AMothPlayerAutomationCharacter>(Landed.GetLocation()+FVector(-150,0,90));
				if (!T->TestNotNull(TEXT("Actual character collision fixture"), Pawn)) return true;
				Pawn->GetCharacterMovement()->DisableMovement();
				FHitResult WalkHit;
				Pawn->SetActorLocation(Landed.GetLocation()+FVector(150,0,90), true, &WalkHit);
				T->TestTrue(TEXT("Character sweep passes through the ground prop"), !WalkHit.bBlockingHit);
				ChangedAt = Now; Stage = 1; return false;
			}
			if (Stage == 1 && Now >= ChangedAt+.3)
			{
				T->TestTrue(TEXT("Walking cannot kick, roll or slide the ground device"), D->GetActorTransform().Equals(Landed, .01));
				IImpulseReceiver::Execute_ReceiveImpulse(D.Get(), FVector(200, 0, 700), true, FGuid::NewGuid());
				ChangedAt = Now; Stage = 2; return false;
			}
			if (Stage == 2 && Now >= ChangedAt+.1)
			{
				T->TestFalse(TEXT("A real device impulse clears Grounded"), D->IsGrounded());
				T->TestTrue(TEXT("Device force restores physical flight"), D->GetPhysicsBody()->IsSimulatingPhysics()
					&& D->GetActorLocation().Z > Landed.GetLocation().Z+25);
				Stage = 3; return false;
			}
			if (Stage == 3 && D->IsGrounded())
			{
				T->TestFalse(TEXT("Landing again returns to fixed simulation"), D->GetPhysicsBody()->IsSimulatingPhysics());
				T->TestTrue(TEXT("Grounded remains eligible for actual pickup"), D->CanBePickedUp());
				T->TestTrue(TEXT("Explicit external wake supports future device forces"), D->WakeForExternalImpulse());
				T->TestFalse(TEXT("Explicit wake clears Grounded"), D->IsGrounded());
				return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* T;
		TUniquePtr<FMothCombatTestWorld> F;
		TWeakObjectPtr<ADeviceBase> D;
		AMothPlayerAutomationCharacter* Pawn = nullptr;
		FTransform Landed;
		int32 Stage = 0;
		double ChangedAt = 0;
	};

	class FDelayedEffects final : public IAutomationLatentCommand
	{
	public:
		explicit FDelayedEffects(FAutomationTestBase* Test) : T(Test) {}
		bool Update() override
		{
			if (!F)
			{
				F = MakeUnique<FMothCombatTestWorld>(); Floor(*F);
				Target = F->Spawn<AMothPlayerAutomationCharacter>(FVector(150, 0, 400));
				Target->GetCharacterMovement()->GravityScale = 0;
				Target->GetCharacterMovement()->DisableMovement();
				for (int32 I = 0; I < 3; ++I)
				{
					auto* D = Device(*F, static_cast<EDeviceKind>(I), FVector(I*1500, 0, 400));
					if (!T->TestNotNull(TEXT("Each native device type"), D)) return true;
					Devices.Add(D); T->TestEqual(TEXT("Every type uses the shared default .3s delay"), D->ActivationDelaySeconds, .3f);
					// The Bomb is at x=1500. Keep its native victim beside it.
					if (I == 1) Target->SetActorLocation(FVector(1650, 0, 400));
					T->TestTrue(TEXT("Activation is accepted exactly once"), D->TryActivate(Hit()));
					T->TestTrue(TEXT("Active commits before effects"), D->GetDeviceState() == EDeviceState::Active && D->IsActivationPending());
					T->TestFalse(TEXT("Pending cannot be activated again"), D->TryActivate(Hit()));
					D->GetPhysicsBody()->SetEnableGravity(false);
				}
				return false;
			}
			if (!Tick(*F)) return false;
			const double Now = F->World->GetTimeSeconds();
			if (Stage == 0 && Now >= .2)
			{
				T->TestEqual(TEXT("Bomb does no pre-delay damage"), Target->GetHealthComponent()->GetCurrentHealth(), 100.f);
				T->TestNull(TEXT("Launcher creates no pre-delay column"), Devices[0]->GetLaunchColumn());
				T->TestEqual(TEXT("Emitter makes no pre-delay attempt"), Devices[2]->GetEmitterShotAttempts(), 0);
				for (const auto& D : Devices) T->TestTrue(TEXT("All effects remain pending before .3s"), D->IsActivationPending());
				Stage = 1;
			}
			if (Stage == 1 && Now >= .34)
			{
				T->TestEqual(TEXT("Default delayed bomb settles native damage"), Target->GetHealthComponent()->GetCurrentHealth(), 40.f);
				T->TestTrue(TEXT("Delayed bomb commits Spent"), Devices[1]->GetDeviceState() == EDeviceState::Spent);
				T->TestNotNull(TEXT("Default delayed launcher starts a real column"), Devices[0]->GetLaunchColumn());
				T->TestFalse(TEXT("Emitter effect starts after the shared delay"), Devices[2]->IsActivationPending());
				T->TestEqual(TEXT("Emitter keeps its additional first-shot delay"), Devices[2]->GetEmitterShotAttempts(), 0);
				Devices[2]->GetPhysicsBody()->SetEnableGravity(false);
				Stage = 2;
			}
			if (Stage == 2 && Now >= .46)
			{
				T->TestEqual(TEXT("Emitter emits once after .3 plus .1"), Devices[2]->GetEmitterShotAttempts(), 1);
				for (TActorIterator<ARuleProjectile> It(F->World); It; ++It)
					T->TestTrue(TEXT("Actual spawned P01 flies in the horizontal locked direction"), It->GetFlightDirection().Equals(FVector(1,2,0).GetSafeNormal()));
				Stage = 3;
			}
			if (Stage == 3 && Now >= 3.35)
			{
				T->TestTrue(TEXT("Emitter duration begins at effect startup"), Devices[2]->GetDeviceState() == EDeviceState::Spent);
				T->TestEqual(TEXT("Delay retains the finite twelve-slot emitter schedule"), Devices[2]->GetEmitterShotAttempts(), 12);
				return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* T;
		TUniquePtr<FMothCombatTestWorld> F;
		TArray<TWeakObjectPtr<ADeviceBase>> Devices;
		AMothPlayerAutomationCharacter* Target = nullptr;
		int32 Stage = 0;
	};

	class FPauseAndCancellation final : public IAutomationLatentCommand
	{
	public:
		explicit FPauseAndCancellation(FAutomationTestBase* Test) : T(Test) {}
		bool Update() override
		{
			if (!F)
			{
				F = MakeUnique<FMothCombatTestWorld>(); Floor(*F);
				Target = F->Spawn<AMothPlayerAutomationCharacter>(FVector(1650, 0, 400));
				Target->GetCharacterMovement()->GravityScale = 0;
				Target->GetCharacterMovement()->DisableMovement();
				for (int32 I = 0; I < 3; ++I)
				{
					auto* D = Device(*F, static_cast<EDeviceKind>(I), FVector(I*1500, 0, 400));
					if (!T->TestNotNull(TEXT("Pending pause device"), D)) return true;
					Devices.Add(D); D->TryActivate(Hit()); D->GetPhysicsBody()->SetEnableGravity(false);
					auto* Cancel = Device(*F, static_cast<EDeviceKind>(I), FVector(I*1500, 1500, 400));
					if (!T->TestNotNull(TEXT("Pending cancellation device"), Cancel)) return true;
					Cancelled.Add(Cancel); Cancel->TryActivate(Hit()); Cancel->SetGameplayEnabled(false);
					T->TestTrue(TEXT("Cancellation immediately releases pending Active"), Cancel->GetDeviceState() == EDeviceState::Spent && !Cancel->IsActivationPending());
				}
				return false;
			}
			if (PausedFrames > 0)
			{
				if (!Tick(*F)) return false;
				T->TestEqual(TEXT("Real pause freezes the delayed gameplay clock"), F->World->GetTimeSeconds(), PausedAt);
				for (const auto& D : Devices) T->TestTrue(TEXT("Pause retains each pending effect"), D->IsActivationPending());
				T->TestEqual(TEXT("No delayed damage leaks during pause"), Target->GetHealthComponent()->GetCurrentHealth(), 100.f);
				if (--PausedFrames == 0) F->World->GetWorldSettings()->SetPauserPlayerState(nullptr);
				return false;
			}
			if (!Tick(*F)) return false;
			const double Now = F->World->GetTimeSeconds();
			if (Stage == 0 && Now >= .1)
			{
				PausedAt = Now;
				F->World->GetWorldSettings()->SetPauserPlayerState(F->Spawn<APlayerState>());
				PausedFrames = 12; Stage = 1; return false;
			}
			if (Stage == 1 && Now >= .23)
			{
				T->TestEqual(TEXT("Resume preserves the remaining delay"), Target->GetHealthComponent()->GetCurrentHealth(), 100.f);
				T->TestNull(TEXT("Resumed launcher still waits the remaining delay"), Devices[0]->GetLaunchColumn());
				Stage = 2;
			}
			if (Stage == 2 && Now >= .46)
			{
				T->TestEqual(TEXT("Resumed delayed bomb executes once"), Target->GetHealthComponent()->GetCurrentHealth(), 40.f);
				T->TestNotNull(TEXT("Resumed delayed launcher creates its effect"), Devices[0]->GetLaunchColumn());
				T->TestEqual(TEXT("Resumed emitter retains the first-shot wait"), Devices[2]->GetEmitterShotAttempts(), 1);
				for (const auto& D : Cancelled)
				{
					if (!D.IsValid()) continue; // Native .25s Bomb Spent cleanup may already have destroyed it.
					T->TestFalse(TEXT("Cancelled activation stays cancelled"), D->IsActivationPending());
					T->TestNull(TEXT("Cancelled launcher cannot allocate a late column"), D->GetLaunchColumn());
					T->TestEqual(TEXT("Cancelled emitter never allocates a late shot"), D->GetEmitterShotAttempts(), 0);
				}
				// Closing the session while another three effects are pending must cancel their timers too.
				for (int32 I = 0; I < 3; ++I)
				{
					auto* D = Device(*F, static_cast<EDeviceKind>(I), FVector(I*1500, -1500, 400));
					Closed.Add(D); D->TryActivate(Hit());
				}
				F->World->GetSubsystem<UDeviceLifecycleSubsystem>()->SetGameplayEnabled(false);
				for (const auto& D : Closed) T->TestTrue(TEXT("Closing the session immediately cancels every pending effect"), D.IsValid() && !D->IsActivationPending() && D->GetDeviceState()==EDeviceState::Spent);
				Stage = 3;
			}
			if (Stage == 3 && Now >= .9)
			{
				for (const auto& D : Closed)
				{
					if (!D.IsValid()) continue; // Allow the actual Spent lifetime to complete normally.
					T->TestFalse(TEXT("Session shutdown cancels pending effects"), D->IsActivationPending());
					T->TestNull(TEXT("Closed session has no late column"), D->GetLaunchColumn());
					T->TestEqual(TEXT("Closed session has no late emitter attempt"), D->GetEmitterShotAttempts(), 0);
				}
				return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* T;
		TUniquePtr<FMothCombatTestWorld> F;
		TArray<TWeakObjectPtr<ADeviceBase>> Devices, Cancelled, Closed;
		AMothPlayerAutomationCharacter* Target = nullptr;
		int32 Stage = 0, PausedFrames = 0;
		double PausedAt = 0;
	};

	class FExplosionChain final : public IAutomationLatentCommand
	{
	public:
		explicit FExplosionChain(FAutomationTestBase* Test) : T(Test) {}
		bool Update() override
		{
			if (!F)
			{
				F = MakeUnique<FMothCombatTestWorld>(); Floor(*F);
				const FTransform VictimTransform(FVector(250,100,100));
				Victim = F->World->SpawnActorDeferred<AMothPlayerAutomationCharacter>(AMothPlayerAutomationCharacter::StaticClass(), VictimTransform,
					nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (!T->TestNotNull(TEXT("High-health chain victim"), Victim)) return true;
				T->TestTrue(TEXT("Chain fixture configures health before BeginPlay"), Victim->GetHealthComponent()->ConfigureMaxHealth(600.f));
				Victim->FinishSpawning(VictimTransform);
				Victim->FindComponentByClass<UExternalImpulseComponent>()->SetGameplayEnabled(false); // Isolate independent damage IDs from target motion.
				for (int32 I = 0; I < 3; ++I)
				{
					auto* D = Device(*F, EDeviceKind::Bomb, FVector(I*250, 0, 40));
					if (!T->TestNotNull(TEXT("Ordinary bomb in chain"), D)) return true;
					D->GetPhysicsBody()->SetEnableGravity(false); Bombs.Add(D);
					D->OnDeviceStateChangedNative.AddLambda([this, I](ADeviceBase* Changed, EDeviceState, EDeviceState New)
					{
						if (New == EDeviceState::Active) { ++Activations[I]; Ids[I] = Changed->GetActivationHitId(); }
						if (New == EDeviceState::Spent) bSpent[I] = true;
					});
				}
				const FHitContext H = Hit(FVector::ForwardVector);
				T->TestTrue(TEXT("Only the first bomb receives the starter"), Bombs[0]->TryActivate(H));
				return false;
			}
			if (!Tick(*F)) return false;
			const double Now = F->World->GetTimeSeconds();
			if (Stage == 0 && Now >= .2)
			{
				T->TestTrue(TEXT("Neighbor stays Dormant until the first actual explosion"), Bombs[1]->GetDeviceState() == EDeviceState::Dormant);
				Stage = 1;
			}
			if (Stage == 1 && Now >= .34)
			{
				T->TestTrue(TEXT("First explosion activates the nearby ordinary bomb"), Bombs[1]->GetDeviceState() == EDeviceState::Active && Bombs[1]->IsActivationPending());
				T->TestTrue(TEXT("Out-of-radius bomb waits for the next explosion"), Bombs[2]->GetDeviceState() == EDeviceState::Dormant);
				T->TestEqual(TEXT("First real explosion damages the victim once"), Victim->GetHealthComponent()->GetCurrentHealth(), 540.f);
				Stage = 2;
			}
			if (Stage == 2 && Now >= .66)
			{
				T->TestTrue(TEXT("Second explosion starts the next independently delayed bomb"), Bombs[2]->GetDeviceState() == EDeviceState::Active && Bombs[2]->IsActivationPending());
				T->TestEqual(TEXT("Second real explosion independently damages the same victim"), Victim->GetHealthComponent()->GetCurrentHealth(), 480.f);
				Stage = 3;
			}
			if (Stage == 3 && Now >= 1.0)
			{
				for (int32 I = 0; I < 3; ++I)
				{
					T->TestEqual(TEXT("Each ordinary bomb activates only once"), Activations[I], 1);
					T->TestTrue(TEXT("Each chain hit has a real unique identity"), Ids[I].IsValid());
					T->TestTrue(TEXT("Each delayed bomb was observed committing Spent"), bSpent[I]);
					if (Bombs[I].IsValid()) Bombs[I]->OnDeviceStateChangedNative.Clear();
				}
				T->TestEqual(TEXT("Third independently identified explosion deals its own native damage"), Victim->GetHealthComponent()->GetCurrentHealth(), 420.f);
				T->TestTrue(TEXT("Consecutive explosions do not reuse their activation IDs"), Ids[0] != Ids[1] && Ids[1] != Ids[2] && Ids[0] != Ids[2]);
				return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* T;
		TUniquePtr<FMothCombatTestWorld> F;
		TArray<TWeakObjectPtr<ADeviceBase>> Bombs;
		AMothPlayerAutomationCharacter* Victim = nullptr;
		int32 Activations[3] = {0,0,0};
		bool bSpent[3] = {false,false,false};
		FGuid Ids[3];
		int32 Stage = 0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFoundationSize, "MothEffect.Devices.Foundation.EditableSizesKeepQueryAndVisualIndependent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFoundationSize::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	auto* D = MothFoundationTest::Device(F, EDeviceKind::Emitter, FVector(0,0,1000));
	if (!TestNotNull(TEXT("Editable native device"), D)) return false;
	const FTransform ActorBefore = D->GetActorTransform();
	const FVector VisualBefore = D->GetDeviceMesh()->GetRelativeScale3D();
	FindFProperty<FFloatProperty>(ADeviceBase::StaticClass(), TEXT("BodyRadiusCm"))->SetPropertyValue_InContainer(D, 35.f);
	FindFProperty<FFloatProperty>(ADeviceBase::StaticClass(), TEXT("ShotRadiusCm"))->SetPropertyValue_InContainer(D, 42.f);
	D->VisualScale = 1.7f; D->RefreshDeviceSize();
	TestEqual(TEXT("Editable physical radius refreshes"), D->GetPhysicsBody()->GetUnscaledSphereRadius(), 35.f);
	auto* Shot = FindObject<USphereComponent>(D, TEXT("ShotCollider"));
	if (!TestNotNull(TEXT("Editable shot sphere"), Shot)) return false;
	TestEqual(TEXT("Editable query radius refreshes separately"), Shot->GetUnscaledSphereRadius(), 42.f);
	TestTrue(TEXT("Visual multiplier changes only local mesh scale"), D->GetDeviceMesh()->GetRelativeScale3D().Equals(VisualBefore*1.7f, .001));
	TestTrue(TEXT("Size edits do not relocate or rescale the actor"), D->GetActorTransform().Equals(ActorBefore));
	D->RefreshDeviceSize(); D->RefreshPrototypeAppearance();
	TestTrue(TEXT("Repeated appearance/size refresh does not compound the multiplier"), D->GetDeviceMesh()->GetRelativeScale3D().Equals(VisualBefore*1.7f, .001));
	FindFProperty<FFloatProperty>(ADeviceBase::StaticClass(), TEXT("ShotRadiusCm"))->SetPropertyValue_InContainer(D, 10.f);
	D->RefreshDeviceSize();
	TestEqual(TEXT("Query envelope cannot shrink inside the physical body"), Shot->GetUnscaledSphereRadius(), 35.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFoundationHeading, "MothEffect.Devices.Foundation.HorizontalProjectionAndVerticalFallback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFoundationHeading::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	const FVector Directions[] {FVector(1,2,3), FVector(-2,3,-8), FVector::UpVector, -FVector::UpVector};
	for (int32 I = 0; I < 4; ++I)
	{
		auto* D = MothFoundationTest::Device(F, EDeviceKind::Emitter, FVector(I*300,0,1000));
		if (!TestNotNull(TEXT("Directional emitter"), D)) return false;
		const FHitContext H = MothFoundationTest::Hit(Directions[I]);
		TestTrue(TEXT("Valid pitched/vertical shot commits activation"), D->TryActivate(H));
		const FVector Expected = I < 2 ? Directions[I].GetSafeNormal2D() : FVector::ForwardVector;
		TestTrue(TEXT("Locked direction is the horizontal unit vector or stable world fallback"), D->GetActivationDirection().Equals(Expected, .001));
		TestTrue(TEXT("Projected direction has zero Z and unit length"), FMath::IsNearlyZero(D->GetActivationDirection().Z) && FMath::IsNearlyEqual(D->GetActivationDirection().Size(),1.0));
		D->SetActorRotation(FRotator(60,120,80), ETeleportType::TeleportPhysics);
		TestFalse(TEXT("A later shot cannot redirect pending"), D->TryActivate(MothFoundationTest::Hit(FVector::RightVector)));
		TestTrue(TEXT("Original direction/identity remain locked after body spin"), D->GetActivationDirection().Equals(Expected,.001) && D->GetActivationHitId()==H.HitId);
		D->FinishActivation();
	}
	return true;
}

#define MOTH_FOUNDATION_LATENT(Type, Path, Command) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(Type, Path, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool Type::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothFoundationTest::Command(this)); return true; }
MOTH_FOUNDATION_LATENT(FMothFoundationGround, "MothEffect.Devices.Foundation.GroundQueryCharacterPassageAndForceWake", FGroundFlight)
MOTH_FOUNDATION_LATENT(FMothFoundationDelay, "MothEffect.Devices.Foundation.DefaultDelayForAllThreeEffects", FDelayedEffects)
MOTH_FOUNDATION_LATENT(FMothFoundationPause, "MothEffect.Devices.Foundation.PendingPauseCancellationAndSessionShutdown", FPauseAndCancellation)
MOTH_FOUNDATION_LATENT(FMothFoundationChain, "MothEffect.Devices.Foundation.ExplosionsActivateIndependentDelayedBombs", FExplosionChain)
#undef MOTH_FOUNDATION_LATENT

#endif
