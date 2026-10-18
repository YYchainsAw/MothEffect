#if WITH_DEV_AUTOMATION_TESTS

#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "Types/MothCollisionChannels.h"
#include "UObject/UnrealType.h"

namespace
{
	FHitContext LauncherHit()
	{
		FHitContext Hit;
		Hit.HitId = FGuid::NewGuid();
		Hit.ShotDirection = FVector::RightVector;
		Hit.ImpactPoint = FVector(9000.0);
		return Hit;
	}
	ADeviceBase* Device(FMothCombatTestWorld& F, FVector Location, EDeviceKind Kind = EDeviceKind::Launcher)
	{
		const FTransform Transform(FRotator::ZeroRotator, Location);
		ADeviceBase* Result = F.World->SpawnActorDeferred<ADeviceBase>(ADeviceBase::StaticClass(), Transform,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Result) return nullptr;
		Result->ActivationDelaySeconds = 0.0f; // Test column geometry/settlement independently of the shared delay.
		FEnumProperty* KindProperty = FindFProperty<FEnumProperty>(ADeviceBase::StaticClass(), TEXT("DeviceKind"));
		KindProperty->GetUnderlyingProperty()->SetIntPropertyValue(KindProperty->ContainerPtrToValuePtr<void>(Result), static_cast<int64>(Kind));
		Result->FinishSpawning(Transform);
		F.AgeDeviceForActivation(Result);
		Result->GetPhysicsBody()->SetEnableGravity(false);
		Result->GetPhysicsBody()->SetLinearDamping(0.0f);
		return Result;
	}
	UBoxComponent* Box(FMothCombatTestWorld& F, FVector Position, FVector Extent,
		ECollisionChannel Type = ECC_WorldStatic, bool bTagged = false, AActor* Owner = nullptr)
	{
		if (!Owner) Owner = F.Spawn<AActor>();
		UBoxComponent* Result = NewObject<UBoxComponent>(Owner);
		Owner->AddInstanceComponent(Result);
		if (Owner->GetRootComponent()) Result->SetupAttachment(Owner->GetRootComponent());
		else Owner->SetRootComponent(Result);
		Result->SetBoxExtent(Extent);
		Result->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Result->SetCollisionObjectType(Type);
		Result->SetCollisionResponseToAllChannels(ECR_Block);
		if (bTagged) Result->ComponentTags.Add(TEXT("MothColumnSurface"));
		Result->RegisterComponent();
		Result->SetWorldLocation(Position);
		return Result;
	}
	UBoxComponent* Floor(FMothCombatTestWorld& F)
	{
		return Box(F, FVector(0.0, 0.0, -10.0), FVector(2000.0, 2000.0, 10.0), ECC_WorldStatic, true);
	}
	AMothPlayerAutomationCharacter* Pawn(FMothCombatTestWorld& F, FVector Position)
	{
		AMothPlayerAutomationCharacter* P = F.Spawn<AMothPlayerAutomationCharacter>(Position);
		if (P)
		{
			UCharacterMovementComponent* M = P->GetCharacterMovement();
			M->GravityScale = 0.0f;
			M->BrakingDecelerationFalling = 0.0f;
			M->FallingLateralFriction = 0.0f;
			M->bRunPhysicsWithNoController = true;
			M->SetMovementMode(MOVE_Falling);
		}
		return P;
	}
	ALaunchColumn* Activate(FAutomationTestBase* Test, ADeviceBase* Seed)
	{
		if (!Test->TestNotNull(TEXT("Launcher seed"), Seed)) return nullptr;
		Test->TestTrue(TEXT("Ballistic activation commits once"), Seed->TryActivate(LauncherHit()));
		ALaunchColumn* C = Seed->GetLaunchColumn();
		Test->TestNotNull(TEXT("Qualified floor creates a column"), C);
		return C;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherFailureTest, "MothEffect.Devices.Launcher.InvalidGroundConsumesSeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherFailureTest::RunTest(const FString& Parameters)
{
	for (int32 Case = 0; Case < 4; ++Case)
	{
		FMothCombatTestWorld F;
		if (!TestNotNull(TEXT("Ground validation world"), F.World)) return false;
		if (Case == 1) Box(F, FVector(0.0, 0.0, -1600.0), FVector(500.0, 500.0, 10.0), ECC_WorldStatic, true);
		if (Case == 2) Box(F, FVector(0.0, 0.0, -10.0), FVector(500.0, 500.0, 10.0));
		if (Case == 3) Box(F, FVector(0.0, 0.0, -10.0), FVector(500.0, 500.0, 10.0), ECC_WorldDynamic, true);
		ADeviceBase* S = Device(F, FVector(0.0, 0.0, 100.0));
		if (!S) return false;
		TestTrue(TEXT("Invalid placement still consumes the accepted hit"), S->TryActivate(LauncherHit()));
		TestTrue(TEXT("Failed seed is Spent with finite cleanup"), S->GetDeviceState() == EDeviceState::Spent && S->GetLifeSpan() > 0.0f);
		TestTrue(TEXT("Failure reason separates untagged floor from absent static ground"), S->GetLauncherFailureReason()
			== (Case == 2 ? ELauncherFailureReason::UnsupportedSurface : ELauncherFailureReason::NoGround));
		TestNull(TEXT("Failure never retries or creates a floating column"), S->GetLaunchColumn());
		TestFalse(TEXT("Failed seed cannot be restarted"), S->TryActivate(LauncherHit()));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherAnchorTest, "MothEffect.Devices.Launcher.WorldDownAndFrozenAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherAnchorTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld F;
	if (!F.World) return false;
	UBoxComponent* G = Floor(F);
	G->ComponentTags.Reset();
	G->GetOwner()->Tags.Add(TEXT("MothColumnSurface"));
	// Dynamic blockers and devices cannot become the ground or intercept the static-only probe.
	Box(F, FVector(120.0, 80.0, 500.0), FVector(40.0), ECC_WorldDynamic, true);
	ADeviceBase* S = Device(F, FVector(120.0, 80.0, 900.0));
	if (!S) return false;
	S->SetActorRotation(FRotator(60.0, 70.0, 45.0), ETeleportType::TeleportPhysics);
	S->OnDeviceStateChangedNative.AddLambda([](ADeviceBase* D, EDeviceState Old, EDeviceState New)
		{ if (New == EDeviceState::Active) D->SetActorLocation(FVector(1500.0), false, nullptr, ETeleportType::TeleportPhysics); });
	ALaunchColumn* C = Activate(this, S);
	if (!C) return false;
	TestTrue(TEXT("Probe origin precedes observers and ignores ImpactPoint/local rotation"), S->GetLauncherOrigin().Equals(FVector(120.0, 80.0, 900.0)));
	TestTrue(TEXT("Column uses world-down hit and world-up orientation"), C->GetActorLocation().Equals(FVector(120.0, 80.0, 0.0), 0.1)
		&& C->GetActorRotation().IsNearlyZero());
	TestTrue(TEXT("Active seed has no physics, shot collision or external movement"), !S->GetPhysicsBody()->IsSimulatingPhysics()
		&& S->GetPhysicsBody()->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !S->CanReceiveExternalImpulse());
	TestFalse(TEXT("Repeat hit cannot create another column"), S->TryActivate(LauncherHit()));
	TestEqual(TEXT("Existing column remains the sole logical effect"), S->GetLaunchColumn(), C);
	S->OnDeviceStateChangedNative.Clear();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherSpaceTest, "MothEffect.Devices.Launcher.FullStaticVolumeAndContact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherSpaceTest::RunTest(const FString& Parameters)
{
	for (int32 Case = 0; Case < 4; ++Case)
	{
		FMothCombatTestWorld F;
		if (!F.World) return false;
		UBoxComponent* G = Floor(F);
		if (Case == 1) Box(F, FVector(0.0, 0.0, 300.0), FVector(150.0, 150.0, 10.0), ECC_WorldStatic, false, G->GetOwner());
		if (Case == 2) Box(F, FVector(65.0, 0.0, 200.0), FVector(10.0, 40.0, 120.0));
		if (Case == 3) Box(F, FVector(90.0, 0.0, 200.0), FVector(10.0, 40.0, 120.0));
		ADeviceBase* S = Device(F, FVector(0.0, 0.0, 100.0));
		if (!S) return false;
		S->TryActivate(LauncherHit());
		if (Case == 1 || Case == 2)
		{
			TestTrue(TEXT("Full volume catches ceiling on the same floor actor and side wall"), S->GetLauncherFailureReason() == ELauncherFailureReason::StaticObstruction);
			TestNull(TEXT("Obstructed volume has no column"), S->GetLaunchColumn());
		}
		else TestNotNull(TEXT("Bottom floor contact and geometry outside the full width are allowed"), S->GetLaunchColumn());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherSlopeTest, "MothEffect.Devices.Launcher.SlopeAndPlatformEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherSlopeTest::RunTest(const FString& Parameters)
{
	for (int32 Case = 0; Case < 4; ++Case)
	{
		FMothCombatTestWorld F;
		if (!F.World) return false;
		UBoxComponent* G = Box(F, FVector(0.0, 0.0, -10.0), FVector(Case >= 2 ? 100.0 : 500.0, 500.0, 10.0), ECC_WorldStatic, true);
		if (Case < 2) G->SetWorldRotation(FRotator(Case == 0 ? 20.0 : 60.0, 0.0, 0.0));
		ADeviceBase* S = Device(F, FVector(Case == 2 ? 90.0 : Case == 3 ? 110.0 : 0.0, 0.0, 900.0));
		if (!S) return false;
		S->TryActivate(LauncherHit());
		if (Case == 0)
		{
			ALaunchColumn* C = S->GetLaunchColumn();
			if (!TestNotNull(TEXT("Qualified slope has clearance above the support plane"), C)) return false;
			TestTrue(TEXT("Slope base is raised to highest square-footprint contact while staying world-up"), C->GetActorLocation().Z > 20.0
				&& C->GetActorRotation().IsNearlyZero());
		}
		if (Case == 1) TestTrue(TEXT("Steep surface fails normal threshold"), S->GetLauncherFailureReason() == ELauncherFailureReason::UnsupportedSurface);
		if (Case == 2) TestNotNull(TEXT("Edge with qualified center support and free final volume is allowed"), S->GetLaunchColumn());
		if (Case == 3) TestTrue(TEXT("Outside the platform center probe cannot make a floating platform"), S->GetLauncherFailureReason() == ELauncherFailureReason::NoGround);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherOnceTest, "MothEffect.Devices.Launcher.OncePerTargetAndLateEntrants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherOnceTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld F;
	if (!F.World) return false;
	Floor(F);
	ADeviceBase* S = Device(F, FVector(0.0, 0.0, 900.0));
	ALaunchColumn* C = Activate(this, S);
	AMothPlayerAutomationCharacter* P = Pawn(F, FVector(-40.0, 0.0, 100.0));
	ADeviceBase* D = Device(F, FVector(40.0, 0.0, 20.0), EDeviceKind::Bomb);
	AMothPlayerAutomationCharacter* Late = Pawn(F, FVector(300.0, 0.0, 100.0));
	if (!C || !P || !D || !Late) return false;
	Box(F, P->GetActorLocation(), FVector(10.0), ECC_Pawn, false, P);
	C->Tick(0.05f);
	TestEqual(TEXT("Character and physical device each count once despite extra components"), C->GetAffectedActorCount(), 2);
	C->Tick(0.04f);
	TestEqual(TEXT("Persistent overlap does not relaunch"), C->GetAffectedActorCount(), 2);
	Late->SetActorLocation(FVector(0.0, 0.0, 150.0));
	C->Tick(0.04f);
	TestEqual(TEXT("Later entry into already reached volume launches once"), C->GetAffectedActorCount(), 3);
	C->Tick(0.10f);
	AMothPlayerAutomationCharacter* AfterRise = Pawn(F, FVector(0.0, 0.0, 200.0));
	if (!AfterRise) return false;
	C->Tick(0.05f);
	TestEqual(TEXT("Holding no longer accepts new launch targets"), C->GetAffectedActorCount(), 3);
	TestTrue(TEXT("Launch does not directly activate the bomb"), D->GetDeviceState() == EDeviceState::Dormant);
	TestEqual(TEXT("Column deals no direct damage"), P->GetHealthComponent()->GetCurrentHealth(), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherSupportTest, "MothEffect.Devices.Launcher.WaitForClearanceAndBlockProjectiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherSupportTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld F;
	if (!F.World) return false;
	Floor(F);
	ADeviceBase* S = Device(F, FVector(0.0, 0.0, 900.0));
	ALaunchColumn* C = Activate(this, S);
	if (!C) return false;
	UBoxComponent* Occupant = Box(F, FVector(0.0, 0.0, 175.0), FVector(20.0), ECC_WorldDynamic);
	C->Tick(0.21f);
	TestTrue(TEXT("Full rise waits for a dynamic occupant without crushing it"), C->GetPhase() == ELaunchColumnPhase::Holding && !C->IsSolidSupportEnabled());
	FHitResult Hit;
	TestFalse(TEXT("Unready column cannot block a projectile trace"), F.World->LineTraceSingleByChannel(Hit,
		FVector(-200.0, 60.0, 200.0), FVector(200.0, 60.0, 200.0), MothCollision::Projectile));
	Occupant->SetWorldLocation(FVector(500.0, 0.0, 175.0));
	C->Tick(0.05f);
	TestTrue(TEXT("Holding enables support only after occupancy clears"), C->IsSolidSupportEnabled());
	TestTrue(TEXT("Ready column blocks actual projectile channel query"), F.World->LineTraceSingleByChannel(Hit,
		FVector(-200.0, 60.0, 200.0), FVector(200.0, 60.0, 200.0), MothCollision::Projectile) && Hit.GetActor() == C);
	TestTrue(TEXT("Ready column blocks the rifle channel"), F.World->LineTraceSingleByChannel(Hit,
		FVector(-200.0, 60.0, 200.0), FVector(200.0, 60.0, 200.0), ECC_Visibility) && Hit.GetActor() == C);
	TestTrue(TEXT("Column is ignored by the ground channel"), F.World->LineTraceSingleByChannel(Hit,
		FVector(0.0, 0.0, 500.0), FVector(0.0, 0.0, -100.0), MothCollision::GroundTrace) && Hit.GetActor() != C);
	TestTrue(TEXT("Early finish releases the seed's Active state"), S->FinishActivation());
	TestTrue(TEXT("Retraction disables support immediately, before lowering the visual"), C->GetPhase() == ELaunchColumnPhase::Retracting && !C->IsSolidSupportEnabled());
	C->Tick(0.4f);
	TestNull(TEXT("Ended column releases its source reference"), S->GetLaunchColumn());
	TestTrue(TEXT("Column is destroyed after lowering"), C->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherOccupiedTest, "MothEffect.Devices.Launcher.OccupiedUntilExpiry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherOccupiedTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld F;
	if (!F.World) return false;
	Floor(F);
	ADeviceBase* S = Device(F, FVector(0.0, 0.0, 900.0));
	ALaunchColumn* C = Activate(this, S);
	if (!C) return false;
	Box(F, FVector(0.0, 0.0, 175.0), FVector(20.0), ECC_PhysicsBody);
	C->Tick(0.21f);
	C->Tick(1.5f);
	TestTrue(TEXT("Uncleared volume skips entity support for the whole hold"), !C->IsSolidSupportEnabled() && C->GetPhase() == ELaunchColumnPhase::Retracting);
	Pawn(F, FVector(0.0, 0.0, 100.0));
	C->Tick(0.1f);
	TestEqual(TEXT("Retraction never launches new targets"), C->GetAffectedActorCount(), 0);
	C->Tick(0.3f);
	TestTrue(TEXT("Natural end marks the seed Spent and cleans the column"), S->GetDeviceState() == EDeviceState::Spent && S->GetLaunchColumn() == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherCleanupTest, "MothEffect.Devices.Launcher.PauseCancellationAndEndPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherCleanupTest::RunTest(const FString& Parameters)
{
	for (int32 Case = 0; Case < 4; ++Case)
	{
		FMothCombatTestWorld F;
		if (!F.World) return false;
		Floor(F);
		ADeviceBase* S = Device(F, FVector(0.0, 0.0, 900.0));
		if (!S) return false;
		if (Case == 0) S->OnDeviceStateChangedNative.AddLambda([](ADeviceBase* D, EDeviceState Old, EDeviceState New)
			{ if (New == EDeviceState::Active) D->SetGameplayEnabled(false); });
		S->TryActivate(LauncherHit());
		ALaunchColumn* C = S->GetLaunchColumn();
		if (Case == 0) { TestNull(TEXT("Cancelling observer prevents column creation"), C); continue; }
		if (!TestNotNull(TEXT("Cleanup case column"), C)) return false;
		if (Case == 1)
		{
			F.World->GetWorldSettings()->SetPauserPlayerState(F.Spawn<APlayerState>());
			C->Tick(2.0f);
			TestTrue(TEXT("Paused gameplay freezes column stage"), C->GetPhase() == ELaunchColumnPhase::Rising && !C->IsSolidSupportEnabled());
			F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
			C->Tick(0.21f);
			TestTrue(TEXT("Unpausing resumes stage and enables empty support"), C->IsSolidSupportEnabled());
			S->SetGameplayEnabled(false);
			TestTrue(TEXT("Gameplay disable immediately removes entity collision"), !C->IsSolidSupportEnabled() && S->GetDeviceState() == EDeviceState::Spent);
		}
		if (Case == 2)
		{
			S->DestroyDevice();
			TestTrue(TEXT("Destroying seed destroys its column without orphan collision"), C->IsActorBeingDestroyed() && !C->IsSolidSupportEnabled());
		}
		if (Case == 3)
		{
			C->Destroy();
			TestTrue(TEXT("Independent column EndPlay completes the source exactly once"), S->GetDeviceState() == EDeviceState::Spent && !S->GetLaunchColumn());
			TestFalse(TEXT("Repeated completion stays rejected"), S->FinishActivation());
		}
	}
	return true;
}

namespace
{
	class FLauncherMovement final : public IAutomationLatentCommand
	{
	public:
		explicit FLauncherMovement(FAutomationTestBase* InTest, bool bInMerge) : Test(InTest), bMerge(bInMerge) {}
		bool Update() override
		{
			if (!F)
			{
				F = MakeUnique<FMothCombatTestWorld>();
				if (!F->World) { Test->AddError(TEXT("Create launcher movement world")); return true; }
				Floor(*F);
				S = Device(*F, FVector(0.0, 0.0, 900.0));
				C = Activate(Test, S);
				P = Pawn(*F, FVector(-40.0, 0.0, 100.0));
				D = Device(*F, FVector(40.0, 0.0, 50.0), EDeviceKind::Emitter);
				if (!C || !P || !D) { Test->AddError(TEXT("Spawn movement targets")); return true; }
				Test->TestTrue(TEXT("Activate emitter before the column moves it"), D->TryActivate(LauncherHit()));
				D->GetPhysicsBody()->SetEnableGravity(false);
				P->GetCharacterMovement()->Velocity = FVector(100.0, 200.0, -50.0);
				D->GetPhysicsBody()->SetPhysicsLinearVelocity(FVector(100.0, 200.0, -50.0));
				ExpectedPawn = FVector(100.0, 200.0, 1000.0);
				ExpectedDevice = FVector(100.0, 200.0, 1200.0);
				if (bMerge)
				{
					ADeviceBase* B = Device(*F, FVector(300.0, 0.0, 150.0), EDeviceKind::Bomb);
					if (!B) return true;
					ExpectedPawn = FVector(-550.0, 200.0, 1000.0).GetClampedToMaxSize(1600.0);
					ExpectedDevice = FVector(100.0, 200.0, -50.0) + (D->GetActorLocation() - B->GetActorLocation()).GetSafeNormal() * 1100.0;
					ExpectedDevice.Z = 1200.0;
					ExpectedDevice = ExpectedDevice.GetClampedToMaxSize(1600.0);
					if (Order == 0) { B->TryActivate(LauncherHit()); C->Tick(0.05f); }
					else { C->Tick(0.05f); B->TryActivate(LauncherHit()); }
				}
				else C->Tick(0.05f);
				FWorldDelegates::OnWorldPostActorTick.Broadcast(F->World, LEVELTICK_All, 0.0f);
				return false;
			}
			if (F->World->GetTimerManager().HasBeenTickedThisFrame()) return false;
			F->World->Tick(LEVELTICK_All, 0.01f);
			++Frames;
			if (Frames == 2)
			{
				Test->AddInfo(FString::Printf(TEXT("Order=%d pawn=%s expected=%s device=%s expected=%s affected=%d"), Order,
					*P->GetVelocity().ToString(), *ExpectedPawn.ToString(), *D->GetPhysicsBody()->GetPhysicsLinearVelocity().ToString(),
					*ExpectedDevice.ToString(), C->GetAffectedActorCount()));
				Test->TestTrue(TEXT("Actual CharacterMovement preserves XY and overrides Z after blast merge"), P->GetVelocity().Equals(ExpectedPawn, 1.0));
				Test->TestTrue(TEXT("Actual physical root uses device Z and the shared final clamp"), D->GetPhysicsBody()->GetPhysicsLinearVelocity().Equals(ExpectedDevice, 1.0));
				Test->TestTrue(TEXT("Active emitter keeps its locked ballistic world direction"), D->GetDeviceState() == EDeviceState::Active && D->GetActivationDirection().Equals(FVector::RightVector));
				if (bMerge)
				{
					F.Reset();
					Frames = 0;
					if (++Order == 2) return true;
					return false;
				}
			}
			if (bAwaitRelease)
			{
				Test->TestFalse(TEXT("Retraction releases the standing character's support"), C->IsSolidSupportEnabled());
				Test->TestTrue(TEXT("Losing support returns movement to falling"), P->GetCharacterMovement()->IsFalling());
				return true;
			}
			if (Frames == 45)
			{
				Test->TestTrue(TEXT("Launch targets leave volume before entity support enables"), C->IsSolidSupportEnabled());
				P->SetActorLocation(FVector(0.0, 0.0, 540.0), false, nullptr, ETeleportType::TeleportPhysics);
				P->GetCharacterMovement()->Velocity = FVector::ZeroVector;
				P->GetCharacterMovement()->GravityScale = 1.0f;
			}
			if (Frames > 45 && P->GetCharacterMovement()->IsMovingOnGround())
			{
				Test->TestEqual(TEXT("Real character lands on ready column support"), P->GetCharacterMovement()->CurrentFloor.HitResult.GetActor(), static_cast<AActor*>(C));
				S->FinishActivation();
				bAwaitRelease = true;
				return false;
			}
			if (Frames > 110) { Test->AddError(TEXT("Character failed to land on the column during hold")); return true; }
			return false;
		}
	private:
		FAutomationTestBase* Test;
		bool bMerge;
		TUniquePtr<FMothCombatTestWorld> F;
		ADeviceBase* S = nullptr;
		ALaunchColumn* C = nullptr;
		AMothPlayerAutomationCharacter* P = nullptr;
		ADeviceBase* D = nullptr;
		int32 Frames = 0;
		int32 Order = 0;
		bool bAwaitRelease = false;
		FVector ExpectedPawn;
		FVector ExpectedDevice;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherMotionTest, "MothEffect.Devices.Launcher.PhysicalMovementAndStandingSupport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherMotionTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FLauncherMovement(this, false));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherMergeTest, "MothEffect.Devices.Launcher.BombColumnOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherMergeTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FLauncherMovement(this, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothLauncherBlueprintTest, "MothEffect.Devices.Launcher.SavedBlueprintIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothLauncherBlueprintTest::RunTest(const FString& Parameters)
{
	auto SharedFixture = MakeShared<FMothCombatTestWorld>();
	auto& F = *SharedFixture;
	if (!F.World) return false;
	Floor(F);
	UClass* Class = LoadClass<ADeviceBase>(nullptr, TEXT("/Game/MothEffect/Devices/BP_DeviceLauncher.BP_DeviceLauncher_C"));
	if (!TestNotNull(TEXT("Load saved launcher asset"), Class)) return false;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADeviceBase* S = F.World->SpawnActor<ADeviceBase>(Class, FVector(0.0, 0.0, 900.0), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Saved seed"), S)) return false;
	TestTrue(TEXT("Seed retains Launcher kind"), S->GetDeviceKind() == EDeviceKind::Launcher);
	TestEqual(TEXT("Saved seed inherits the shared .3s activation delay"), S->ActivationDelaySeconds, 0.3f);
	F.AgeDeviceForActivation(S);
	TestTrue(TEXT("Saved seed commits its accepted hit"), S->TryActivate(LauncherHit()));
	TestNull(TEXT("No column exists before the default delay"), S->GetLaunchColumn());
	ADD_LATENT_AUTOMATION_COMMAND(FMothTickWorldUntil(SharedFixture, 0.35f, [this, S]()
	{
		auto* C = S->GetLaunchColumn();
		if (!TestNotNull(TEXT("Delayed saved seed creates a column"), C)) return;
		TestEqual(TEXT("Saved seed spawns the saved column Blueprint"), C->GetClass()->GetPathName(), FString(TEXT("/Game/MothEffect/Devices/BP_LaunchColumn.BP_LaunchColumn_C")));
		TestEqual(TEXT("Column defaults retain baseline height"), C->GetColumnHeightCm(), 350.0f);
		TestEqual(TEXT("Column defaults retain baseline half-width"), C->GetColumnRadiusCm(), 70.0f);
		C->Tick(0.21f);
		TestTrue(TEXT("Saved Blueprint uses native clear support phase"), C->IsSolidSupportEnabled());
		S->FinishActivation();
		TestFalse(TEXT("Saved Blueprint retracts safely"), C->IsSolidSupportEnabled());
	}));
	return true;
}

#endif
