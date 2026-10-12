#if WITH_DEV_AUTOMATION_TESTS
#include "AI/EnemyBase.h"
#include "AIController.h"
#include "Combat/RuleProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Devices/LaunchColumn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Types/MothCollisionChannels.h"

namespace MothArenaRuntimeTest
{
	UWorld* Arena(FAutomationTestBase* T)
	{
		UWorld* W = nullptr;
		for (const auto& Context : GEngine->GetWorldContexts())
			if (Context.WorldType == EWorldType::Game) { W = Context.World(); break; }
		if (!T->TestNotNull(TEXT("Run in -game /Game/MothEffect/Maps/L_Arena_01"), W)) return nullptr;
		return T->TestTrue(TEXT("Actual saved main arena loaded"), W->GetOutermost()->GetName().Contains(TEXT("L_Arena_01"))) ? W : nullptr;
	}
	bool Trace(UWorld* W, FVector From, FVector To, ECollisionChannel Channel, FHitResult& Hit)
	{
		return W->LineTraceSingleByChannel(Hit, From, To, Channel, FCollisionQueryParams(SCENE_QUERY_STAT(MothArena)));
	}
	class FPhysicalSpace final : public IAutomationLatentCommand
	{
	public:
		explicit FPhysicalSpace(FAutomationTestBase* InTest) : T(InTest) {}
		bool Update() override
		{
			if (!W)
			{
				W = Arena(T); if (!W) return true;
				High = Projectile(FVector(-900,600,100), FVector::ForwardVector);
				Low = Projectile(FVector(-500,-1100,130), FVector::RightVector);
				if (!High.IsValid() || !Low.IsValid()) { Cleanup(); return true; }
				High->OnImpactNative.AddLambda([this](ARuleProjectile*, AActor* Actor, const FHitContext&) { HighHit = Actor; });
				Started = W->GetTimeSeconds(); return false;
			}
			if (W->GetTimeSeconds() - Started < .4) return false;
			T->TestTrue(TEXT("Swept P01 actually impacts saved high cover"), HighHit.IsValid() && HighHit->GetActorLocation().Equals(FVector(-600,600,90), 1));
			T->TestTrue(TEXT("P01 above 90cm low cover stays in flight past its far face"), Low.IsValid() && !Low->HasProcessedImpact() && Low->GetActorLocation().Y > -700);
			T->AddInfo(FString::Printf(TEXT("Saved arena physical P01 blocker=%s lowFlight=%s"), *GetNameSafe(HighHit.Get()), Low.IsValid() ? *Low->GetActorLocation().ToString() : TEXT("ended")));
			Cleanup(); return true;
		}
	private:
		ARuleProjectile* Projectile(FVector Position, FVector Direction)
		{
			const FTransform Transform(Position);
			auto* P = W->SpawnActorDeferred<ARuleProjectile>(ARuleProjectile::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!T->TestNotNull(TEXT("Native P01 fixture"), P)) return nullptr;
			if (!P->InitializeProjectile(Direction, nullptr, nullptr)) { T->AddError(TEXT("P01 initialization rejected")); P->Destroy(); return nullptr; }
			P->FinishSpawning(Transform); return P;
		}
		void Cleanup() { if (High.IsValid()) High->Destroy(); if (Low.IsValid()) Low->Destroy(); }
		FAutomationTestBase* T;
		UWorld* W = nullptr;
		TWeakObjectPtr<ARuleProjectile> High, Low;
		TWeakObjectPtr<AActor> HighHit;
		double Started = 0;
	};
	class FWalkRamps final : public IAutomationLatentCommand
	{
	public:
		explicit FWalkRamps(FAutomationTestBase* InTest) : T(InTest) {}
		bool Update() override
		{
			if (!W) { W = Arena(T); if (!W) return true; return BeginSide(); }
			if (!E.IsValid()) { T->AddError(TEXT("Ramp walker ended unexpectedly")); return true; }
			if (W->GetTimeSeconds() - Started < 4) return false;
			const FVector Actual = E->GetActorLocation();
			T->TestTrue(Side == 0 ? TEXT("West ramp physically reaches deck without jumping") : TEXT("East ramp physically reaches deck without jumping"), Actual.Z > 200 && FVector::Dist2D(Actual, Destination) < 75);
			T->AddInfo(FString::Printf(TEXT("Saved arena ramp side=%d actual=%s destination=%s"), Side, *Actual.ToString(), *Destination.ToString()));
			E->Destroy(); E = nullptr;
			if (++Side == 2) return true;
			return BeginSide();
		}
	private:
		bool BeginSide()
		{
			FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			E = W->SpawnActor<AEnemyBase>(FVector(Side == 0 ? 1050 : 2550,1200,92), FRotator::ZeroRotator, Params);
			if (!T->TestNotNull(TEXT("Real CharacterMovement ramp walker"), E.Get())) return true;
			E->SetGameplayEnabled(false); // Isolate navigation from combat, preserving movement/collision.
			auto* AI = Cast<AAIController>(E->GetController());
			if (!T->TestNotNull(TEXT("Formal AIController for ramp path"), AI)) { E->Destroy(); return true; }
			Destination = FVector(Side == 0 ? 1700 : 1900,1200,120);
			AI->MoveToLocation(Destination, 5, true, true, false, true);
			Started = W->GetTimeSeconds(); return false;
		}
		FAutomationTestBase* T;
		UWorld* W = nullptr;
		TWeakObjectPtr<AEnemyBase> E;
		FVector Destination;
		int32 Side = 0;
		double Started = 0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArenaSavedRoutes, "MothEffect.ArenaRuntime.SavedRoutesSupportAndCameraCollision", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothArenaSavedRoutes::RunTest(const FString&)
{
	using namespace MothArenaRuntimeTest;
	UWorld* W = Arena(this); if (!W) return false;
	const FVector Player(-2200,-1800,100);
	const FVector Spawns[] { {-1800,1800,90}, {-800,1800,90}, {800,1800,90}, {-800,-1600,90}, {1800,1800,90}, {2600,200,90}, {1800,-1800,90}, {200,-1800,90} };
	for (int32 I = 0; I < UE_ARRAY_COUNT(Spawns); ++I)
	{
		auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(W, Spawns[I], Player);
		TestTrue(FString::Printf(TEXT("Saved spawn S%d has a complete path to player"), I+1), Path && Path->IsValid() && !Path->IsPartial());
	}
	for (float Y : {-2300.f,2300.f})
	{
		auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(W, FVector(-2700,Y,90), FVector(2700,Y,90));
		const bool bComplete = Path && Path->IsValid() && !Path->IsPartial();
		TestTrue(TEXT("Return lane is connected"), bComplete);
		if (bComplete) for (const FVector& Point : Path->PathPoints)
			TestTrue(TEXT("Return path stays within the unobstructed lane"), FMath::Abs(Point.Y - Y) < 150);
	}
	auto* CoverPath = UNavigationSystemV1::FindPathToLocationSynchronously(W, FVector(-900,600,90), FVector(-300,600,90));
	TestTrue(TEXT("Saved high-cover path bends around a solid obstruction"), CoverPath && CoverPath->IsValid() && !CoverPath->IsPartial() && CoverPath->PathPoints.Num() >= 3);
	FHitResult Hit;
	TestTrue(TEXT("High cover blocks the gameplay projectile channel"), Trace(W, FVector(-900,600,100), FVector(-300,600,100), MothCollision::Projectile, Hit));
	TestFalse(TEXT("90cm low cover permits shoulder-height fire"), Trace(W, FVector(-500,-1100,130), FVector(-500,-500,130), MothCollision::Projectile, Hit));
	TestTrue(TEXT("Outer wall blocks the camera sweep channel"), Trace(W, FVector(-2800,-1800,160), FVector(-3300,-1800,160), ECC_Camera, Hit));
	const auto* Column = GetDefault<ALaunchColumn>();
	for (FVector Base : {FVector(0,0,0), FVector(1800,1200,120), FVector(-2700,2300,0)})
	{
		const bool bGround = Trace(W, Base+FVector(0,0,30), Base-FVector(0,0,30), MothCollision::GroundTrace, Hit);
		TestTrue(TEXT("Arena floor/deck is tagged D01 support"), bGround && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("MothColumnSurface")));
		TestTrue(TEXT("Open overhead permits full D01 column height"), Column->HasStaticClearance(W, Base, nullptr));
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArenaP01Space, "MothEffect.ArenaRuntime.PhysicalProjectilesAgainstCover", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothArenaP01Space::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothArenaRuntimeTest::FPhysicalSpace(this)); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArenaRampWalking, "MothEffect.ArenaRuntime.BothRampsPhysicalWalking", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothArenaRampWalking::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothArenaRuntimeTest::FWalkRamps(this)); return true; }
#endif
