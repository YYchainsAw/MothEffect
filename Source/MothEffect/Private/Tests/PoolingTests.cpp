#if WITH_DEV_AUTOMATION_TESTS

#include "AI/EnemyBase.h"
#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ExternalImpulseComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "Framework/MothWaveDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Subsystems/EnemyPoolSubsystem.h"
#include "Subsystems/ProjectilePoolSubsystem.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "UObject/UnrealType.h"

namespace MothPoolTest
{
	FHitContext Damage(float Amount)
	{
		FHitContext Hit;
		Hit.HitId = FGuid::NewGuid();
		Hit.Damage = Amount;
		Hit.ShotDirection = FVector::ForwardVector;
		return Hit;
	}

	FHitResult BlockingHit(AEnemyBase* Enemy)
	{
		FHitResult Hit(Enemy, Enemy->GetCapsuleComponent(), Enemy->GetActorLocation(), -FVector::ForwardVector);
		Hit.bBlockingHit = true;
		return Hit;
	}

	void Floor(FMothCombatTestWorld& Fixture)
	{
		auto* Actor = Fixture.Spawn<AActor>();
		auto* Box = NewObject<UBoxComponent>(Actor);
		Actor->AddInstanceComponent(Box);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(10000, 10000, 10));
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->RegisterComponent();
		Box->SetWorldLocation(FVector(0, 0, -10));
	}

	AActor* BlockingBox(FMothCombatTestWorld& Fixture, const FVector& Position)
	{
		auto* Actor = Fixture.Spawn<AActor>();
		auto* Box = NewObject<UBoxComponent>(Actor);
		Actor->AddInstanceComponent(Box);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(100));
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->RegisterComponent();
		Box->SetWorldLocation(Position);
		return Actor;
	}

	AEnemyBase* Enemy(FMothCombatTestWorld& Fixture, const FVector& Position = FVector(0, 0, 92), bool bDrop = false)
	{
		auto* Pool = Fixture.World->GetSubsystem<UEnemyPoolSubsystem>();
		const FTransform Transform(Position);
		auto* Result = Pool->AcquireEnemy(AEnemyBase::StaticClass(), Transform, nullptr);
		if (!Result) return nullptr;
		// A native fixed-drop archetype keeps this test independent of saved game assets.
		if (bDrop)
			FindFProperty<FClassProperty>(AEnemyBase::StaticClass(), TEXT("DropDeviceClass"))
				->SetObjectPropertyValue_InContainer(Result, ADeviceBase::StaticClass());
		Pool->FinishSpawningEnemy(Result, Transform);
		return IsValid(Result) && !Result->IsActorBeingDestroyed() && !Result->IsPoolInactive() ? Result : nullptr;
	}

	ARuleProjectile* Projectile(FMothCombatTestWorld& Fixture, AActor* Source = nullptr,
		APawn* Instigator = nullptr, const FVector& Direction = FVector::ForwardVector,
		const FVector& Position = FVector(0, 0, 1000))
	{
		return Fixture.World->GetSubsystem<UProjectilePoolSubsystem>()->SpawnProjectile(
			ARuleProjectile::StaticClass(), FTransform(Direction.Rotation(), Position), Direction, Source, Instigator);
	}

	class FExpiryAndTimerReuse final : public IAutomationLatentCommand
	{
	public:
		explicit FExpiryAndTimerReuse(FAutomationTestBase* InTest) : Test(InTest) {}
		bool Update() override
		{
			if (!Fixture) return Setup();
			if (Fixture->World->GetTimerManager().HasBeenTickedThisFrame()) return false;
			Fixture->World->Tick(LEVELTICK_All, 0.01f);
			if (Fixture->World->GetTimeSeconds() < 3.1) return false;
			Test->TestTrue(TEXT("Actual lifespan timer returns the projectile without destruction"),
				Shot->IsPoolInactive() && !Shot->IsActorBeingDestroyed());
			Test->TestEqual(TEXT("Expiry releases the live projectile budget"),
				Fixture->World->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(), 0);
			Test->TestTrue(TEXT("Old death feedback timer cannot reclaim a reused enemy"),
				!ReusedEnemy->IsPoolInactive() && ReusedEnemy->GetHealthComponent()->IsAlive());
			auto* ReusedShot = Projectile(*Fixture);
			if (!Test->TestNotNull(TEXT("Expired projectile can be checked out again"), ReusedShot)) return true;
			Test->TestTrue(TEXT("Expiry preserves the same actor for reuse"), ReusedShot == Shot);
			Test->TestTrue(TEXT("Reused projectile receives its entire new lifetime"), ReusedShot->GetLifeSpan() > 2.9f);
			static_cast<AActor*>(ReusedShot)->FellOutOfWorld(*GetDefault<UDamageType>());
			Test->TestTrue(TEXT("KillZ removal returns a pooled projectile"), ReusedShot->IsPoolInactive());
			Test->TestEqual(TEXT("Repeated disposal never duplicates the free entry"),
				Fixture->World->GetSubsystem<UProjectilePoolSubsystem>()->GetInactiveCount(), 1);
			return true;
		}
	private:
		bool Setup()
		{
			Fixture = MakeUnique<FMothCombatTestWorld>();
			if (!Test->TestNotNull(TEXT("Timer test world"), Fixture->World)) return true;
			Floor(*Fixture);
			Shot = Projectile(*Fixture);
			auto* FirstLife = Enemy(*Fixture);
			if (!Test->TestNotNull(TEXT("Timed projectile"), Shot)
				|| !Test->TestNotNull(TEXT("Enemy before timed death"), FirstLife)) return true;
			Test->TestTrue(TEXT("First life commits death and its visual timer"),
				IBallisticReactive::Execute_ReceiveBallisticHit(FirstLife, Damage(60)));
			Test->TestTrue(TEXT("Death retains the configured feedback interval"), FirstLife->GetLifeSpan() > 0);
			FirstLife->ReturnToPool();
			ReusedEnemy = Enemy(*Fixture, FVector(1000, 0, 92));
			if (!Test->TestNotNull(TEXT("Enemy reused before old death timer"), ReusedEnemy)) return true;
			Test->TestTrue(TEXT("Timed death actor is reused"), ReusedEnemy == FirstLife);
			return false;
		}
		FAutomationTestBase* Test;
		TUniquePtr<FMothCombatTestWorld> Fixture;
		ARuleProjectile* Shot = nullptr;
		AEnemyBase* ReusedEnemy = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyPoolLifeReset,
	"MothEffect.Pooling.EnemyHealthHitIdentityDropAndAIReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyPoolLifeReset::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Enemy pool world"), Fixture.World)) return false;
	MothPoolTest::Floor(Fixture);
	auto* Pool = Fixture.World->GetSubsystem<UEnemyPoolSubsystem>();
	auto* Enemy = MothPoolTest::Enemy(Fixture, FVector(0, 0, 92), true);
	auto* Target = Fixture.Spawn<AEnemyBase>(FVector(1000, 0, 92));
	if (!TestNotNull(TEXT("Pool enemy"), Enemy) || !TestNotNull(TEXT("Combat target"), Target)) return false;
	TestTrue(TEXT("Fresh deferred checkout enables its configured actor tick"), Enemy->IsActorTickEnabled());
	TestTrue(TEXT("Fresh deferred checkout enables its configured CharacterMovement tick"), Enemy->GetCharacterMovement()->IsComponentTickEnabled());
	Target->SetGameplayEnabled(false);
	const FHitContext RepeatedHit = MothPoolTest::Damage(10);
	TestTrue(TEXT("A live actor accepts the first hit identity"), IBallisticReactive::Execute_ReceiveBallisticHit(Enemy, RepeatedHit));
	TestFalse(TEXT("A live actor rejects a duplicate hit"), IBallisticReactive::Execute_ReceiveBallisticHit(Enemy, RepeatedHit));
	TestTrue(TEXT("Pooled AI accepts a healthy target"), Enemy->SetCombatTarget(Target));
	Enemy->Tick(0.01f);
	TestTrue(TEXT("The original life enters a real attack warning"), Enemy->GetCombatPhase() == EEnemyCombatPhase::Telegraphing);
	auto* Impulse = Enemy->FindComponentByClass<UExternalImpulseComponent>();
	const FGuid ImpulseId = FGuid::NewGuid();
	TestTrue(TEXT("First life accepts an external impulse identity"), Impulse->QueueImpulse(FVector(200, 0, 100), false, ImpulseId));
	Enemy->GetCharacterMovement()->Velocity = FVector(100, 50, 20);
	Enemy->ReturnToPool();
	TestTrue(TEXT("Returned enemy is hidden, dormant and collision-free"),
		Enemy->IsPoolInactive() && Enemy->IsHidden() && !Enemy->IsActorTickEnabled() && !Enemy->GetActorEnableCollision());
	TestFalse(TEXT("Inactive enemies reject even direct component damage"), Enemy->GetHealthComponent()->ApplyHit(MothPoolTest::Damage(10)));
	TestFalse(TEXT("Inactive enemies reject external impulse"), Impulse->QueueImpulse(FVector(100, 0, 0), false, FGuid::NewGuid()));
	Enemy->ReturnToPool();
	TestEqual(TEXT("Repeated enemy recycle has one inactive entry"), Pool->GetInactiveCount(), 1);
	auto* Reused = MothPoolTest::Enemy(Fixture, FVector(2000, 0, 92));
	if (!TestNotNull(TEXT("Reuse enemy"), Reused)) return false;
	TestTrue(TEXT("Enemy checkout reuses the same actor"), Reused == Enemy);
	TestEqual(TEXT("Only one enemy was allocated"), Pool->GetCreatedCount(), 1);
	TestEqual(TEXT("One checkout reused its allocation"), Pool->GetReuseCount(), 1);
	TestEqual(TEXT("Health is fully restored for the new life"), Reused->GetHealthComponent()->GetCurrentHealth(), 60.0f);
	TestNull(TEXT("Previous target is cleared"), Reused->GetCombatTarget());
	TestTrue(TEXT("Previous attack phase is cleared"), Reused->GetCombatPhase() == EEnemyCombatPhase::Idle);
	TestTrue(TEXT("New enemy is active and visible"), Reused->IsCombatEnabled() && !Reused->IsHidden() && Reused->IsActorTickEnabled());
	TestTrue(TEXT("Reused enemy restores CharacterMovement ticking"), Reused->GetCharacterMovement()->IsComponentTickEnabled());
	TestTrue(TEXT("Previous movement and impulse accumulation do not carry over"), Reused->GetCharacterMovement()->Velocity.IsNearlyZero());
	TestTrue(TEXT("CharacterMovement resumes after the pooled death/disable path"), Reused->GetCharacterMovement()->MovementMode != MOVE_None);
	TestEqual(TEXT("Attack counters start from zero"), Reused->GetShotAttempts(), 0);
	TestEqual(TEXT("Projectile counters start from zero"), Reused->GetProjectilesSpawned(), 0);
	TestEqual(TEXT("Navigation counters start from zero"), Reused->GetNavigationRequests(), 0);
	TestTrue(TEXT("A previous life's hit ID is accepted in the new life"), IBallisticReactive::Execute_ReceiveBallisticHit(Reused, RepeatedHit));
	TestTrue(TEXT("A previous life's impulse ID is accepted in the new life"), Impulse->QueueImpulse(FVector(100, 0, 0), false, ImpulseId));
	int32 Deaths = 0;
	const FDelegateHandle DeathHandle = Reused->OnEnemyDiedNative.AddLambda([&](AEnemyBase*, AActor*) { ++Deaths; });
	const FHitContext Fatal = MothPoolTest::Damage(60);
	TestTrue(TEXT("First pooled death applies"), IBallisticReactive::Execute_ReceiveBallisticHit(Reused, Fatal));
	TestFalse(TEXT("First pooled death is idempotent"), IBallisticReactive::Execute_ReceiveBallisticHit(Reused, Fatal));
	TestEqual(TEXT("One death observer fires"), Deaths, 1);
	TestEqual(TEXT("First death attempts its fixed drop once"), Reused->GetDropAttempts(), 1);
	auto* FirstDrop = Reused->GetDroppedDevice();
	TestNotNull(TEXT("Native archetype produces a real managed drop"), FirstDrop);
	Reused->ReturnToPool();
	Reused = MothPoolTest::Enemy(Fixture, FVector(3000, 0, 92));
	if (!TestNotNull(TEXT("Second life after lethal damage"), Reused)) return false;
	TestTrue(TEXT("Death does not destroy the pooled actor"), Reused == Enemy);
	TestEqual(TEXT("Drop guard is reset before the next death"), Reused->GetDropAttempts(), 0);
	TestNull(TEXT("Previous life no longer owns its drop pointer"), Reused->GetDroppedDevice());
	TestTrue(TEXT("Same lethal identity may kill the new life"), IBallisticReactive::Execute_ReceiveBallisticHit(Reused, Fatal));
	TestEqual(TEXT("Death delegate remains bound exactly once across pooling"), Deaths, 2);
	TestEqual(TEXT("New life can attempt one fresh drop"), Reused->GetDropAttempts(), 1);
	TestTrue(TEXT("Each life owns a separate real drop"), IsValid(FirstDrop) && Reused->GetDroppedDevice() && Reused->GetDroppedDevice() != FirstDrop);
	TestEqual(TEXT("Both successful deaths contribute exactly one device"), Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>()->GetDeviceCount(), 2);
	Reused->OnEnemyDiedNative.Remove(DeathHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyPoolReentrantDeath,
	"MothEffect.Pooling.EnemyDeathObserversCannotReuseTheirDispatchingActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyPoolReentrantDeath::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Reentrant enemy pool world"), Fixture.World)) return false;
	MothPoolTest::Floor(Fixture);
	auto* Original = MothPoolTest::Enemy(Fixture, FVector(0, 0, 92), true);
	if (!TestNotNull(TEXT("Original enemy life"), Original)) return false;
	AEnemyBase* NestedCheckout = nullptr;
	int32 ObserverCalls = 0;
	const FDelegateHandle ReturnHandle = Original->OnEnemyDiedNative.AddLambda([&](AEnemyBase* Dead, AActor*)
	{
		++ObserverCalls;
		Dead->ReturnToPool();
		NestedCheckout = MothPoolTest::Enemy(Fixture, FVector(1000, 0, 92));
		TestTrue(TEXT("Nested death checkout cannot resurrect the actor still dispatching observers"), NestedCheckout && NestedCheckout != Dead);
	});
	const FDelegateHandle LaterHandle = Original->OnEnemyDiedNative.AddLambda([&](AEnemyBase* Dead, AActor*)
	{
		++ObserverCalls;
		// Delegate order is unspecified; this observer may run before or after the return.
		TestFalse(TEXT("All old death observers see the old life rather than a revived actor"), Dead->GetHealthComponent()->IsAlive());
	});
	TestTrue(TEXT("Death commits while its observer returns the enemy"),
		IBallisticReactive::Execute_ReceiveBallisticHit(Original, MothPoolTest::Damage(60)));
	TestEqual(TEXT("Both old death observers run once"), ObserverCalls, 2);
	TestTrue(TEXT("Original is available only after the old death stack unwinds"), Original->IsPoolInactive());
	TestNull(TEXT("Recycled death cannot resume its old drop operation"), Original->GetDroppedDevice());
	TestEqual(TEXT("Reentrant cancellation leaves no old-life drop"), Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>()->GetDeviceCount(), 0);
	if (TestNotNull(TEXT("Nested checkout survived old callbacks"), NestedCheckout))
	{
		TestTrue(TEXT("Old death cannot poison another checkout"), NestedCheckout->IsCombatEnabled() && NestedCheckout->GetHealthComponent()->IsAlive());
		TestEqual(TEXT("Nested enemy has full health"), NestedCheckout->GetHealthComponent()->GetCurrentHealth(), 60.0f);
	}
	Original->OnEnemyDiedNative.Remove(ReturnHandle);
	Original->OnEnemyDiedNative.Remove(LaterHandle);
	auto* AfterDispatch = MothPoolTest::Enemy(Fixture, FVector(2000, 0, 92));
	TestTrue(TEXT("Original actor becomes reusable after all observers finish"), AfterDispatch == Original);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothProjectilePoolLifeReset,
	"MothEffect.Pooling.ProjectileImpactReuseHitIdentitySourceAndMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothProjectilePoolLifeReset::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Projectile pool world"), Fixture.World)) return false;
	auto* Pool = Fixture.World->GetSubsystem<UProjectilePoolSubsystem>();
	auto* Lifecycle = Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>();
	auto* SourceA = Fixture.Spawn<AActor>();
	auto* SourceB = Fixture.Spawn<AActor>();
	auto* Instigator = Fixture.Spawn<APawn>();
	auto* Target = Fixture.Spawn<AEnemyBase>(FVector(500, 0, 92));
	Target->SetActorTickEnabled(false);
	Target->GetCharacterMovement()->DisableMovement();
	auto* Shot = MothPoolTest::Projectile(Fixture, SourceA, Instigator);
	if (!TestNotNull(TEXT("First pooled shot"), Shot)) return false;
	auto* Sphere = Shot->FindComponentByClass<USphereComponent>();
	auto* Movement = Shot->FindComponentByClass<UProjectileMovementComponent>();
	const FGuid FirstId = Shot->GetHitId();
	int32 OriginalObserverCalls = 0;
	Shot->OnImpactNative.AddLambda([&](ARuleProjectile*, AActor*, const FHitContext&) { ++OriginalObserverCalls; });
	TestTrue(TEXT("Original shot ignores its source"), Sphere->GetMoveIgnoreActors().Contains(SourceA));
	TestFalse(TEXT("Instigator attribution does not grant immunity"), Sphere->GetMoveIgnoreActors().Contains(Instigator));
	TestTrue(TEXT("First shot actually hits shared enemy health"), Shot->ProcessBlockingHit(MothPoolTest::BlockingHit(Target)));
	TestEqual(TEXT("First hit deals projectile damage"), Target->GetHealthComponent()->GetCurrentHealth(), 50.0f);
	TestTrue(TEXT("Impact recycles the shot synchronously after callbacks"), Shot->IsPoolInactive() && !Shot->IsActorBeingDestroyed());
	TestTrue(TEXT("Inactive projectile has no collision or movement"), Sphere->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Movement->IsActive() && Movement->Velocity.IsNearlyZero());
	TestEqual(TEXT("Impact frees the shared live budget"), Lifecycle->GetProjectileCount(), 0);
	auto* Reused = MothPoolTest::Projectile(Fixture, SourceB, Instigator, FVector::RightVector, FVector(0, 500, 1000));
	if (!TestNotNull(TEXT("Reused shot"), Reused)) return false;
	TestTrue(TEXT("Second shot checks out the identical actor"), Reused == Shot);
	TestTrue(TEXT("Each flight receives a fresh hit GUID"), Reused->GetHitId().IsValid() && Reused->GetHitId() != FirstId);
	TestFalse(TEXT("New life clears the consumed-impact guard"), Reused->HasProcessedImpact());
	TestFalse(TEXT("Previous source is removed from movement ignores"), Sphere->GetMoveIgnoreActors().Contains(SourceA));
	TestTrue(TEXT("New source is added to movement ignores"), Sphere->GetMoveIgnoreActors().Contains(SourceB));
	TestTrue(TEXT("Owner and instigator follow the new shot"), Reused->GetOwner() == SourceB && Reused->GetInstigator() == Instigator);
	TestTrue(TEXT("Stopped projectile movement is reattached to its sphere"), Movement->UpdatedComponent == Sphere);
	TestTrue(TEXT("Flight resumes with the new world direction and speed"), Movement->IsActive() && Movement->Velocity.Equals(FVector(0, 1400, 0), 0.01));
	const FGuid SecondId = Reused->GetHitId();
	Reused->OnImpactNative.AddLambda([this, SourceB, SecondId](ARuleProjectile*, AActor*, const FHitContext& Context)
	{
		TestTrue(TEXT("Reused impact reports the new source and hit identity"), Context.SourceActor.Get() == SourceB && Context.HitId == SecondId);
	});
	TestTrue(TEXT("Reused projectile can hit the same target again"), Reused->ProcessBlockingHit(MothPoolTest::BlockingHit(Target)));
	TestEqual(TEXT("New hit ID passes target deduplication"), Target->GetHealthComponent()->GetCurrentHealth(), 40.0f);
	TestEqual(TEXT("Per-flight observers do not leak into later flights"), OriginalObserverCalls, 1);
	Reused->ReturnToPool();
	Reused->ReturnToPool();
	TestEqual(TEXT("Repeated recycling leaves one available projectile"), Pool->GetInactiveCount(), 1);
	TestEqual(TEXT("Two flights required one allocation"), Pool->GetCreatedCount(), 1);
	TestEqual(TEXT("One flight reused a cached actor"), Pool->GetReuseCount(), 1);
	auto* CheckoutA = MothPoolTest::Projectile(Fixture);
	auto* CheckoutB = MothPoolTest::Projectile(Fixture);
	TestTrue(TEXT("One cached actor cannot be checked out twice concurrently"), CheckoutA && CheckoutB && CheckoutA != CheckoutB);
	TestEqual(TEXT("Concurrent demand allocates one additional actor"), Pool->GetCreatedCount(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothProjectileOldSourceLife,
	"MothEffect.Pooling.InFlightProjectileCannotGrantImmunityToReusedSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothProjectileOldSourceLife::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Old source identity world"), Fixture.World)) return false;
	auto* Source = MothPoolTest::Enemy(Fixture, FVector(0, 0, 1000));
	if (!TestNotNull(TEXT("Original source life"), Source)) return false;
	auto* Shot = MothPoolTest::Projectile(Fixture, Source, Source, FVector::ForwardVector, FVector(0, 500, 1000));
	if (!TestNotNull(TEXT("Original life has an independent in-flight shot"), Shot)) return false;
	auto* Sphere = Shot->FindComponentByClass<USphereComponent>();
	auto* Movement = Shot->FindComponentByClass<UProjectileMovementComponent>();
	const FGuid FlightId = Shot->GetHitId();
	const FVector Velocity = Movement->Velocity;
	const float RemainingLife = Shot->GetLifeSpan();
	TestTrue(TEXT("Original shot initially ignores its firing actor"), Sphere->GetMoveIgnoreActors().Contains(Source));
	Source->ReturnToPool();
	TestTrue(TEXT("Returning the shooter leaves its old shot in independent flight"), !Shot->IsPoolInactive() && !Shot->HasProcessedImpact());
	TestFalse(TEXT("Old flight no longer grants immunity through the source actor pointer"), Sphere->GetMoveIgnoreActors().Contains(Source));
	TestNull(TEXT("Old flight detaches its recycled owner"), Shot->GetOwner());
	TestNull(TEXT("Old flight detaches its recycled instigator"), Shot->GetInstigator());
	TestTrue(TEXT("Detachment preserves the original flight's hit identity and direction"), Shot->GetHitId() == FlightId && Shot->GetFlightDirection().Equals(FVector::ForwardVector));
	TestTrue(TEXT("Detachment preserves the original flight's velocity and remaining lifetime"),
		Movement->IsActive() && Movement->Velocity.Equals(Velocity) && FMath::IsNearlyEqual(Shot->GetLifeSpan(), RemainingLife));
	auto* NewLife = MothPoolTest::Enemy(Fixture, FVector(1000, 0, 1000));
	if (!TestNotNull(TEXT("Shooter actor is reused for a new life"), NewLife)) return false;
	TestTrue(TEXT("The new life uses the identical actor pointer"), NewLife == Source);
	bool bObservedImpact = false;
	Shot->OnImpactNative.AddLambda([this, &bObservedImpact, FlightId](ARuleProjectile*, AActor*, const FHitContext& Context)
	{
		bObservedImpact = true;
		TestTrue(TEXT("Old projectile retains its own hit ID on the new life's impact"), Context.HitId == FlightId);
		TestNull(TEXT("Impact cannot attribute an old shot to the recycled source life"), Context.SourceActor.Get());
		TestNull(TEXT("Impact cannot attribute an old shot to the recycled instigator life"), Context.InstigatorPawn.Get());
	});
	TestTrue(TEXT("Old shot can now hit its former actor's new life"), Shot->ProcessBlockingHit(MothPoolTest::BlockingHit(NewLife)));
	TestTrue(TEXT("Actual common projectile impact was observed"), bObservedImpact);
	TestEqual(TEXT("New life takes the old projectile's real damage without stale immunity"), NewLife->GetHealthComponent()->GetCurrentHealth(), 50.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothProjectilePoolReentrantImpact,
	"MothEffect.Pooling.ProjectileCannotBeReusedDuringImpactCallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothProjectilePoolReentrantImpact::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Reentrant pool world"), Fixture.World)) return false;
	auto* Target = Fixture.Spawn<AEnemyBase>(FVector(500, 0, 92));
	Target->SetActorTickEnabled(false);
	Target->GetCharacterMovement()->DisableMovement();
	auto* Original = MothPoolTest::Projectile(Fixture);
	if (!TestNotNull(TEXT("Original shot"), Original)) return false;
	ARuleProjectile* SpawnedInCallback = nullptr;
	Original->OnImpactNative.AddLambda([&](ARuleProjectile* Impacting, AActor*, const FHitContext&)
	{
		Impacting->ReturnToPool();
		Impacting->ReturnToPool();
		SpawnedInCallback = MothPoolTest::Projectile(Fixture);
		TestTrue(TEXT("An in-progress impact cannot lend its actor to a nested shot"), SpawnedInCallback && SpawnedInCallback != Impacting);
		TestFalse(TEXT("The impact guard survives a reentrant recycle request"), Impacting->ProcessBlockingHit(MothPoolTest::BlockingHit(Target)));
	});
	TestTrue(TEXT("Outer impact completes"), Original->ProcessBlockingHit(MothPoolTest::BlockingHit(Target)));
	TestTrue(TEXT("Original becomes inactive after dispatch completes"), Original->IsPoolInactive());
	TestEqual(TEXT("Only the nested shot remains live"), Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(), 1);
	if (SpawnedInCallback) SpawnedInCallback->ReturnToPool();
	TestEqual(TEXT("Both distinct actors are cached exactly once"), Fixture.World->GetSubsystem<UProjectilePoolSubsystem>()->GetInactiveCount(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothPoolWorldIsolationAndReset,
	"MothEffect.Pooling.WorldIsolationResetBoundsAndDuplicateRecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothPoolWorldIsolationAndReset::RunTest(const FString&)
{
	FMothCombatTestWorld A, B;
	if (!TestNotNull(TEXT("First pool world"), A.World) || !TestNotNull(TEXT("Second pool world"), B.World)) return false;
	auto* EnemyPoolA = A.World->GetSubsystem<UEnemyPoolSubsystem>();
	auto* EnemyPoolB = B.World->GetSubsystem<UEnemyPoolSubsystem>();
	auto* ProjectilePoolA = A.World->GetSubsystem<UProjectilePoolSubsystem>();
	auto* ProjectilePoolB = B.World->GetSubsystem<UProjectilePoolSubsystem>();
	auto* EnemyA = MothPoolTest::Enemy(A);
	auto* EnemyB = MothPoolTest::Enemy(B);
	auto* ShotA = MothPoolTest::Projectile(A);
	auto* ShotB = MothPoolTest::Projectile(B);
	if (!TestNotNull(TEXT("World A enemy"), EnemyA) || !TestNotNull(TEXT("World B enemy"), EnemyB)
		|| !TestNotNull(TEXT("World A projectile"), ShotA) || !TestNotNull(TEXT("World B projectile"), ShotB)) return false;
	EnemyPoolB->RecycleEnemy(EnemyA);
	ProjectilePoolB->RecycleProjectile(ShotA);
	TestFalse(TEXT("Foreign enemy cannot enter another world's pool"), EnemyA->IsPoolInactive());
	TestFalse(TEXT("Foreign projectile cannot enter another world's pool"), ShotA->IsPoolInactive());
	TestEqual(TEXT("Foreign recycle adds no enemy entries"), EnemyPoolB->GetInactiveCount(), 0);
	TestEqual(TEXT("Foreign recycle adds no projectile entries"), ProjectilePoolB->GetInactiveCount(), 0);
	EnemyA->ReturnToPool();
	EnemyA->ReturnToPool();
	TestEqual(TEXT("Enemy free list stores an actor once"), EnemyPoolA->GetInactiveCount(), 1);
	auto* FirstCheckout = MothPoolTest::Enemy(A);
	auto* SecondCheckout = MothPoolTest::Enemy(A, FVector(500, 0, 92));
	TestTrue(TEXT("Duplicate enemy recycling cannot lend one actor to two callers"), FirstCheckout == EnemyA && SecondCheckout && FirstCheckout != SecondCheckout);
	auto* LifecycleA = A.World->GetSubsystem<UDeviceLifecycleSubsystem>();
	LifecycleA->ResetManagedObjects();
	TestTrue(TEXT("Run reset recycles its pooled projectile"), ShotA->IsPoolInactive() && !ShotA->IsActorBeingDestroyed());
	TestEqual(TEXT("Run reset clears the first live projectile budget"), LifecycleA->GetProjectileCount(), 0);
	TestFalse(TEXT("Another world's projectile remains in flight"), ShotB->IsPoolInactive());
	TestTrue(TEXT("Another world's enemy remains active"), EnemyB->IsCombatEnabled());
	TestEqual(TEXT("Other live projectile budget is unchanged"), B.World->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(), 1);
	auto* AfterReset = MothPoolTest::Projectile(A);
	if (!TestNotNull(TEXT("Admission reopens after reset"), AfterReset)) return false;
	TestTrue(TEXT("Reset keeps the cached allocation available"), AfterReset == ShotA);
	LifecycleA->SetSafetyBounds(FVector::ZeroVector, FVector(100, 100, 2000));
	AfterReset->SetActorLocation(FVector(300, 0, 1000));
	LifecycleA->Tick(0.01f);
	TestTrue(TEXT("Arena safety cleanup recycles an escaped pooled projectile"), AfterReset->IsPoolInactive());
	TestEqual(TEXT("Safety cleanup releases its budget"), LifecycleA->GetProjectileCount(), 0);
	LifecycleA->SetGameplayEnabled(false);
	TestNull(TEXT("Closed session cannot check out an enemy"), MothPoolTest::Enemy(A));
	TestNull(TEXT("Closed session cannot check out a projectile"), MothPoolTest::Projectile(A));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothEnemyPoolBlockedBirth,
	"MothEffect.Pooling.BlockedBirthNeverActivatesFreshOrReusedEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothEnemyPoolBlockedBirth::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Blocked reuse world"), Fixture.World)) return false;
	auto* Pool = Fixture.World->GetSubsystem<UEnemyPoolSubsystem>();
	auto* Original = MothPoolTest::Enemy(Fixture, FVector(0, 0, 1000));
	if (!TestNotNull(TEXT("Original unobstructed pooled enemy"), Original)) return false;
	Original->ReturnToPool();
	const FTransform BlockedTransform(FVector(2000, 0, 1000));
	auto* Blocker = MothPoolTest::BlockingBox(Fixture, BlockedTransform.GetLocation());
	auto* Checkout = Pool->AcquireEnemy(AEnemyBase::StaticClass(), BlockedTransform, nullptr);
	if (!TestNotNull(TEXT("Cached enemy checkout remains inert until final admission"), Checkout)) return false;
	TestTrue(TEXT("Blocked checkout reuses the existing actor"), Checkout == Original);
	Pool->FinishSpawningEnemy(Checkout, BlockedTransform);
	TestTrue(TEXT("Blocking geometry keeps the reused enemy inactive and retained"),
		IsValid(Checkout) && !Checkout->IsActorBeingDestroyed() && Checkout->IsPoolInactive());
	if (!IsValid(Checkout) || Checkout->IsActorBeingDestroyed()) return false;
	TestFalse(TEXT("A rejected birth never enables combat"), Checkout->IsCombatEnabled());
	TestTrue(TEXT("A rejected birth keeps actor collision and ticks disabled"),
		!Checkout->GetActorEnableCollision() && !Checkout->IsActorTickEnabled());
	const float InactiveHealth = Checkout->GetHealthComponent()->GetCurrentHealth();
	TestFalse(TEXT("Direct component hits cannot damage a rejected birth"), Checkout->GetHealthComponent()->ApplyHit(MothPoolTest::Damage(10)));
	TestEqual(TEXT("Rejected birth health remains unchanged"), Checkout->GetHealthComponent()->GetCurrentHealth(), InactiveHealth);
	TestEqual(TEXT("Rejected checkout returns one cached actor"), Pool->GetInactiveCount(), 1);
	Blocker->SetActorLocation(FVector(6000, 0, 1000));
	auto* Cleared = MothPoolTest::Enemy(Fixture, BlockedTransform.GetLocation());
	if (!TestNotNull(TEXT("Removing the physical obstruction permits a fresh admission"), Cleared)) return false;
	TestTrue(TEXT("Admission after obstacle removal retains the same pooled actor"), Cleared == Original && Cleared->IsCombatEnabled());
	TestTrue(TEXT("Unblocked birth restores collision and its announced position"),
		Cleared->GetActorEnableCollision() && Cleared->GetActorLocation().Equals(BlockedTransform.GetLocation(), 0.01));

	FMothCombatTestWorld Fresh;
	if (!TestNotNull(TEXT("Blocked fresh birth world"), Fresh.World)) return false;
	auto* FreshPool = Fresh.World->GetSubsystem<UEnemyPoolSubsystem>();
	MothPoolTest::BlockingBox(Fresh, BlockedTransform.GetLocation());
	TestNull(TEXT("A fresh allocation cannot spawn inside existing blocking geometry"),
		FreshPool->AcquireEnemy(AEnemyBase::StaticClass(), BlockedTransform, nullptr));
	const FTransform LateBlockedTransform(FVector(3500, 0, 1000));
	auto* Deferred = FreshPool->AcquireEnemy(AEnemyBase::StaticClass(), LateBlockedTransform, nullptr);
	if (!TestNotNull(TEXT("Fresh allocation can be reserved while its birthplace is clear"), Deferred)) return false;
	MothPoolTest::BlockingBox(Fresh, LateBlockedTransform.GetLocation());
	FreshPool->FinishSpawningEnemy(Deferred, LateBlockedTransform);
	TestTrue(TEXT("An obstacle appearing before FinishSpawning cancels the fresh birth"),
		!IsValid(Deferred) || Deferred->IsActorBeingDestroyed() || Deferred->IsPoolInactive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothPoolIdleRetention,
	"MothEffect.Pooling.IdleRetentionBoundsMemoryWithoutLimitingActiveDemand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothPoolIdleRetention::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Retention test world"), Fixture.World)) return false;
	auto* EnemyPool = Fixture.World->GetSubsystem<UEnemyPoolSubsystem>();
	auto* ProjectilePool = Fixture.World->GetSubsystem<UProjectilePoolSubsystem>();
	auto* EnemyLimit = FindFProperty<FIntProperty>(UEnemyPoolSubsystem::StaticClass(), TEXT("MaxRetainedEnemies"));
	auto* ProjectileLimit = FindFProperty<FIntProperty>(UProjectilePoolSubsystem::StaticClass(), TEXT("MaxRetainedProjectiles"));
	if (!TestNotNull(TEXT("Configurable enemy idle retention"), EnemyLimit)
		|| !TestNotNull(TEXT("Configurable projectile idle retention"), ProjectileLimit)) return false;
	EnemyLimit->SetPropertyValue_InContainer(EnemyPool, 1);
	ProjectileLimit->SetPropertyValue_InContainer(ProjectilePool, 1);
	auto* EnemyA = MothPoolTest::Enemy(Fixture, FVector(0, 0, 1000));
	auto* EnemyB = MothPoolTest::Enemy(Fixture, FVector(1000, 0, 1000));
	auto* ShotA = MothPoolTest::Projectile(Fixture, nullptr, nullptr, FVector::ForwardVector, FVector(0, 1000, 1000));
	auto* ShotB = MothPoolTest::Projectile(Fixture, nullptr, nullptr, FVector::ForwardVector, FVector(1000, 1000, 1000));
	if (!TestNotNull(TEXT("First active enemy"), EnemyA) || !TestNotNull(TEXT("Second active enemy"), EnemyB)
		|| !TestNotNull(TEXT("First active projectile"), ShotA) || !TestNotNull(TEXT("Second active projectile"), ShotB)) return false;
	TestTrue(TEXT("An idle retention of one still admits two distinct active enemies"), EnemyA != EnemyB && EnemyA->IsCombatEnabled() && EnemyB->IsCombatEnabled());
	TestEqual(TEXT("An idle retention of one still admits two live projectiles"), Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(), 2);
	EnemyA->ReturnToPool();
	EnemyB->ReturnToPool();
	ShotA->ReturnToPool();
	ShotB->ReturnToPool();
	TestEqual(TEXT("Enemy cache obeys its configured retention ceiling"), EnemyPool->GetInactiveCount(), 1);
	TestEqual(TEXT("Projectile cache obeys its configured retention ceiling"), ProjectilePool->GetInactiveCount(), 1);
	TestTrue(TEXT("Surplus returned enemy is destroyed without a death drop"), EnemyB->IsActorBeingDestroyed() && EnemyB->GetDropAttempts() == 0);
	TestTrue(TEXT("Surplus returned projectile is destroyed"), ShotB->IsActorBeingDestroyed());
	TestEqual(TEXT("Both projectile returns release live admission quota"), Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(), 0);
	auto* ReusedEnemy = MothPoolTest::Enemy(Fixture, FVector(2000, 0, 1000));
	auto* ReusedShot = MothPoolTest::Projectile(Fixture);
	TestTrue(TEXT("Retained enemy remains available for normal reuse"), ReusedEnemy && ReusedEnemy == EnemyA && ReusedEnemy->IsCombatEnabled());
	TestTrue(TEXT("Retained projectile remains available for normal reuse"), ReusedShot && ReusedShot == ShotA && !ReusedShot->IsPoolInactive());
	TestEqual(TEXT("Retention overflow does not allocate again on the next enemy checkout"), EnemyPool->GetCreatedCount(), 2);
	TestEqual(TEXT("Retention overflow does not allocate again on the next projectile checkout"), ProjectilePool->GetCreatedCount(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothPoolRealTimers,
	"MothEffect.Pooling.RealExpiryKillZAndNoStaleEnemyDeathTimer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothPoolRealTimers::RunTest(const FString&)
{
	ADD_LATENT_AUTOMATION_COMMAND(MothPoolTest::FExpiryAndTimerReuse(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothWaveEnemyScaling,
	"MothEffect.Pooling.ConfigurableWaveScalingAndRuntimeLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothWaveEnemyScaling::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Scaled waves world"), Fixture.World)) return false;
	// This one scenario loads the cooked archetype references required by StartEncounter;
	// the enemy/projectile lifecycle tests above use native actors exclusively.
	auto* DirectorClass = LoadClass<AMothWaveDirector>(nullptr, TEXT("/Game/MothEffect/Framework/BP_WaveDirector.BP_WaveDirector_C"));
	if (!TestNotNull(TEXT("Configured wave director class"), DirectorClass)) return false;
	auto* Director = Fixture.World->SpawnActor<AMothWaveDirector>(DirectorClass);
	if (!TestNotNull(TEXT("Configured wave director"), Director)) return false;
	TestTrue(TEXT("Ready director accepts baseline density"), Director->SetWaveEnemyMultiplier(1));
	const TArray<FMothWaveDefinition> Baseline = Director->GetWaves();
	TestEqual(TEXT("Baseline still has three waves"), Baseline.Num(), 3);
	if (Baseline.Num() != 3) return false;
	TestTrue(TEXT("Ready director accepts three times density"), Director->SetWaveEnemyMultiplier(3));
	TestEqual(TEXT("Configured density is observable"), Director->GetWaveEnemyMultiplier(), 3);
	const TArray<FMothWaveDefinition>& Scaled = Director->GetWaves();
	const int32 ExpectedTotals[] = { 12, 18, 24 };
	if (!TestEqual(TEXT("Scaling preserves three wave definitions"), Scaled.Num(), 3)) return false;
	for (int32 Wave = 0; Wave < 3; ++Wave)
	{
		if (!TestEqual(TEXT("Unscaled wave matches the original baseline"), Baseline[Wave].Spawns.Num(), ExpectedTotals[Wave] / 3)) return false;
		TestEqual(*FString::Printf(TEXT("Expanded wave %d actual spawn count"), Wave + 1), Scaled[Wave].Spawns.Num(), ExpectedTotals[Wave]);
		int32 BaselineBatches = 0;
		for (const FMothWaveSpawn& Entry : Baseline[Wave].Spawns) BaselineBatches = FMath::Max(BaselineBatches, Entry.Batch + 1);
		for (int32 Index = 0; Index < Scaled[Wave].Spawns.Num(); ++Index)
		{
			const FMothWaveSpawn& Actual = Scaled[Wave].Spawns[Index];
			const FMothWaveSpawn& Original = Baseline[Wave].Spawns[Index % Baseline[Wave].Spawns.Num()];
			const int32 Copy = Index / Baseline[Wave].Spawns.Num();
			TestTrue(TEXT("Scaling preserves each original point and fixed drop type"), Actual.Point == Original.Point && Actual.Kind == Original.Kind);
			TestEqual(TEXT("Extra copies occupy later batches instead of one burst"), Actual.Batch, Original.Batch + Copy * BaselineBatches);
		}
	}
	TestTrue(TEXT("Repeated density changes rebuild from the baseline"), Director->SetWaveEnemyMultiplier(3));
	TestEqual(TEXT("Repeated rebuild does not multiply the expanded data again"), Director->GetWaves()[0].Spawns.Num(), 12);
	auto* Player = Fixture.Spawn<AMothPlayerAutomationCharacter>(FVector(-2200, -1800, 100));
	TestTrue(TEXT("Scaled director accepts a real player"), Director->SetPlayer(Player));
	TestTrue(TEXT("Scaled encounter starts"), Director->StartEncounter());
	TestEqual(TEXT("The full expanded first wave is queued"), Director->GetPendingCount(), 12);
	TestFalse(TEXT("A running encounter cannot change its wave definitions"), Director->SetWaveEnemyMultiplier(1));
	TestEqual(TEXT("Rejected density changes preserve the pending wave"), Director->GetPendingCount(), 12);
	return true;
}

#endif
