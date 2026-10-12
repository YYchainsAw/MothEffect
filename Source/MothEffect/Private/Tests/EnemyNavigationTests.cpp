#if WITH_DEV_AUTOMATION_TESTS
#include "AI/EnemyBase.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Tests/PlayerAutomationCharacter.h"

namespace MothEnemyNavTest
{
	class FNavRuntime final : public IAutomationLatentCommand
	{
	public:
		FNavRuntime(FAutomationTestBase* InTest, bool bInRecover) : T(InTest), bRecover(bInRecover) {}
		bool Update() override
		{
			if (!World) return Setup();
			const double Elapsed = World->GetTimeSeconds() - StartedAt; // Engine owns game-world ticks.
			if (!IsValid(E.Get()) || !IsValid(Target.Get())) { T->AddError(TEXT("Navigation actors ended unexpectedly")); Cleanup(); return true; }
			if (bRecover && !Wall.IsValid() && Elapsed >= 0.2)
			{
				FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Wall = World->SpawnActor<AActor>(Params);
				auto* Box = NewObject<UBoxComponent>(Wall.Get());
				Wall->AddInstanceComponent(Box); Wall->SetRootComponent(Box);
				Box->SetBoxExtent(FVector(30, 220, 200));
				Box->SetCollisionObjectType(ECC_WorldDynamic); Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Box->SetCollisionResponseToAllChannels(ECR_Block); Box->SetCanEverAffectNavigation(false);
				Box->RegisterComponent(); Box->SetWorldLocation(FVector(1000, -750, 200));
			}
			if (Elapsed < (bRecover ? 7.0 : 2.0)) return false;
			T->TestTrue(TEXT("Formal AIController issues navigation requests"), Cast<AAIController>(E->GetController()) && E->GetNavigationRequests() > 0);
			T->TestTrue(TEXT("Real CharacterMovement advances in saved NavMesh world"), FVector::Dist2D(E->GetActorLocation(), StartLocation) > 150);
			if (bRecover)
			{
				T->TestTrue(TEXT("Temporary non-nav obstruction triggers timed recovery"), E->GetStallRecoveries() > 0);
				T->TestTrue(TEXT("Recovery requests alternate reachable positions"), E->GetNavigationRequests() >= 3);
				T->TestTrue(TEXT("Recovery produces lateral physical movement around obstruction"), FMath::Abs(E->GetActorLocation().Y - StartLocation.Y) > 200);
			}
			T->AddInfo(FString::Printf(TEXT("Saved range navigation recover=%d elapsed=%.3f from=%s to=%s requests=%d stalls=%d"), bRecover, Elapsed,
				*StartLocation.ToString(), *E->GetActorLocation().ToString(), E->GetNavigationRequests(), E->GetStallRecoveries()));
			T->AddInfo(FString::Printf(TEXT("Navigation diagnostics combat=%d phase=%s target=%s sight=%d controller=%s targetEnabled=%d targetHealth=%.1f tick=%d"),
				E->IsCombatEnabled(), *UEnum::GetValueAsString(E->GetCombatPhase()), *GetNameSafe(E->GetCombatTarget()), E->HasTargetSight(),
				*GetNameSafe(E->GetController()), Target->IsGameplayEnabled(), Target->FindComponentByClass<UHealthComponent>()->GetCurrentHealth(), E->IsActorTickEnabled()));
			FHitResult SightHit; FCollisionQueryParams SightParams(SCENE_QUERY_STAT(MothNavDiagnostics), false, E.Get());
			World->LineTraceSingleByChannel(SightHit, E->GetPawnViewLocation(), Target->GetPawnViewLocation(), ECC_Visibility, SightParams);
			T->AddInfo(FString::Printf(TEXT("Navigation sight blocker=%s component=%s origin=%s targetEye=%s targetPosition=%s impact=%s"),
				*GetNameSafe(SightHit.GetActor()), *GetNameSafe(SightHit.GetComponent()), *E->GetPawnViewLocation().ToString(),
				*Target->GetPawnViewLocation().ToString(), *Target->GetActorLocation().ToString(), *SightHit.ImpactPoint.ToString()));
			Cleanup(); return true;
		}
	private:
		bool Setup()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts()) if (Context.WorldType == EWorldType::Game) { World = Context.World(); break; }
			if (!T->TestNotNull(TEXT("Run with -game /Game/MothEffect/Tests/L_T14AI"), World)) return true;
			if (!T->TestTrue(TEXT("Game actually loaded the saved T14 range"), World->GetOutermost()->GetName().Contains(TEXT("L_T14AI")))) { World = nullptr; return true; }
			// The placed shooter may move into this fixture's sight line during startup.
			// Disabling AI alone retains its blocking capsule; isolate both gameplay and collision.
			for (TActorIterator<AEnemyBase> It(World); It; ++It) { It->SetGameplayEnabled(false); It->SetActorEnableCollision(false); }
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!T->TestNotNull(TEXT("Saved game-world navigation system"), Nav)) return true;
			UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, FVector(1000, 0, 90), FVector(-1000, 0, 90));
			if (!T->TestTrue(TEXT("Saved static cover has a complete navigable route"), Path && Path->IsValid() && !Path->IsPartial())) return true;
			T->TestTrue(TEXT("Path bends around cover rather than crossing it"), Path->PathPoints.Num() >= 3);
			StartLocation = FVector(1200, -750, 90);
			FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			E = World->SpawnActor<AEnemyBase>(StartLocation, FRotator::ZeroRotator, Params);
			const FTransform Transform(FVector(-800, -750, 90));
			Target = World->SpawnActorDeferred<AMothPlayerAutomationCharacter>(AMothPlayerAutomationCharacter::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!T->TestNotNull(TEXT("Navigation enemy"), E.Get()) || !T->TestNotNull(TEXT("Navigation target"), Target.Get())) { Cleanup(); return true; }
			Target->FindComponentByClass<UHealthComponent>()->ConfigureMaxHealth(1000); Target->FinishSpawning(Transform);
			T->TestTrue(TEXT("Living navigation target is accepted"), E->SetCombatTarget(Target.Get()));
			T->TestTrue(TEXT("Controlled navigation fixture starts with clear target sight"), E->HasTargetSight());
			StartedAt = World->GetTimeSeconds();
			return false;
		}
		void Cleanup()
		{
			if (E.IsValid()) E->Destroy(); if (Target.IsValid()) Target->Destroy(); if (Wall.IsValid()) Wall->Destroy();
		}
		FAutomationTestBase* T; bool bRecover;
		UWorld* World = nullptr;
		TWeakObjectPtr<AEnemyBase> E;
		TWeakObjectPtr<AMothPlayerAutomationCharacter> Target;
		TWeakObjectPtr<AActor> Wall;
		FVector StartLocation;
		double StartedAt = 0;
	};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothSavedNavPursuit, "MothEffect.EnemyNavigation.SavedCoverRouteAndPursuit", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothSavedNavPursuit::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothEnemyNavTest::FNavRuntime(this, false)); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothSavedNavStall, "MothEffect.EnemyNavigation.TemporaryObstructionRecovery", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMothSavedNavStall::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothEnemyNavTest::FNavRuntime(this, true)); return true; }
#endif
