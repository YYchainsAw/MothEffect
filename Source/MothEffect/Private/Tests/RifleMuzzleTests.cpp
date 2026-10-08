#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/Rifle.h"
#include "Components/SceneComponent.h"
#include "CombatTestWorld.h"
#include "Misc/AutomationTest.h"

namespace
{
	USceneComponent* CreateMuzzleSource(FMothCombatTestWorld& Fixture, const FVector& Location)
	{
		AActor* SourceActor = Fixture.Spawn<AActor>();
		if (!SourceActor) return nullptr;
		USceneComponent* Source = NewObject<USceneComponent>(SourceActor);
		SourceActor->AddInstanceComponent(Source);
		SourceActor->SetRootComponent(Source);
		Source->RegisterComponent();
		Source->SetWorldLocation(Location);
		return Source;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothRifleExternalMuzzleTest,
	"MothEffect.Weapons.Muzzle.ExternalSourceFollowsMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothRifleExternalMuzzleTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	ARifle* Rifle = Fixture.Spawn<ARifle>(FVector(100.0f, 0.0f, 200.0f));
	if (!TestNotNull(TEXT("Create rifle gameplay actor"), Rifle)) return false;
	FVector OriginalLocation;
	TestTrue(TEXT("Separate rifle has its own muzzle"), Rifle->GetMuzzleLocation(OriginalLocation));

	const FVector FirstLocation(20.0f, 80.0f, 260.0f);
	USceneComponent* Source = CreateMuzzleSource(Fixture, FirstLocation);
	if (!TestNotNull(TEXT("Create independently moving muzzle source"), Source)) return false;
	Rifle->SetMuzzleSource(Source, NAME_None);
	Rifle->SetActorHiddenInGame(true);
	FVector Location;
	TestTrue(TEXT("Hidden gameplay rifle still reads external muzzle"), Rifle->GetMuzzleLocation(Location));
	TestTrue(TEXT("Use external origin rather than legacy weapon offset"), Location.Equals(FirstLocation));

	const FVector NextLocation(80.0f, -40.0f, 330.0f);
	Source->SetWorldLocation(NextLocation);
	TestTrue(TEXT("External source remains valid after movement"), Rifle->GetMuzzleLocation(Location));
	TestTrue(TEXT("Each shot origin follows source movement"), Location.Equals(NextLocation));
	Rifle->ClearMuzzleSource();
	TestTrue(TEXT("Separate weapon mode restores its muzzle"), Rifle->GetMuzzleLocation(Location));
	TestTrue(TEXT("Separate weapon offset is preserved"), Location.Equals(OriginalLocation));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothRifleInvalidMuzzleTest,
	"MothEffect.Weapons.Muzzle.InvalidSourceFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothRifleInvalidMuzzleTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	ARifle* Rifle = Fixture.Spawn<ARifle>();
	if (!TestNotNull(TEXT("Create rifle gameplay actor"), Rifle)) return false;
	FVector Location;
	Rifle->SetMuzzleSource(nullptr, TEXT("Muzzle"));
	TestFalse(TEXT("Missing integrated source cannot use separate weapon origin"), Rifle->GetMuzzleLocation(Location));
	TestTrue(TEXT("Failure clears output location"), Location.IsNearlyZero());

	USceneComponent* Source = CreateMuzzleSource(Fixture, FVector(500.0f, 0.0f, 200.0f));
	if (!TestNotNull(TEXT("Create source without the requested socket"), Source)) return false;
	Rifle->SetMuzzleSource(Source, TEXT("MissingMuzzleSocket"));
	TestFalse(TEXT("Missing socket cannot silently become the component origin"), Rifle->GetMuzzleLocation(Location));
	Rifle->SetMuzzleSource(Source, NAME_None);
	TestTrue(TEXT("Explicit component origin can be selected"), Rifle->GetMuzzleLocation(Location));
	Source->DestroyComponent();
	TestFalse(TEXT("Destroyed source cannot fall back to the separate weapon"), Rifle->GetMuzzleLocation(Location));
	USceneComponent* ActorSource = CreateMuzzleSource(Fixture, FVector(300.0f, 0.0f, 200.0f));
	if (!TestNotNull(TEXT("Create a source whose owning actor can be destroyed"), ActorSource)) return false;
	Rifle->SetMuzzleSource(ActorSource, NAME_None);
	TestTrue(TEXT("Live source actor provides a muzzle"), Rifle->GetMuzzleLocation(Location));
	ActorSource->GetOwner()->Destroy();
	TestFalse(TEXT("Destroyed source actor cannot leave a stale firing origin"), Rifle->GetMuzzleLocation(Location));
	Rifle->ClearMuzzleSource();
	TestTrue(TEXT("Explicitly returning to separate mode restores firing origin"), Rifle->GetMuzzleLocation(Location));
	return true;
}

#endif
