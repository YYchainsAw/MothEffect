#if WITH_DEV_AUTOMATION_TESTS
#include "Framework/MothWaveDirector.h"
#include "AI/EnemyBase.h"
#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/ExternalImpulseComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "Tests/WaveFlowObserver.h"
#include "TimerManager.h"
#include "Weapons/Rifle.h"

namespace MothFlowTest
{
	FHitContext Hit(float Damage = 100) { FHitContext H; H.HitId = FGuid::NewGuid(); H.Damage = Damage; H.ShotDirection = FVector::ForwardVector; return H; }
	struct FFixture
	{
		FMothCombatTestWorld F;
		AMothPlayerAutomationCharacter* Player = nullptr;
		AMothWaveDirector* Director = nullptr;
		UDeviceLifecycleSubsystem* Manager = nullptr;
		FFixture()
		{
			Manager = F.World->GetSubsystem<UDeviceLifecycleSubsystem>();
			Player = F.Spawn<AMothPlayerAutomationCharacter>(FVector(-2200,-1800,100));
			Player->GetCharacterMovement()->GravityScale = 0;
			auto* Class = LoadClass<AMothWaveDirector>(nullptr, TEXT("/Game/MothEffect/Framework/BP_WaveDirector.BP_WaveDirector_C"));
			if (Class) Director = F.World->SpawnActor<AMothWaveDirector>(Class);
			if (Director) Director->SetPlayer(Player);
		}
	};
	ADeviceBase* Device(FFixture& F, int32 Kind, FVector Position)
	{
		const TCHAR* Names[] { TEXT("Launcher"), TEXT("Bomb"), TEXT("Emitter") };
		auto* Class = LoadClass<ADeviceBase>(nullptr, *FString::Printf(TEXT("/Game/MothEffect/Devices/Drops/BP_Drop%s.BP_Drop%s_C"), Names[Kind], Names[Kind]));
		auto* Result = F.Manager->SpawnDevice(Class, FTransform(Position));
		F.F.AgeDeviceForActivation(Result);
		if (Result) Result->ActivationDelaySeconds = 0.0f; // Isolate transition cleanup with an already-created effect.
		return Result;
	}
	void Floor(FFixture& F)
	{
		auto* A = F.F.Spawn<AActor>(); auto* B = NewObject<UBoxComponent>(A);
		A->AddInstanceComponent(B); A->SetRootComponent(B);
		B->SetBoxExtent(FVector(3000,2500,10)); B->SetCollisionObjectType(ECC_WorldStatic);
		B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); B->SetCollisionResponseToAllChannels(ECR_Block);
		B->ComponentTags.Add(TEXT("MothColumnSurface")); B->RegisterComponent(); B->SetWorldLocation(FVector(0,0,-10));
	}
	int32 CombatObjects(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
			if (!It->IsActorBeingDestroyed() && (It->IsA<AEnemyBase>() || It->IsA<ADeviceBase>() || It->IsA<ARuleProjectile>() || It->IsA<ALaunchColumn>())) ++Count;
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFlowTutorialReset, "MothEffect.WaveFlow.TutorialResetBeforePreparation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFlowTutorialReset::RunTest(const FString&)
{
	MothFlowTest::FFixture F;
	if (!TestNotNull(TEXT("Saved director"), F.Director)) return false;
	MothFlowTest::Floor(F);
	auto* Held = MothFlowTest::Device(F, 2, F.Player->GetActorLocation()+FVector(140,0,0));
	TestTrue(TEXT("Actual held tutorial device"), F.Player->TryPickupDevice(Held));
	auto* Emitter = MothFlowTest::Device(F, 2, FVector(0,0,100));
	Emitter->TryActivate(MothFlowTest::Hit());
	auto* Column = MothFlowTest::Device(F, 0, FVector(1000,0,30));
	TestTrue(TEXT("Actual active tutorial column"), Column->TryActivate(MothFlowTest::Hit()));
	TestNotNull(TEXT("Column actor exists before transition"), Column->GetLaunchColumn());
	F.F.Spawn<ARuleProjectile>(FVector(2000,0,500));
	F.F.Spawn<AEnemyBase>(FVector(0,1000,92));
	TestTrue(TEXT("Start after tutorial succeeds"), F.Director->StartEncounter());
	TestTrue(TEXT("Preparation precedes first birth"), F.Director->GetPhase() == EMothFlowPhase::Preparing);
	TestEqual(TEXT("All first-wave births are queued during preparation"), F.Director->GetPendingCount(), 4);
	TestEqual(TEXT("Five seconds preparation"), F.Director->GetPreparationRemaining(), 5.0f);
	TestEqual(TEXT("Tutorial combat actors removed"), MothFlowTest::CombatObjects(F.F.World), 0);
	TestNull(TEXT("Holder cleared"), F.Player->GetHeldDevice());
	TestEqual(TEXT("Device count starts from zero"), F.Manager->GetDeviceCount(), 0);
	TestEqual(TEXT("Projectile count starts from zero"), F.Manager->GetProjectileCount(), 0);
	TestTrue(TEXT("Clean formal session opens admission"), F.Manager->IsGameplayEnabled() && F.Player->IsGameplayEnabled());
	TestFalse(TEXT("Duplicate start cannot reset a running wave"), F.Director->StartEncounter());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFlowDeathClosure, "MothEffect.WaveFlow.DeathClosesDamageSpawnsAndReentrantCallbacks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFlowDeathClosure::RunTest(const FString&)
{
	MothFlowTest::FFixture F;
	if (!TestNotNull(TEXT("Saved director"), F.Director)) return false;
	TestTrue(TEXT("Formal flow starts"), F.Director->StartEncounter());
	auto* Held = MothFlowTest::Device(F, 2, F.Player->GetActorLocation()+FVector(140,0,0));
	TestTrue(TEXT("Held fixture"), F.Player->TryPickupDevice(Held));
	auto* D = MothFlowTest::Device(F, 2, FVector(0,0,100)); D->TryActivate(MothFlowTest::Hit());
	auto* Victim = F.F.Spawn<AEnemyBase>(FVector(0,800,92));
	bool bCallback = false, bAdmitted = false, bDamaged = false;
	D->OnDeviceStateChangedNative.AddLambda([&](ADeviceBase*, EDeviceState, EDeviceState State)
	{
		if (State != EDeviceState::Destroyed) return;
		bCallback = true;
		bAdmitted = F.Manager->SpawnDevice(ADeviceBase::StaticClass(), FTransform(FVector(0,0,200))) != nullptr;
		bDamaged = Victim->GetHealthComponent()->ApplyHit(MothFlowTest::Hit(10));
	});
	TestTrue(TEXT("Player receives lethal damage"), F.Player->FindComponentByClass<UHealthComponent>()->ApplyHit(MothFlowTest::Hit()));
	TestTrue(TEXT("Death commits defeat immediately"), F.Director->GetPhase() == EMothFlowPhase::Defeat);
	TestTrue(TEXT("Destroyed-device callback actually executed"), bCallback);
	TestFalse(TEXT("Callbacks cannot spawn after terminal commitment"), bAdmitted);
	TestFalse(TEXT("Direct component damage is closed before cleanup callbacks"), bDamaged);
	TestFalse(TEXT("Projectile admission closed"), F.Manager->CanSpawnProjectile());
	TestNull(TEXT("Held ownership cleared"), F.Player->GetHeldDevice());
	TestEqual(TEXT("All terminal combat objects removed"), MothFlowTest::CombatObjects(F.F.World), 0);
	TestEqual(TEXT("Terminal pending count zero"), F.Director->GetPendingCount(), 0);
	TestFalse(TEXT("Terminal start cannot resurrect gameplay"), F.Director->StartEncounter());
	TestFalse(TEXT("Rifle cannot start after result"), F.Player->GetRifle()->TryStartFire());
	TestFalse(TEXT("Direct external impulse is also gated"), F.Player->FindComponentByClass<UExternalImpulseComponent>()->QueueImpulse(FVector(100,0,100), false, FGuid::NewGuid()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFlowPlayerBounds, "MothEffect.WaveFlow.PlayerBoundsFailWithoutWaitingForHealthDeath", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFlowPlayerBounds::RunTest(const FString&)
{
	MothFlowTest::FFixture F;
	if (!TestNotNull(TEXT("Saved director"), F.Director)) return false;
	F.Director->StartEncounter();
	F.Player->SetActorLocation(FVector(3001,0,100)); F.Director->Tick(.01f);
	TestTrue(TEXT("Outside safe volume commits defeat"), F.Director->GetPhase() == EMothFlowPhase::Defeat);
	TestTrue(TEXT("Bounds failure does not invent lethal damage"), F.Player->FindComponentByClass<UHealthComponent>()->IsAlive());
	TestFalse(TEXT("Living player is nevertheless frozen"), F.Player->IsGameplayEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFlowPlayerRemoved, "MothEffect.WaveFlow.PlayerRemovalAndDirectorTeardownClearBindings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFlowPlayerRemoved::RunTest(const FString&)
{
	MothFlowTest::FFixture F;
	if (!TestNotNull(TEXT("Saved director"), F.Director)) return false;
	F.Director->StartEncounter(); F.Player->Destroy();
	TestTrue(TEXT("Player EndPlay commits defeat"), F.Director->GetPhase() == EMothFlowPhase::Defeat);
	TestEqual(TEXT("Removal clears pending births"), F.Director->GetPendingCount(), 0);
	F.Director->Destroy();
	TestFalse(TEXT("Old world remains closed during teardown"), F.Manager->IsGameplayEnabled());
	TestFalse(TEXT("Destroyed director refuses new session"), F.Director->StartEncounter());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFlowWorldIsolation, "MothEffect.WaveFlow.TerminalDamageGateIsWorldLocal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFlowWorldIsolation::RunTest(const FString&)
{
	MothFlowTest::FFixture A, B;
	if (!TestNotNull(TEXT("First director"), A.Director) || !TestNotNull(TEXT("Second director"), B.Director)) return false;
	A.Director->StartEncounter(); B.Director->StartEncounter();
	A.Player->FindComponentByClass<UHealthComponent>()->ApplyHit(MothFlowTest::Hit());
	TestFalse(TEXT("First session closed"), A.Manager->IsGameplayEnabled());
	TestTrue(TEXT("Other session still admits objects"), B.Manager->IsGameplayEnabled() && B.Manager->CanSpawnProjectile());
	TestTrue(TEXT("Other session still accepts damage"), B.Player->FindComponentByClass<UHealthComponent>()->ApplyHit(MothFlowTest::Hit(10)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFlowFatalPresentation, "MothEffect.WaveFlow.FatalHealthPresentationObservesClosedSession", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFlowFatalPresentation::RunTest(const FString&)
{
	MothFlowTest::FFixture F;
	if (!TestNotNull(TEXT("Saved director"), F.Director)) return false;
	F.Director->StartEncounter();
	auto* Other = F.F.Spawn<AMothPlayerAutomationCharacter>(FVector(1000,0,100));
	auto* Observer = NewObject<UMothWaveFlowObserver>(); Observer->Director = F.Director; Observer->OtherHealth = Other->GetHealthComponent();
	F.Player->GetHealthComponent()->OnHealthChanged.AddDynamic(Observer, &UMothWaveFlowObserver::HealthChanged);
	TestTrue(TEXT("Fatal hit applies"), F.Player->GetHealthComponent()->ApplyHit(MothFlowTest::Hit()));
	TestTrue(TEXT("Actual dynamic health presentation executed"), Observer->bNotified);
	TestTrue(TEXT("Defeat was committed before health presentation"), Observer->bTerminal);
	TestFalse(TEXT("Health presentation cannot damage another player"), Observer->bDamaged);
	TestFalse(TEXT("Health presentation cannot generate a replacement"), Observer->bSpawned);
	TestEqual(TEXT("Other player remains at full health"), Other->GetHealthComponent()->GetCurrentHealth(), 100.0f);
	F.Player->GetHealthComponent()->OnHealthChanged.RemoveDynamic(Observer, &UMothWaveFlowObserver::HealthChanged);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFlowDuplicateDirector, "MothEffect.WaveFlow.DuplicateDirectorCannotCloseActiveSession", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothFlowDuplicateDirector::RunTest(const FString&)
{
	MothFlowTest::FFixture F;
	if (!TestNotNull(TEXT("Saved director"), F.Director)) return false;
	F.Director->StartEncounter();
	auto* Duplicate = F.F.Spawn<AMothWaveDirector>();
	TestTrue(TEXT("Duplicate is destroyed without tearing down owner"), !IsValid(Duplicate) || Duplicate->IsActorBeingDestroyed());
	TestTrue(TEXT("Original remains in preparation"), F.Director->GetPhase() == EMothFlowPhase::Preparing && F.Manager->IsGameplayEnabled());
	TestEqual(TEXT("Original pending counts retained"), F.Director->GetPendingCount(), 4);
	return true;
}
#endif
