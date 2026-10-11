#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "Engine/Level.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Tests/CombatTestWorld.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "TimerManager.h"
#include "Types/MothCollisionChannels.h"
#include "UObject/UnrealType.h"

namespace
{
	FHitContext StarterHit()
	{
		FHitContext Hit;
		Hit.HitId = FGuid::NewGuid();
		Hit.ShotDirection = FVector::ForwardVector;
		return Hit;
	}

	void StaticBox(FMothCombatTestWorld& F, FVector Position, FVector Extent, bool bGround = false)
	{
		AActor* Actor = F.Spawn<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->AddInstanceComponent(Box);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		if (bGround) Box->ComponentTags.Add(TEXT("MothColumnSurface"));
		Box->RegisterComponent();
		Box->SetWorldLocation(Position);
	}

	ADeviceBase* ChainDevice(FMothCombatTestWorld& F, FVector Position, EDeviceKind Kind, bool bSaved = false)
	{
		UClass* Class = ADeviceBase::StaticClass();
		if (bSaved)
		{
			const TCHAR* Path = Kind == EDeviceKind::Bomb
				? TEXT("/Game/MothEffect/Devices/BP_DeviceBomb.BP_DeviceBomb_C")
				: Kind == EDeviceKind::Launcher
				? TEXT("/Game/MothEffect/Devices/BP_DeviceLauncher.BP_DeviceLauncher_C")
				: TEXT("/Game/MothEffect/Devices/BP_DeviceEmitter.BP_DeviceEmitter_C");
			Class = LoadClass<ADeviceBase>(nullptr, Path);
		}
		if (!Class) return nullptr;
		const FTransform Transform(FRotator::ZeroRotator, Position);
		ADeviceBase* Device = F.World->SpawnActorDeferred<ADeviceBase>(Class, Transform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Device) return nullptr;
		if (!bSaved)
		{
			FEnumProperty* Property = FindFProperty<FEnumProperty>(ADeviceBase::StaticClass(), TEXT("DeviceKind"));
			Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Device), static_cast<int64>(Kind));
		}
		FindFProperty<FBoolProperty>(ADeviceBase::StaticClass(), TEXT("bLogDeviceEvents"))->SetPropertyValue_InContainer(Device, true);
		Device->FinishSpawning(Transform);
		return Device;
	}

	enum class EChainCase { DirectEmitter, DirectBomb, DirectLauncher, BombCrossing, ColumnCrossing,
		PawnDamage, ThinWall, IndependentLifetime, SavedBlueprints };

	/** Only arrange initial state and start the source/mover. All subsequent hits, motion,
	 * activation, timers and expiry come from real World ticks, never scripted target hits. */
	class FNaturalChain final : public IAutomationLatentCommand
	{
	public:
		FNaturalChain(FAutomationTestBase* InTest, EChainCase InCase) : Test(InTest), Case(InCase) {}
		bool Update() override
		{
			if (!F) return Setup();
			if (F->World->GetTimerManager().HasBeenTickedThisFrame()) return false;
			ObserveProjectiles();
			const bool bPaused = F->World->IsPaused();
			F->World->Tick(LEVELTICK_All, 0.01f);
			if (bPaused)
			{
				Test->TestEqual(TEXT("Actual paused World Tick freezes gameplay time"), F->World->GetTimeSeconds(), PausedTime);
				Test->TestTrue(TEXT("Pause freezes flight and independent lifetime"), Projectile.IsValid()
					&& Projectile->GetActorLocation().Equals(PausedPosition));
				if (--PauseFrames == 0) F->World->GetWorldSettings()->SetPauserPlayerState(nullptr);
				return false;
			}
			const double Elapsed = F->World->GetTimeSeconds();
			if (Target.IsValid()) MaxTargetZ = FMath::Max(MaxTargetZ, Target->GetActorLocation().Z);
			if (Case == EChainCase::IndependentLifetime)
			{
				if (!bSourceRemoved && Elapsed >= 0.1f)
				{
					Source->DestroyDevice();
					bSourceRemoved = true;
					PausedTime = Elapsed;
					PausedPosition = Projectile->GetActorLocation();
					PauseFrames = 10;
					F->World->GetWorldSettings()->SetPauserPlayerState(F->Spawn<APlayerState>());
				}
				if (Elapsed >= 3.1f)
				{
					Test->TestFalse(TEXT("Removed owner does not extend the projectile's three-second lifetime"), Projectile.IsValid());
					Test->TestEqual(TEXT("No phantom impact on owner removal or expiry"), ImpactCount, 0);
					return true;
				}
				return false;
			}
			if (Case == EChainCase::PawnDamage)
			{
				if (Elapsed < 0.4f) return false;
				Test->TestEqual(TEXT("Real swept P01 damages its attributed instigator without immunity"),
					Victim->FindComponentByClass<UHealthComponent>()->GetCurrentHealth(), 90.0f);
				Test->TestEqual(TEXT("First blocking character consumes the projectile exactly once"), ImpactCount, 1);
				Test->TestEqual(TEXT("No activation behind the first blocker"), TargetActivations, 0);
				Test->TestFalse(TEXT("Flight cannot continue through the character"), Projectile.IsValid());
				return true;
			}
			const bool bLayoutPair = Case == EChainCase::BombCrossing || Case == EChainCase::ColumnCrossing;
			// A blast can move the emitter away from the backstop. Its final projectile
			// may legitimately live until 2.85 + 3 seconds, independently of the source.
			const float FinishAt = bLayoutPair ? 0.9f : 6.2f;
			if (Elapsed < FinishAt) return false;
			Test->AddInfo(FString::Printf(TEXT("Chain case=%d layout=%d activations=%d targetImpacts=%d impulseStart=%s activatedAt=%s peakZ=%.1f"),
				static_cast<int32>(Case), Layout, TargetActivations, TargetImpactCount, *InitialTarget.ToString(), *ActivationPosition.ToString(), MaxTargetZ));
			const bool bExpectedHit = Case != EChainCase::ThinWall && (!bLayoutPair || Layout == 0);
			Test->TestEqual(TEXT("Placement changes the ordinary ballistic activation outcome"), TargetActivations, bExpectedHit ? 1 : 0);
			if (bExpectedHit)
			{
				Test->TestTrue(TEXT("Target was activated by a real projectile with its unique HitId"), bMatchingImpact);
				Test->TestTrue(TEXT("Chain activation locks the actual world trajectory"), ActivationDirection.Equals(FVector::ForwardVector));
				if (bLayoutPair)
					Test->TestTrue(TEXT("Impulse physically moved target before the ballistic activation"), !ActivationPosition.Equals(InitialTarget, 50.0));
				if (Case == EChainCase::DirectLauncher)
					Test->TestTrue(TEXT("P01 activation created an actual column"), bColumnCreated);
			}
			if (!bLayoutPair)
			{
				Test->TestEqual(TEXT("Original source keeps its finite twelve-shot schedule"), SourceAttempts, 12);
				Test->TestEqual(TEXT("Backstop and finite lifetimes leave no flying projectiles"), CountProjectiles(), 0);
				Test->TestFalse(TEXT("Original emitter reaches actual timed destruction"), Source.IsValid());
				if (bExpectedHit) Test->TestFalse(TEXT("Activated target reaches its own finite end"), Target.IsValid());
			}
			if (bLayoutPair && Layout++ == 0)
			{
				F.Reset();
				Seen.Reset();
				TargetActivations = TargetImpactCount = ImpactCount = 0;
				bMatchingImpact = bColumnCreated = false;
				ActivationPosition = ActivationDirection = FVector::ZeroVector;
				return false;
			}
			return true;
		}

	private:
		bool Setup()
		{
			F = MakeUnique<FMothCombatTestWorld>();
			if (!Test->TestNotNull(TEXT("Create real chain world"), F->World)) return true;
			StaticBox(*F, FVector(0.0, 0.0, -10.0), FVector(5000.0, 5000.0, 10.0), true);
			StaticBox(*F, FVector(1100.0, 0.0, 450.0), FVector(10.0, 1800.0, 450.0));
			if (Case == EChainCase::PawnDamage || Case == EChainCase::IndependentLifetime)
			{
				APawn* Attribution = nullptr;
				AActor* FiringActor = nullptr;
				FVector Birth(0.0, 0.0, 100.0);
				if (Case == EChainCase::PawnDamage)
				{
					FiringActor = F->Spawn<AMothPlayerAutomationCharacter>(Birth);
					Victim = F->Spawn<AMothPlayerAutomationCharacter>(FVector(300.0, 0.0, 100.0));
					for (AMothPlayerAutomationCharacter* Pawn : {Cast<AMothPlayerAutomationCharacter>(FiringActor), Victim})
					{
						if (!Pawn) { Test->AddError(TEXT("Spawn representative character")); return true; }
						Pawn->GetCharacterMovement()->GravityScale = 0.0f;
						Pawn->GetCharacterMovement()->bRunPhysicsWithNoController = true;
						Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
					}
					Attribution = Victim;
					Target = ChainDevice(*F, FVector(600.0, 0.0, 20.0), EDeviceKind::Emitter);
					ObserveTarget();
				}
				else
				{
					// Above the backstop: only actual game-time lifetime can end this flight.
					Birth.Z = 1500.0;
					Source = ChainDevice(*F, Birth, EDeviceKind::Emitter);
					FiringActor = Source.Get();
				}
				const FTransform Transform(FRotator::ZeroRotator, Birth);
				Projectile = F->World->SpawnActorDeferred<ARuleProjectile>(ARuleProjectile::StaticClass(), Transform,
					FiringActor, Attribution, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (!Test->TestTrue(TEXT("Initialize common P01 from the firing actor"), Projectile.IsValid()
					&& Projectile->InitializeProjectile(FVector::ForwardVector, FiringActor, Attribution))) return true;
				Projectile->SetEventLoggingEnabled(true);
				Projectile->FinishSpawning(Transform);
				return false;
			}
			const bool bColumn = Case == EChainCase::ColumnCrossing;
			Source = ChainDevice(*F, bColumn ? FVector(-190.0, 0.0, 400.0) : FVector(0.0, 0.0, 20.0),
				EDeviceKind::Emitter, Case == EChainCase::SavedBlueprints);
			InitialTarget = bColumn ? FVector(250.0, Layout == 0 ? 0.0 : 300.0, 20.0)
				: Case == EChainCase::BombCrossing ? FVector(250.0, Layout == 0 ? 280.0 : 600.0, 20.0)
				: FVector(250.0, 0.0, 20.0);
			const EDeviceKind Kind = Case == EChainCase::DirectBomb || Case == EChainCase::SavedBlueprints
				? EDeviceKind::Bomb : Case == EChainCase::DirectLauncher ? EDeviceKind::Launcher : EDeviceKind::Emitter;
			Target = ChainDevice(*F, InitialTarget, Kind, Case == EChainCase::SavedBlueprints);
			if (!Test->TestTrue(TEXT("Spawn ordinary source and dormant target"), Source.IsValid() && Target.IsValid())) return true;
			MaxTargetZ = InitialTarget.Z;
			ObserveTarget();
			if (bColumn) StaticBox(*F, FVector(-190.0, 0.0, 370.0), FVector(60.0, 60.0, 10.0), true);
			if (Case == EChainCase::ThinWall) StaticBox(*F, FVector(100.0, 0.0, 150.0), FVector(1.0, 100.0, 150.0));
			Test->TestTrue(TEXT("Ballistic starter activates only the source"), Source->TryActivate(StarterHit()));
			Source->OnDeviceStateChangedNative.AddLambda([this](ADeviceBase* Device, EDeviceState Old, EDeviceState New)
			{
				if (New == EDeviceState::Spent) SourceAttempts = Device->GetEmitterShotAttempts();
			});
			if (Case == EChainCase::BombCrossing || bColumn)
			{
				ADeviceBase* Mover = ChainDevice(*F, bColumn ? FVector(250.0, 0.0, 800.0) : FVector(250.0, 500.0, 20.0),
					bColumn ? EDeviceKind::Launcher : EDeviceKind::Bomb);
				if (!Test->TestNotNull(TEXT("Spawn ordinary impulse source"), Mover)) return true;
				Test->TestTrue(TEXT("Ballistic starter activates only the impulse source"), Mover->TryActivate(StarterHit()));
				Test->TestTrue(TEXT("Blast/column do not directly activate the target"), Target->GetDeviceState() == EDeviceState::Dormant);
			}
			return false;
		}

		void ObserveTarget()
		{
			Target->OnDeviceStateChangedNative.AddLambda([this](ADeviceBase* Device, EDeviceState Old, EDeviceState New)
			{
				if (New != EDeviceState::Active) return;
				++TargetActivations;
				ActivationId = Device->GetActivationHitId();
				ActivationPosition = Device->GetActorLocation();
				ActivationDirection = Device->GetActivationDirection();
			});
		}
		void ObserveProjectiles()
		{
			for (TActorIterator<ARuleProjectile> It(F->World); It; ++It)
			{
				if (Seen.Contains(It->GetHitId())) continue;
				Seen.Add(It->GetHitId());
				It->OnImpactNative.AddLambda([this](ARuleProjectile* P, AActor* Actor, const FHitContext& Hit)
				{
					++ImpactCount;
					if (Actor != Target.Get()) return;
					++TargetImpactCount;
					bMatchingImpact |= Hit.HitId == ActivationId && Hit.SourceActor.Get() == Source.Get();
					if (Case == EChainCase::DirectLauncher) bColumnCreated |= Target->GetLaunchColumn() != nullptr;
				});
			}
		}
		int32 CountProjectiles() const
		{
			int32 Count = 0;
			for (TActorIterator<ARuleProjectile> It(F->World); It; ++It) ++Count;
			return Count;
		}
		FAutomationTestBase* Test;
		EChainCase Case;
		TUniquePtr<FMothCombatTestWorld> F;
		TWeakObjectPtr<ADeviceBase> Source, Target;
		TWeakObjectPtr<ARuleProjectile> Projectile;
		AMothPlayerAutomationCharacter* Victim = nullptr;
		TSet<FGuid> Seen;
		FGuid ActivationId;
		FVector InitialTarget = FVector::ZeroVector, ActivationPosition = FVector::ZeroVector;
		FVector ActivationDirection = FVector::ZeroVector, PausedPosition = FVector::ZeroVector;
		int32 Layout = 0, TargetActivations = 0, TargetImpactCount = 0, ImpactCount = 0, SourceAttempts = 0, PauseFrames = 0;
		float MaxTargetZ = 0.0f;
		double PausedTime = 0.0;
		bool bMatchingImpact = false, bColumnCreated = false, bSourceRemoved = false;
	};
}

#define MOTH_CHAIN_TEST(Name, Suffix, Case) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(Name, "MothEffect.Chains." Suffix, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool Name::RunTest(const FString& Parameters) { ADD_LATENT_AUTOMATION_COMMAND(FNaturalChain(this, EChainCase::Case)); return true; }

MOTH_CHAIN_TEST(FMothChainEmitter, "DirectEmitter", DirectEmitter)
MOTH_CHAIN_TEST(FMothChainBomb, "DirectBomb", DirectBomb)
MOTH_CHAIN_TEST(FMothChainLauncher, "DirectLauncher", DirectLauncher)
MOTH_CHAIN_TEST(FMothChainBlast, "BombIntoFirelineAndLayoutControl", BombCrossing)
MOTH_CHAIN_TEST(FMothChainColumn, "ColumnIntoFirelineAndLayoutControl", ColumnCrossing)
MOTH_CHAIN_TEST(FMothChainDamage, "SourceExclusionAndAttributionDamage", PawnDamage)
MOTH_CHAIN_TEST(FMothChainThinWall, "ThinWallConsumesFirstImpact", ThinWall)
MOTH_CHAIN_TEST(FMothChainLifetime, "PausedFlightAndIndependentLifetime", IndependentLifetime)
MOTH_CHAIN_TEST(FMothChainSavedBlueprints, "SavedBlueprintNaturalChain", SavedBlueprints)

#undef MOTH_CHAIN_TEST

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothSavedChainRange, "MothEffect.Chains.SavedRangeConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothSavedChainRange::RunTest(const FString& Parameters)
{
	UWorld* Range = LoadObject<UWorld>(nullptr, TEXT("/Game/MothEffect/Tests/L_T12Chains.L_T12Chains"));
	if (!TestNotNull(TEXT("Reload the saved ordinary-actor test map"), Range) || !Range->PersistentLevel) return false;
	int32 Devices = 0, Emitters = 0, Bombs = 0, Launchers = 0, Grounds = 0;
	for (AActor* Actor : Range->PersistentLevel->Actors)
	{
		if (!Actor) continue;
		if (ADeviceBase* Device = Cast<ADeviceBase>(Actor))
		{
			++Devices;
			Emitters += Device->GetDeviceKind() == EDeviceKind::Emitter;
			Bombs += Device->GetDeviceKind() == EDeviceKind::Bomb;
			Launchers += Device->GetDeviceKind() == EDeviceKind::Launcher;
			TestTrue(TEXT("Saved range has dormant devices, with no automatic chain"), Device->GetDeviceState() == EDeviceState::Dormant);
			TestTrue(TEXT("Per-instance developer evidence survives saving"),
				FindFProperty<FBoolProperty>(ADeviceBase::StaticClass(), TEXT("bLogDeviceEvents"))->GetPropertyValue_InContainer(Device));
		}
		if (Actor->ActorHasTag(TEXT("MothColumnSurface")))
		{
			++Grounds;
			const UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
			TestTrue(TEXT("Saved tagged floors have a queryable WorldStatic blocking response"), Body
				&& Body->GetCollisionObjectType() == ECC_WorldStatic
				&& Body->GetCollisionResponseToChannel(MothCollision::GroundTrace) == ECR_Block
				&& Body->IsQueryCollisionEnabled());
		}
	}
	TestEqual(TEXT("Five positive/control stations retain fourteen ordinary devices"), Devices, 14);
	TestEqual(TEXT("Sources and movable emitter targets"), Emitters, 9);
	TestEqual(TEXT("Direct bomb and two blast layouts"), Bombs, 3);
	TestEqual(TEXT("Two column layouts"), Launchers, 2);
	TestTrue(TEXT("Saved map contains real tagged floors and source shelves"), Grounds >= 6);
	TestTrue(TEXT("Saved room selects the existing player game mode"), Range->GetWorldSettings()->DefaultGameMode
		&& Range->GetWorldSettings()->DefaultGameMode->GetPathName() == TEXT("/Game/MothEffect/Framework/BP_MothGameMode.BP_MothGameMode_C"));
	return true;
}
#endif
