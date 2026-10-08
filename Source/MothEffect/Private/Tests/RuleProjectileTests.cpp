#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/CombatTestWorld.h"
#include "Types/MothCollisionChannels.h"

namespace
{
	ARuleProjectile* SpawnProjectile(FMothCombatTestWorld& Fixture, AActor* Source,
		APawn* Instigator = nullptr, const FVector& Direction = FVector::ForwardVector)
	{
		if (!Fixture.World)
		{
			return nullptr;
		}
		const FTransform Transform(Direction.Rotation(), FVector::ZeroVector);
		ARuleProjectile* Projectile = Fixture.World->SpawnActorDeferred<ARuleProjectile>(
			ARuleProjectile::StaticClass(), Transform, Source, Instigator,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Projectile)
		{
			Projectile->InitializeProjectile(Direction, Source, Instigator);
			Projectile->FinishSpawning(Transform);
			Projectile->DispatchBeginPlay();
		}
		return Projectile;
	}

	FHitResult BlockingHit(AActor* Actor, UPrimitiveComponent* Component)
	{
		FHitResult Hit(Actor, Component, Actor->GetActorLocation(), -FVector::ForwardVector);
		Hit.bBlockingHit = true;
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothProjectileSingleImpactTest,
	"MothEffect.Projectiles.FirstImpactIsSingleUse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothProjectileSingleImpactTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	AActor* Source = Fixture.Spawn<AActor>();
	ADeviceBase* Target = Fixture.Spawn<ADeviceBase>(FVector(200.0, 0.0, 0.0));
	ARuleProjectile* Projectile = SpawnProjectile(Fixture, Source, nullptr, FVector(2.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Spawn P01"), Projectile) || !TestNotNull(TEXT("Spawn target"), Target))
	{
		return false;
	}
	const FHitResult Hit = BlockingHit(Target, Target->GetPhysicsBody());
	const FGuid HitId = Projectile->GetHitId();
	int32 ImpactCount = 0;
	Projectile->OnImpactNative.AddLambda([this, &Hit, &ImpactCount, Source, HitId]
		(ARuleProjectile* ChangedProjectile, AActor* HitActor, const FHitContext& Context)
		{
			++ImpactCount;
			TestTrue(TEXT("Birth ID is passed to the ballistic target"), Context.HitId == HitId);
			TestTrue(TEXT("Source is the firing actor"), Context.SourceActor.Get() == Source);
			TestEqual(TEXT("P01 damage is passed through the common context"), Context.Damage, 10.0f);
			TestTrue(TEXT("World trajectory, not the hit normal, is passed"),
				Context.ShotDirection.Equals(FVector::ForwardVector));
			TestFalse(TEXT("Re-entry cannot deliver a second impact"), ChangedProjectile->ProcessBlockingHit(Hit));
		});
	TestTrue(TEXT("First blocking hit consumes P01"), Projectile->ProcessBlockingHit(Hit));
	TestTrue(TEXT("Dormant receives the ballistic activation"), Target->GetDeviceState() == EDeviceState::Active);
	TestTrue(TEXT("Device retains the projectile's unique ID"), Target->GetActivationHitId() == HitId);
	TestEqual(TEXT("One impact notification"), ImpactCount, 1);
	TestFalse(TEXT("Repeated callback stays consumed"), Projectile->ProcessBlockingHit(Hit));
	TestTrue(TEXT("Collision is disabled before disposal"),
		Projectile->FindComponentByClass<USphereComponent>()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothProjectileRejectionAndSourceTest,
	"MothEffect.Projectiles.RejectionAndSourceExclusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothProjectileRejectionAndSourceTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	ADeviceBase* Source = Fixture.Spawn<ADeviceBase>();
	APawn* Activator = Fixture.Spawn<APawn>(FVector(300.0, 0.0, 0.0));
	ADeviceBase* Target = Fixture.Spawn<ADeviceBase>(FVector(200.0, 0.0, 0.0));
	ARuleProjectile* Projectile = SpawnProjectile(Fixture, Source, Activator);
	if (!TestNotNull(TEXT("Spawn projectile"), Projectile) || !TestNotNull(TEXT("Spawn source"), Source)
		|| !TestNotNull(TEXT("Spawn activator"), Activator) || !TestNotNull(TEXT("Spawn target"), Target))
	{
		return false;
	}
	USphereComponent* Sphere = Projectile->FindComponentByClass<USphereComponent>();
	TestTrue(TEXT("Movement ignores the firing device"), Sphere->GetMoveIgnoreActors().Contains(Source));
	TestFalse(TEXT("Activation attribution does not grant pawn immunity"),
		Sphere->GetMoveIgnoreActors().Contains(Activator));
	TestFalse(TEXT("A callback on the source is also rejected"),
		Projectile->ProcessBlockingHit(BlockingHit(Source, Source->GetPhysicsBody())));
	TestFalse(TEXT("Self hit does not consume"), Projectile->HasProcessedImpact());
	FHitContext Activation;
	Activation.HitId = FGuid::NewGuid();
	Activation.ShotDirection = FVector::RightVector;
	TestTrue(TEXT("Prepare an already active target"), Target->TryActivate(Activation));
	TestTrue(TEXT("An interface rejection still consumes P01"),
		Projectile->ProcessBlockingHit(BlockingHit(Target, Target->GetPhysicsBody())));
	TestTrue(TEXT("Rejected activation does not rotate the target"),
		Target->GetActivationDirection().Equals(FVector::RightVector));
	ARuleProjectile* Survivor = SpawnProjectile(Fixture, Source, Activator);
	if (!TestNotNull(TEXT("Spawn an independently living projectile"), Survivor))
	{
		return false;
	}
	TestTrue(TEXT("P01 has its own lifetime"), FMath::IsNearlyEqual(Survivor->GetLifeSpan(), 3.0f));
	Source->DestroyDevice();
	TestFalse(TEXT("Removing the source does not consume an in-flight projectile"), Survivor->HasProcessedImpact());
	TestTrue(TEXT("A plain actor still blocks after source disposal"),
		Survivor->ProcessBlockingHit(BlockingHit(Activator, nullptr)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothProjectileSweptWallTest,
	"MothEffect.Projectiles.SweptThinWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMothProjectileSweptWallTest::RunTest(const FString& Parameters)
{
	FMothCombatTestWorld Fixture;
	AActor* Wall = Fixture.Spawn<AActor>(FVector(70.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Spawn a thin wall"), Wall))
	{
		return false;
	}
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(1.0, 100.0, 100.0));
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponent();
	Box->SetWorldLocation(FVector(70.0, 0.0, 0.0));
	ARuleProjectile* Projectile = SpawnProjectile(Fixture, nullptr);
	if (!TestNotNull(TEXT("Spawn swept projectile"), Projectile))
	{
		return false;
	}
	USphereComponent* Sphere = Projectile->FindComponentByClass<USphereComponent>();
	UProjectileMovementComponent* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
	TestFalse(TEXT("P01 is not moved by Chaos"), Sphere->IsSimulatingPhysics());
	TestTrue(TEXT("P01 uses its collision object channel"), Sphere->GetCollisionObjectType() == MothCollision::Projectile);
	TestEqual(TEXT("Flight has no gravity"), Movement->ProjectileGravityScale, 0.0f);
	TestTrue(TEXT("Flight speed uses the P01 baseline"), Movement->Velocity.Equals(FVector(1400.0, 0.0, 0.0)));
	Movement->TickComponent(0.1f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Swept movement consumes on the wall before the frame endpoint"), Projectile->HasProcessedImpact());
	TestTrue(TEXT("P01 cannot travel through the thin wall"), Projectile->GetActorLocation().X < 70.0);
	return true;
}

#endif
