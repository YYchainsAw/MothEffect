#if WITH_DEV_AUTOMATION_TESTS

#include "Devices/DeviceBase.h"
#include "Devices/DeviceReleaseSafety.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"

namespace
{
	struct FReleaseTestWorld
	{
		UWorld* World = nullptr;
		FReleaseTestWorld()
		{
			if (GEngine)
			{
				World = UWorld::CreateWorld(EWorldType::Game, false);
				if (World)
				{
					GEngine->CreateNewWorldContext(World->WorldType).SetCurrentWorld(World);
					World->SetShouldTick(false);
					World->InitializeActorsForPlay(FURL());
				}
			}
		}
		~FReleaseTestWorld()
		{
			if (World)
			{
				World->BeginTearingDown();
				for (FActorIterator It(World); It; ++It) It->RouteEndPlay(EEndPlayReason::Quit);
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}

		UBoxComponent* AddBlocker(const FVector& Location, const FVector& Extent, ECollisionChannel ObjectType)
		{
			AActor* Actor = World ? World->SpawnActor<AActor>() : nullptr;
			if (!Actor) return nullptr;
			UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
			Actor->AddInstanceComponent(Box);
			Actor->SetRootComponent(Box);
			Box->InitBoxExtent(Extent);
			Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Box->SetCollisionObjectType(ObjectType);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->SetGenerateOverlapEvents(false);
			Box->RegisterComponent();
			Box->SetWorldLocation(Location);
			return Box;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothReleaseThinWallTest,
	"MothEffect.Devices.ReleaseSafety.ThinWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothReleaseThinWallTest::RunTest(const FString& Parameters)
{
	FReleaseTestWorld Fixture;
	ADeviceBase* Device = Fixture.World ? Fixture.World->SpawnActor<ADeviceBase>() : nullptr;
	if (!TestNotNull(TEXT("Create physical device"), Device)) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothReleaseTest), false, Device);
	const FVector Start(0.0f, 0.0f, 200.0f), End(120.0f, 0.0f, 200.0f);
	TestTrue(TEXT("Unoccupied physical release path succeeds"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), Start, End, Params));
	UBoxComponent* Wall = Fixture.AddBlocker(FVector(60.0f, 0.0f, 200.0f), FVector(1.0f, 100.0f, 100.0f), ECC_WorldStatic);
	if (!TestNotNull(TEXT("Create a two-centimetre wall between clear endpoints"), Wall)) return false;
	TestFalse(TEXT("Swept physical sphere cannot cross a thin wall"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), Start, End, Params));
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestTrue(TEXT("Removing blocker permits the same path"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), Start, End, Params));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothReleaseEndpointTest,
	"MothEffect.Devices.ReleaseSafety.EndpointOccupancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothReleaseEndpointTest::RunTest(const FString& Parameters)
{
	FReleaseTestWorld Fixture;
	ADeviceBase* Device = Fixture.World ? Fixture.World->SpawnActor<ADeviceBase>() : nullptr;
	if (!TestNotNull(TEXT("Create physical device"), Device)) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothReleaseTest), false, Device);
	const FVector End(90.0f, 0.0f, 200.0f);
	UBoxComponent* Prop = Fixture.AddBlocker(End + FVector(0.0f, 30.0f, 0.0f), FVector(5.0f), ECC_PhysicsBody);
	if (!TestNotNull(TEXT("Create a nearby physics prop"), Prop)) return false;
	TestTrue(TEXT("Default twenty-centimetre sphere fits beside the prop"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), End, End, Params));
	Device->SetActorScale3D(FVector(2.0f));
	TestFalse(TEXT("Query uses actual scaled body rather than a fixed small point test"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), End, End, Params));
	Prop->SetWorldLocation(End);
	TestFalse(TEXT("Occupied endpoint is rejected even for a zero-length sweep"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), End, End, Params));
	TestFalse(TEXT("Occupied starting volume is rejected"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), End, End + FVector(120.0f, 0.0f, 0.0f), Params));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothReleasePawnTest,
	"MothEffect.Devices.ReleaseSafety.PawnAndIgnoredActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothReleasePawnTest::RunTest(const FString& Parameters)
{
	FReleaseTestWorld Fixture;
	ADeviceBase* Device = Fixture.World ? Fixture.World->SpawnActor<ADeviceBase>() : nullptr;
	if (!TestNotNull(TEXT("Create physical device"), Device)) return false;
	const FVector Start(0.0f, 0.0f, 200.0f), End(90.0f, 0.0f, 200.0f);
	UBoxComponent* Pawn = Fixture.AddBlocker(End, FVector(15.0f), ECC_Pawn);
	if (!TestNotNull(TEXT("Create pawn collision volume"), Pawn)) return false;
	Pawn->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothReleaseTest), false, Device);
	TestFalse(TEXT("A Pawn still blocks physical release when weapon Visibility ignores it"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), Start, End, Params));
	Params.AddIgnoredActor(Pawn->GetOwner());
	TestTrue(TEXT("Explicitly ignored owner volume does not block release"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), Start, End, Params));
	Params.ClearIgnoredActors();
	Params.AddIgnoredActor(Device);
	Pawn->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestTrue(TEXT("Non-colliding carried presentation cannot act as an obstruction"),
		MothDeviceRelease::IsPathClear(Fixture.World, Device->GetPhysicsBody(), Start, End, Params));
	return true;
}

#endif
