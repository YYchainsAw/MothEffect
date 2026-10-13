#if WITH_DEV_AUTOMATION_TESTS
#include "AI/EnemyBase.h"
#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Devices/DeviceBase.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Materials/MaterialInterface.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "Weapons/Rifle.h"

namespace MothDropTest
{
	const TCHAR* Names[] { TEXT("Launcher"), TEXT("Bomb"), TEXT("Emitter") };
	UClass* EnemyClass(int32 Kind) { return LoadClass<AEnemyBase>(nullptr, *FString::Printf(TEXT("/Game/MothEffect/Enemies/BP_Enemy%s.BP_Enemy%s_C"), Names[Kind], Names[Kind])); }
	UClass* DeviceClass(int32 Kind) { return LoadClass<ADeviceBase>(nullptr, *FString::Printf(TEXT("/Game/MothEffect/Devices/Drops/BP_Drop%s.BP_Drop%s_C"), Names[Kind], Names[Kind])); }
	FHitContext Hit(float Damage = 60) { FHitContext H; H.HitId = FGuid::NewGuid(); H.Damage = Damage; H.ShotDirection = FVector::ForwardVector; return H; }
	UDeviceLifecycleSubsystem* Manager(FMothCombatTestWorld& F) { return F.World->GetSubsystem<UDeviceLifecycleSubsystem>(); }
	AActor* Box(FMothCombatTestWorld& F, FVector Position, FVector Extent, FRotator Rotation = FRotator::ZeroRotator)
	{
		auto* A = F.Spawn<AActor>(); auto* B = NewObject<UBoxComponent>(A);
		A->AddInstanceComponent(B); A->SetRootComponent(B); B->SetBoxExtent(Extent);
		B->SetCollisionObjectType(ECC_WorldStatic); B->SetCollisionResponseToAllChannels(ECR_Block);
		B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); B->RegisterComponent();
		B->SetWorldLocationAndRotation(Position, Rotation); return A;
	}
	void Floor(FMothCombatTestWorld& F) { Box(F, FVector(0, 0, -10), FVector(18000, 2000, 10)); }
	AEnemyBase* Enemy(FMothCombatTestWorld& F, int32 Kind, FVector Position = FVector(0, 0, 90))
	{
		UClass* Class = EnemyClass(Kind); if (!Class) return nullptr;
		FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* E = F.World->SpawnActor<AEnemyBase>(Class, Position, FRotator::ZeroRotator, P);
		if (E) { E->GetCharacterMovement()->DisableMovement(); E->SetCombatTarget(nullptr); }
		return E;
	}
	AMothPlayerAutomationCharacter* Player(FMothCombatTestWorld& F, FVector Position) { return F.Spawn<AMothPlayerAutomationCharacter>(Position); }

	enum class ECase { Rifle, Projectile, Terrain };
	class FDropFlight final : public IAutomationLatentCommand
	{
	public:
		FDropFlight(FAutomationTestBase* InTest, ECase InCase) : T(InTest), Case(InCase) {}
		bool Update() override
		{
			if (!F) return Setup();
			if (F->World->GetTimerManager().HasBeenTickedThisFrame()) return false;
			F->World->Tick(LEVELTICK_All, .01f);
			if (F->World->GetTimeSeconds() < (Case == ECase::Terrain ? .55f : .65f)) return false;
			if (Case != ECase::Terrain)
			{
				if (P) { P->GetRifle()->StopFire(); P->SetPrimaryDown(false); }
				T->TestEqual(TEXT("Real weapon/projectile kills the full 60HP variant"), E->GetHealthComponent()->GetCurrentHealth(), 0.0f);
				T->TestEqual(TEXT("Real kill produces one managed device"), Manager(*F)->GetDeviceCount(), 1);
				T->TestEqual(TEXT("One drop attempt per enemy"), E->GetDropAttempts(), 1);
				D = E->GetDroppedDevice();
			}
			if (T->TestNotNull(TEXT("Actual Dormant physical drop survives enemy feedback cleanup"), D))
			{
				T->TestTrue(TEXT("Drop type matches fixed variant"), D->GetDeviceKind() == static_cast<EDeviceKind>(Index % 3));
				T->TestTrue(TEXT("Fresh drop remains Dormant and physically simulated"), D->GetDeviceState() == EDeviceState::Dormant && D->GetPhysicsBody()->IsSimulatingPhysics());
				if (Case == ECase::Terrain)
				{
					if (Index == 3) { T->TestTrue(TEXT("Unsupported airborne birth falls under real gravity"), D->GetActorLocation().Z < 2960 && D->GetPhysicsBody()->GetPhysicsLinearVelocity().Z < -100); }
					else
					{
						T->TestTrue(TEXT("Near-death placement stays within 150cm horizontally"), D->GetActorLocation().Size2D() <= 150);
						auto* Holder = Player(*F, D->GetActorLocation() + FVector(0, -120, 90));
						T->TestTrue(TEXT("Actual player pickup works after physical settling on floor/wall/slope"), Holder->TryPickupDevice(D));
						T->TestTrue(TEXT("Pickup commits real Held ownership"), D->GetDeviceState() == EDeviceState::Held && Holder->GetHeldDevice() == D);
					}
				}
				T->AddInfo(FString::Printf(TEXT("Drop case=%d variant=%d position=%s type=%s count=%d"), static_cast<int32>(Case), Index, *D->GetActorLocation().ToString(), *UEnum::GetValueAsString(D->GetDeviceKind()), Manager(*F)->GetDeviceCount()));
			}
			F.Reset(); E = nullptr; P = nullptr; D = nullptr;
			return ++Index >= (Case == ECase::Terrain ? 4 : 3);
		}
	private:
		bool Setup()
		{
			F = MakeUnique<FMothCombatTestWorld>();
			if (!T->TestNotNull(TEXT("Drop test World"), F->World)) return true;
			if (Case == ECase::Terrain && Index == 2) Box(*F, FVector::ZeroVector, FVector(1200, 800, 20), FRotator(25, 0, 0)); else Floor(*F);
			if (Case == ECase::Terrain && Index == 0) F->Spawn<ADeviceBase>(FVector(0, 0, 20));
			if (Case == ECase::Terrain && Index == 1) Box(*F, FVector(55, 0, 180), FVector(20, 600, 180));
			const FVector Location = Case != ECase::Terrain ? FVector(600, 0, 90) : FVector(0, 0, Index == 3 ? 3000 : Index == 2 ? 150 : 90);
			E = Enemy(*F, Index % 3, Location);
			if (!T->TestNotNull(TEXT("Saved fixed-drop enemy variant"), E)) return true;
			if (Case == ECase::Terrain)
			{
				T->TestTrue(TEXT("Health death commits"), IBallisticReactive::Execute_ReceiveBallisticHit(E, Hit())); D = E->GetDroppedDevice();
				if (Index == 0 && D) T->TestTrue(TEXT("Existing device forces a free neighboring birthplace"), D->GetActorLocation().Size2D() > 30);
			}
			else if (Case == ECase::Rifle)
			{
				P = Player(*F, FVector(0, 0, 90));
				auto* Muzzle = NewObject<USceneComponent>(P); P->AddInstanceComponent(Muzzle); Muzzle->RegisterComponent(); Muzzle->SetWorldLocation(FVector(60, 0, 150));
				P->GetRifle()->SetMuzzleSource(Muzzle, NAME_None);
				T->AddExpectedError(FString::Printf(TEXT("could not play on %s. Assign a Montage compatible"), *P->GetName()), EAutomationExpectedErrorFlags::Contains, 1);
				P->SetPrimaryDown(true); P->DoPrimaryStart();
			}
			else for (int32 I = 0; I < 6; ++I)
			{
				const FTransform X(FVector(0, 0, 150));
				auto* Shot = F->World->SpawnActorDeferred<ARuleProjectile>(ARuleProjectile::StaticClass(), X, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				T->TestTrue(TEXT("Real shared P01 initialized"), Shot->InitializeProjectile(FVector::ForwardVector, nullptr, nullptr)); Shot->FinishSpawning(X);
			}
			return false;
		}
		FAutomationTestBase* T; ECase Case; int32 Index = 0;
		TUniquePtr<FMothCombatTestWorld> F;
		AEnemyBase* E = nullptr; ADeviceBase* D = nullptr; AMothPlayerAutomationCharacter* P = nullptr;
	};
}

#define MOTH_DROP_FLIGHT(Class, Name, Case) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(Class, "MothEffect.EnemyDrops." Name, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool Class::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothDropTest::FDropFlight(this, MothDropTest::ECase::Case)); return true; }
MOTH_DROP_FLIGHT(FMothDropRifle, "NormalW01KillsAllVariants", Rifle)
MOTH_DROP_FLIGHT(FMothDropProjectile, "RealP01KillsAllVariants", Projectile)
MOTH_DROP_FLIGHT(FMothDropTerrain, "RealPhysicsAndPickupOnFloorWallSlopeAir", Terrain)
#undef MOTH_DROP_FLIGHT

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDropSaved, "MothEffect.EnemyDrops.SavedVariantsAppearanceAndRange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDropSaved::RunTest(const FString&)
{
	FMothCombatTestWorld F; MothDropTest::Floor(F);
	TSet<UStaticMesh*> Shapes; TSet<UMaterialInterface*> Materials;
	const FLinearColor Colors[] { FLinearColor(0, .8f, 1), FLinearColor(1, .22f, .01f), FLinearColor(.5f, .03f, 1) };
	for (int32 I = 0; I < 3; ++I)
	{
		auto* E = MothDropTest::Enemy(F, I, FVector(I*500, 0, 90));
		if (!TestNotNull(TEXT("Reload/spawn saved enemy"), E)) return false;
		UClass* DropClass = MothDropTest::DeviceClass(I);
		if (!TestNotNull(TEXT("Saved fixed device class"), DropClass)) return false;
		const auto* Defaults = DropClass->GetDefaultObject<ADeviceBase>();
		TestTrue(TEXT("All variants inherit the same saved shooter AI"), E->GetClass()->IsChildOf(LoadClass<AEnemyBase>(nullptr, TEXT("/Game/MothEffect/Enemies/BP_EnemyBase.BP_EnemyBase_C"))));
		TestEqual(TEXT("No per-variant combat health drift"), E->GetHealthComponent()->GetMaxHealth(), 60.0f);
		TestTrue(TEXT("Saved fixed kind"), E->GetDropKind() == static_cast<EDeviceKind>(I) && Defaults->GetDeviceKind() == E->GetDropKind());
		auto* Bag = FindObject<UStaticMeshComponent>(E, TEXT("BackpackMesh"));
		auto* Hint = FindObject<UTextRenderComponent>(E, TEXT("DropHintText"));
		if (!TestNotNull(TEXT("Non-colliding backpack"), Bag) || !TestNotNull(TEXT("Type text independent of color"), Hint)) return false;
		const auto Appearance=Defaults->GetVisualAppearance();
		UStaticMesh* VisualMesh=Appearance.Mesh.LoadSynchronous();
		if (!TestNotNull(TEXT("Resolved saved drop appearance"),VisualMesh)) return false;
		TestEqual(TEXT("Backpack predicts actual drop mesh"), Bag->GetStaticMesh().Get(), VisualMesh);
		const FVector VisualExtent = VisualMesh->GetBounds().BoxExtent * Appearance.RelativeTransform.GetScale3D();
		TestTrue(TEXT("Saved geometry remains pickup sized after OBJ unit conversion"), VisualExtent.GetMin() > 1 && VisualExtent.GetMax() <= 30);
		const int32 MetalSlot=Appearance.bNamedMaterialSlots ? Bag->GetMaterialIndex(TEXT("Metal")) : 0;
		TestEqual(TEXT("Backpack and drop share saved material"), Bag->GetMaterial(MetalSlot), Appearance.Materials[0].LoadSynchronous());
		const int32 ColorSlot=Appearance.bNamedMaterialSlots ? Bag->GetMaterialIndex(TEXT("Glow")) : 0;
		FLinearColor Tint;
		TestTrue(TEXT("Saved material reloads the actual cyan/orange/purple parameter"), Bag->GetMaterial(ColorSlot)
			&& Bag->GetMaterial(ColorSlot)->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Tint")), Tint) && Tint.Equals(Colors[I], .001f));
		TestTrue(TEXT("Hint is visible and contains stable D01/D02/D03 code"), Hint->IsVisible() && Hint->Text.ToString().Contains(FString::Printf(TEXT("D0%d"), I+1)));
		TestTrue(TEXT("Appearance cannot block shots or navigation"), Bag->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Bag->CanEverAffectNavigation());
		Shapes.Add(Bag->GetStaticMesh()); Materials.Add(Bag->GetMaterial(ColorSlot));
	}
	TestEqual(TEXT("Three distinct saved geometric shapes"), Shapes.Num(), 3);
	TestEqual(TEXT("Three distinct color materials"), Materials.Num(), 3);
	TestNotNull(TEXT("Saved T15 range reloads"), LoadObject<UWorld>(nullptr, TEXT("/Game/MothEffect/Tests/L_T15Drops.L_T15Drops")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDropOnce, "MothEffect.EnemyDrops.IdempotentDeathAndFixedClass", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDropOnce::RunTest(const FString&)
{
	FMothCombatTestWorld F; MothDropTest::Floor(F);
	for (int32 I = 0; I < 3; ++I)
	{
		auto* E = MothDropTest::Enemy(F, I, FVector(I*500, 0, 90)); if (!TestNotNull(TEXT("Saved variant"), E)) return false;
		int32 Deaths = 0;
		E->OnEnemyDiedNative.AddLambda([&](AEnemyBase* Dead, AActor*) { ++Deaths; TestEqual(TEXT("Drop guard committed before observers"), Dead->GetDropAttempts(), 1); TestFalse(TEXT("Reentrant lethal damage is rejected"), IBallisticReactive::Execute_ReceiveBallisticHit(Dead, MothDropTest::Hit())); });
		TestTrue(TEXT("First lethal damage succeeds"), IBallisticReactive::Execute_ReceiveBallisticHit(E, MothDropTest::Hit()));
		TestFalse(TEXT("Repeated lethal damage does not retry"), IBallisticReactive::Execute_ReceiveBallisticHit(E, MothDropTest::Hit()));
		TestEqual(TEXT("One death event"), Deaths, 1); E->OnEnemyDiedNative.Clear();
		if (!TestNotNull(TEXT("One actual drop"), E->GetDroppedDevice())) return false;
		TestEqual(TEXT("Fixed drop class, no random table"), E->GetDroppedDevice()->GetClass(), MothDropTest::DeviceClass(I));
		TestEqual(TEXT("Cumulative count has exactly one per death"), MothDropTest::Manager(F)->GetDeviceCount(), I+1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDropBomb, "MothEffect.EnemyDrops.RealD02KillsAllVariants", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDropBomb::RunTest(const FString&)
{
	for (int32 I = 0; I < 3; ++I)
	{
		FMothCombatTestWorld F; MothDropTest::Floor(F);
		auto* E = MothDropTest::Enemy(F, I); if (!TestNotNull(TEXT("Variant for actual bomb"), E)) return false;
		auto* Bomb = MothDropTest::Manager(F)->SpawnDevice(MothDropTest::DeviceClass(1), FTransform(FVector(150, 0, 25)));
		if (!TestNotNull(TEXT("Real configured D02"), Bomb)) return false;
		TestTrue(TEXT("Ordinary D02 activated"), Bomb->TryActivate(MothDropTest::Hit(0)));
		TestEqual(TEXT("D02 actually kills full 60HP enemy"), E->GetHealthComponent()->GetCurrentHealth(), 0.0f);
		if (!TestNotNull(TEXT("Bomb kill drops a normal Dormant"), E->GetDroppedDevice())) return false;
		TestTrue(TEXT("New drop is outside the bomb's pre-damage target snapshot"), E->GetDroppedDevice()->GetDeviceState() == EDeviceState::Dormant);
		TestEqual(TEXT("Consumed bomb slot is released, one drop counted"), MothDropTest::Manager(F)->GetDeviceCount(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDropCapacity, "MothEffect.EnemyDrops.RecyclesOldestDormant", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDropCapacity::RunTest(const FString&)
{
	FMothCombatTestWorld F; MothDropTest::Floor(F); ADeviceBase* Oldest = nullptr;
	for (int32 I = 0; I < 24; ++I) { auto* D = F.Spawn<ADeviceBase>(FVector(2000+I*100, 0, 25)); if (!I) Oldest = D; }
	auto* E = MothDropTest::Enemy(F, 0); if (!TestNotNull(TEXT("Configured enemy"), E)) return false;
	IBallisticReactive::Execute_ReceiveBallisticHit(E, MothDropTest::Hit());
	TestNotNull(TEXT("Actual enemy drop admitted at full capacity"), E->GetDroppedDevice());
	TestTrue(TEXT("Oldest Dormant recycled"), Oldest->GetDeviceState() == EDeviceState::Destroyed);
	TestEqual(TEXT("No total overshoot"), MothDropTest::Manager(F)->GetDeviceCount(), 24);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDropHeldFull, "MothEffect.EnemyDrops.FullHeldCapacityRejectsWithoutRetry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDropHeldFull::RunTest(const FString&)
{
	FMothCombatTestWorld F; MothDropTest::Floor(F); TArray<ADeviceBase*> Held;
	for (int32 I = 0; I < 24; ++I)
	{
		const FVector X(2000+I*500, 0, 100); auto* P = MothDropTest::Player(F, X);
		auto* D = F.Spawn<ADeviceBase>(X+FVector(120, 0, 0)); TestTrue(TEXT("Capacity protected by actual pickup"), P->TryPickupDevice(D)); Held.Add(D);
	}
	auto* E = MothDropTest::Enemy(F, 1); if (!TestNotNull(TEXT("Configured enemy"), E)) return false;
	IBallisticReactive::Execute_ReceiveBallisticHit(E, MothDropTest::Hit());
	TestNull(TEXT("No drop replaces protected Held objects"), E->GetDroppedDevice());
	TestTrue(TEXT("Attempt is rejected once"), E->GetDropResult() == EEnemyDropResult::Rejected && E->GetDropAttempts() == 1);
	for (auto* D : Held) TestTrue(TEXT("Held state and holder remain intact"), D->GetDeviceState() == EDeviceState::Held && D->GetHolder());
	Held[0]->DestroyDevice(); IBallisticReactive::Execute_ReceiveBallisticHit(E, MothDropTest::Hit());
	TestNull(TEXT("A later free slot does not backfill a rejected death"), E->GetDroppedDevice());
	TestEqual(TEXT("No deferred count after freeing a slot"), MothDropTest::Manager(F)->GetDeviceCount(), 23);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDropReset, "MothEffect.EnemyDrops.DeathObserversResetOrDestroyCancel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDropReset::RunTest(const FString&)
{
	for (int32 Case = 0; Case < 3; ++Case)
	{
		FMothCombatTestWorld F; MothDropTest::Floor(F); auto* M = MothDropTest::Manager(F);
		auto* E = MothDropTest::Enemy(F, 2); if (!TestNotNull(TEXT("Configured enemy"), E)) return false;
		if (Case == 0) E->OnEnemyDiedNative.AddLambda([M](AEnemyBase*, AActor*) { M->ResetManagedObjects(); });
		if (Case == 1) E->OnEnemyDiedNative.AddLambda([](AEnemyBase* Dead, AActor*) { Dead->Destroy(); });
		if (Case == 2)
		{
			E->SetCombatTarget(MothDropTest::Player(F, FVector(1000, 0, 90))); F.World->Tick(LEVELTICK_All, .01f);
			E->OnTelegraphChangedNative.AddLambda([M](AEnemyBase*, bool bVisible) { if (!bVisible) M->ResetManagedObjects(); });
		}
		IBallisticReactive::Execute_ReceiveBallisticHit(E, MothDropTest::Hit());
		TestNull(TEXT("No old death drop after reset/destruction observer"), E->GetDroppedDevice());
		TestTrue(TEXT("Lifecycle cancellation recorded"), E->GetDropResult() == EEnemyDropResult::Cancelled);
		TestEqual(TEXT("No count leak after callback"), M->GetDeviceCount(), 0);
		E->OnEnemyDiedNative.Clear(); E->OnTelegraphChangedNative.Clear();
	}
	FMothCombatTestWorld F; auto* E = MothDropTest::Enemy(F, 0); if (!E) return false;
	E->Destroy(); TestEqual(TEXT("Ordinary deletion/EndPlay does not drop"), MothDropTest::Manager(F)->GetDeviceCount(), 0);
	// Even a validator that resets and returns true cannot resurrect its old generation.
	auto* Manager = MothDropTest::Manager(F);
	for (int32 ResetOnCall : {1, 2})
	{
		int32 Calls = 0;
		auto* Drop = Manager->SpawnDeviceForGeneration(MothDropTest::DeviceClass(0), FTransform(FVector(0, 0, 25)),
			Manager->GetLifecycleGeneration(), [Manager, &Calls, ResetOnCall] { if (++Calls == ResetOnCall) Manager->ResetManagedObjects(); return true; });
		TestNull(TEXT("Reset inside either birthplace validator cancels old generation"), Drop);
		TestEqual(TEXT("Validator reset leaves no pending admission"), Manager->GetDeviceCount(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDropBirthCallback, "MothEffect.EnemyDrops.CapacityCallbacksRevalidateBirthplace", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDropBirthCallback::RunTest(const FString&)
{
	FMothCombatTestWorld F; MothDropTest::Floor(F); ADeviceBase* Oldest = nullptr;
	for (int32 I = 0; I < 24; ++I) { auto* D = F.Spawn<ADeviceBase>(FVector(2000+I*100, 0, 25)); if (!I) Oldest = D; }
	Oldest->OnDeviceStateChangedNative.AddLambda([&F](ADeviceBase*, EDeviceState, EDeviceState New) { if (New == EDeviceState::Destroyed) MothDropTest::Box(F, FVector(0, 0, 25), FVector(45, 45, 45)); });
	auto* E = MothDropTest::Enemy(F, 2); if (!TestNotNull(TEXT("Configured enemy"), E)) return false;
	IBallisticReactive::Execute_ReceiveBallisticHit(E, MothDropTest::Hit());
	TestNull(TEXT("Callback-created blocker prevents an embedded drop"), E->GetDroppedDevice());
	TestEqual(TEXT("Rejected birth leaves no pending slot"), MothDropTest::Manager(F)->GetDeviceCount(), 23);
	Oldest->OnDeviceStateChangedNative.Clear();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDropInvalid, "MothEffect.EnemyDrops.InvalidClassAndBlockedOriginDoNotEvict", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDropInvalid::RunTest(const FString&)
{
	for (int32 Case = 0; Case < 2; ++Case)
	{
		FMothCombatTestWorld F; MothDropTest::Floor(F);
		for (int32 I = 0; I < 24; ++I) F.Spawn<ADeviceBase>(FVector(2000+I*100, 0, 25));
		auto* E = MothDropTest::Enemy(F, 0); if (!TestNotNull(TEXT("Configured enemy"), E)) return false;
		if (Case == 0) { auto* P = FindFProperty<FEnumProperty>(AEnemyBase::StaticClass(), TEXT("DropKind")); P->GetUnderlyingProperty()->SetIntPropertyValue(P->ContainerPtrToValuePtr<void>(E), static_cast<int64>(EDeviceKind::Bomb)); }
		else MothDropTest::Box(F, FVector(0, 0, 90), FVector(200, 200, 200));
		IBallisticReactive::Execute_ReceiveBallisticHit(E, MothDropTest::Hit());
		TestNull(TEXT("Invalid type/blocked original path does not spawn"), E->GetDroppedDevice());
		TestTrue(TEXT("Explicit failure result"), E->GetDropResult() == (Case == 0 ? EEnemyDropResult::InvalidClass : EEnemyDropResult::NoSafeLocation));
		TestEqual(TEXT("No capacity eviction before valid birthplace"), MothDropTest::Manager(F)->GetDeviceCount(), 24);
	}
	return true;
}
#endif
