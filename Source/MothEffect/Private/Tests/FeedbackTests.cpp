#if WITH_DEV_AUTOMATION_TESTS
#include "Feedback/MothFeedbackSubsystem.h"
#include "Feedback/MothFeedbackVisual.h"
#include "Feedback/MothDeviceFeedbackComponent.h"
#include "Combat/RuleProjectile.h"
#include "Components/HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Tests/CombatTestWorld.h"
#include "Tutorial/MothTutorialTarget.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFeedbackAssetsTest,"MothEffect.Feedback.DefaultAssetsLoad",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothFeedbackAssetsTest::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	const auto* Profile=F.World->GetSubsystem<UMothFeedbackSubsystem>()->GetProfile();
	if (!TestNotNull(TEXT("Default saved feedback profile"),Profile)) return false;
	TestTrue(TEXT("Loads saved profile rather than fallback"),Profile->GetPathName().StartsWith(TEXT("/Game/MothEffect/Feedback/DA_DefaultFeedback")));
	TestEqual(TEXT("Twelve editable cue categories"),Profile->Styles.Num(),12);
	TestNotNull(TEXT("Shipping material loads"),Profile->Material.LoadSynchronous());
	TestNotNull(TEXT("Spatial attenuation loads"),Profile->Attenuation.LoadSynchronous());
	TSet<USoundBase*> Sounds;
	for (const auto& Entry:Profile->Styles) if (!Entry.Value.Sound.IsNull())
	{
		auto* Sound=Entry.Value.Sound.LoadSynchronous(); TestNotNull(TEXT("Configured sound loads"),Sound);
		if (Sound) { TestTrue(TEXT("Short non-looping feedback audio"),Sound->GetDuration()>0&&Sound->GetDuration()<.5f&&!Sound->IsLooping()); Sounds.Add(Sound); }
	}
	TestEqual(TEXT("Eight distinct authored sounds"),Sounds.Num(),8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFeedbackCommitTest,"MothEffect.Feedback.CommittedEventsAndLockedArrow",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothFeedbackCommitTest::RunTest(const FString&)
{
	FMothCombatTestWorld F;
	auto* Feedback=F.World->GetSubsystem<UMothFeedbackSubsystem>();
	auto* Device=F.Spawn<ADeviceBase>(FVector(0,0,300));
	Device->ActivationDelaySeconds=0.0f; // This case isolates committed effect feedback and geometry.
	FHitContext Hit; Hit.HitId=FGuid::NewGuid(); Hit.ShotDirection=FVector(0,2,1); Hit.ImpactPoint=Device->GetActorLocation();
	const FVector Heading=Hit.ShotDirection.GetSafeNormal2D();
	TestTrue(TEXT("Real device activation commits"),Device->TryActivate(Hit));
	TestEqual(TEXT("One activation cue"),Feedback->GetCueCount(EMothFeedbackCue::Emitter),uint64(1));
	TestFalse(TEXT("Same hit and subsequent hits cannot re-activate"),Device->TryActivate(Hit));
	TestEqual(TEXT("Rejected activation emits no extra cue"),Feedback->GetCueCount(EMothFeedbackCue::Emitter),uint64(1));
	Device->SetActorLocationAndRotation(FVector(200,80,450),FRotator(45,90,110),false,nullptr,ETeleportType::TeleportPhysics);
	Device->DefaultFeedback->RefreshPresentation();
	TArray<UStaticMeshComponent*> Meshes; Device->GetComponents(Meshes);
	int32 MarkerCount=0; UStaticMeshComponent* Shaft=nullptr;
	for (auto* Mesh:Meshes) if (Mesh->HasAnyFlags(RF_Transient))
	{
		++MarkerCount;
		TestTrue(TEXT("Markers never participate in collision"),Mesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!Mesh->GetGenerateOverlapEvents()&&!Mesh->CanEverAffectNavigation());
		if (Mesh->IsVisible()&&Mesh->GetComponentScale().X>.9f) Shaft=Mesh;
	}
	TestEqual(TEXT("Seven runtime pieces with no BP edits"),MarkerCount,7);
	if (TestNotNull(TEXT("Runtime directional shaft visible"),Shaft))
	{
		TestTrue(TEXT("Arrow rotation is locked to the horizontal shot projection, independent of body roll"),Shaft->GetForwardVector().Equals(Heading,.001));
		const FVector Expected=Device->GetActorLocation()+Heading*(32+Feedback->GetProfile()->DirectionLengthCm*.5f);
		TestTrue(TEXT("Arrow follows translation"),Shaft->GetComponentLocation().Equals(Expected,.01));
		const FVector Tip=Device->GetActorLocation()+Heading*(32+Feedback->GetProfile()->DirectionLengthCm+1);
		for (auto* Mesh:Meshes) if (Mesh->HasAnyFlags(RF_Transient)&&Mesh->IsVisible()&&FMath::IsNearlyEqual(Mesh->GetComponentScale().X,.22))
		{
			const FVector Front=Mesh->GetComponentLocation()+Mesh->GetForwardVector()*11;
			const FVector Back=Mesh->GetComponentLocation()-Mesh->GetForwardVector()*11;
			TestTrue(TEXT("Both arrowhead front endpoints converge at the forward tip"),FVector::Dist(Front,Tip)<FVector::Dist(Back,Tip)&&FVector::Dist(Front,Tip)<1);
		}
	}
	Device->FinishActivation();
	TestEqual(TEXT("Spent cue emitted once"),Feedback->GetCueCount(EMothFeedbackCue::Spent),uint64(1));
	Device->FinishActivation(); TestEqual(TEXT("Repeated finish adds no cue"),Feedback->GetCueCount(EMothFeedbackCue::Spent),uint64(1));
	Feedback->Emit(EMothFeedbackCue::Launcher,FVector(0,0,300),FVector::UpVector);
	for (TActorIterator<AMothFeedbackVisual> It(F.World); It; ++It)
		if (!It->IsActorBeingDestroyed()&&It->GetCueKind()==EMothFeedbackCue::Launcher)
			TestTrue(TEXT("D01 transient streaks use world up"),It->GetActorUpVector().Equals(FVector::UpVector));
	UClass* LauncherClass=LoadClass<ADeviceBase>(nullptr,TEXT("/Game/MothEffect/Devices/BP_DeviceLauncher.BP_DeviceLauncher_C"));
	if (TestNotNull(TEXT("Saved D01 class"),LauncherClass))
	{
		FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Launcher=F.World->SpawnActor<ADeviceBase>(LauncherClass,FVector(500,0,300),FRotator::ZeroRotator,Params);
		Launcher->GetComponents(Meshes);
		for (auto* Mesh:Meshes) if (Mesh->HasAnyFlags(RF_Transient)&&Mesh->IsVisible()&&FMath::IsNearlyEqual(Mesh->GetComponentScale().Y,.22))
			TestTrue(TEXT("D01 glyph long axis points up in world space"),Mesh->GetRightVector().Equals(FVector::UpVector,.001));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFeedbackHitTest,"MothEffect.Feedback.DamageAndProjectileAreSingleUse",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothFeedbackHitTest::RunTest(const FString&)
{
	FMothCombatTestWorld F; auto* Feedback=F.World->GetSubsystem<UMothFeedbackSubsystem>();
	auto* Target=F.Spawn<AMothTutorialTarget>(FVector(300,0,300));
	FHitContext Hit; Hit.HitId=FGuid::NewGuid(); Hit.Damage=20; Hit.ImpactPoint=Target->GetActorLocation(); Hit.ShotDirection=FVector::ForwardVector;
	TestTrue(TEXT("Accepted damage emits feedback"),Target->GetHealthComponent()->ApplyHit(Hit));
	TestFalse(TEXT("Repeated damage is rejected"),Target->GetHealthComponent()->ApplyHit(Hit));
	TestEqual(TEXT("Damage cue occurs only once"),Feedback->GetCueCount(EMothFeedbackCue::Hit),uint64(1));
	TestEqual(TEXT("Unattributed damage cannot confirm a local hit"),Feedback->GetHitMarkerAlpha(),0.f);
	auto* Projectile=F.Spawn<ARuleProjectile>(FVector(0,0,300));
	FHitResult Impact(Target,nullptr,Target->GetActorLocation(),FVector::BackwardVector); Impact.bBlockingHit=true;
	TestTrue(TEXT("Projectile consumes its first real blocking hit"),Projectile->ProcessBlockingHit(Impact));
	TestFalse(TEXT("Repeated projectile impact rejected"),Projectile->ProcessBlockingHit(Impact));
	TestEqual(TEXT("One projectile impact visual"),Feedback->GetCueCount(EMothFeedbackCue::Impact),uint64(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFeedbackCleanupTest,"MothEffect.Feedback.BoundedPauseAndSessionCleanup",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothFeedbackCleanupTest::RunTest(const FString&)
{
	FMothCombatTestWorld F; auto* Feedback=F.World->GetSubsystem<UMothFeedbackSubsystem>();
	auto* Session=F.World->GetSubsystem<UDeviceLifecycleSubsystem>();
	auto* Device=F.Spawn<ADeviceBase>(FVector(0,0,300));
	for (int32 I=0; I<40; ++I) Feedback->Emit(EMothFeedbackCue::Impact,FVector(I*10,0,300));
	TestEqual(TEXT("Visual actor budget holds under a burst"),Feedback->GetLiveVisualCount(),Feedback->GetProfile()->MaxVisualCues);
	for (TActorIterator<AMothFeedbackVisual> It(F.World); It; ++It)
	{
		TArray<UStaticMeshComponent*> Meshes; It->GetComponents(Meshes);
		for (auto* Mesh:Meshes) TestTrue(TEXT("Visual never obstructs a ray or physics"),Mesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!Mesh->CanEverAffectNavigation());
	}
	// Expired items must not reorder the remaining queue and evict a newer cue first.
	Feedback->ClearFeedback();
	{
		TGuardValue<int32> Budget(Feedback->GetProfile()->MaxVisualCues,3);
		TGuardValue<float> Duration(Feedback->GetProfile()->Styles[EMothFeedbackCue::Impact].Duration,.02f);
		Feedback->Emit(EMothFeedbackCue::Impact,FVector(0,0,500));
		Feedback->GetProfile()->Styles[EMothFeedbackCue::Impact].Duration=.5f;
		Feedback->Emit(EMothFeedbackCue::Impact,FVector(100,0,500)); Feedback->Emit(EMothFeedbackCue::Impact,FVector(200,0,500));
		AMothFeedbackVisual* Older=nullptr; AMothFeedbackVisual* Newer=nullptr;
		for (TActorIterator<AMothFeedbackVisual> It(F.World); It; ++It) if (!It->IsActorBeingDestroyed())
		{
			if (It->GetActorLocation().Equals(FVector(100,0,500))) Older=*It;
			if (It->GetActorLocation().Equals(FVector(200,0,500))) Newer=*It;
		}
		F.World->Tick(LEVELTICK_All,.03f); Feedback->Tick(0);
		Feedback->Emit(EMothFeedbackCue::Impact,FVector(300,0,500)); Feedback->Emit(EMothFeedbackCue::Impact,FVector(400,0,500));
		TestTrue(TEXT("Budget evicts oldest surviving cue after an earlier cue expires"),Older&&Older->IsActorBeingDestroyed());
		TestTrue(TEXT("A newer cue survives oldest-first eviction"),Newer&&!Newer->IsActorBeingDestroyed());
	}
	auto* Pauser=F.Spawn<APlayerState>(); F.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
	Feedback->Emit(EMothFeedbackCue::Hit,FVector(0,0,300),FVector::ForwardVector,true);
	TestEqual(TEXT("Paused emission is rejected"),Feedback->GetCueCount(EMothFeedbackCue::Hit),uint64(0));
	F.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
	Feedback->Emit(EMothFeedbackCue::Hit,FVector(0,0,300),FVector::ForwardVector,true);
	TestEqual(TEXT("Committed local hit shows a marker"),Feedback->GetHitMarkerAlpha(),1.f);
	Session->SetGameplayEnabled(false);
	TestEqual(TEXT("Result gate immediately destroys visuals"),Feedback->GetLiveVisualCount(),0);
	TestEqual(TEXT("Result gate immediately stops voices"),Feedback->GetLiveAudioCount(),0);
	TestEqual(TEXT("Result gate clears screen feedback"),Feedback->GetHitMarkerAlpha(),0.f);
	TArray<UStaticMeshComponent*> Meshes; Device->GetComponents(Meshes);
	for (auto* Mesh:Meshes) if (Mesh->HasAnyFlags(RF_Transient)) TestFalse(TEXT("Result gate hides persistent markers synchronously"),Mesh->IsVisible());
	Feedback->Emit(EMothFeedbackCue::Hit,FVector(0,0,300),FVector::ForwardVector,true);
	TestEqual(TEXT("Closed session refuses new cue"),Feedback->GetCueCount(EMothFeedbackCue::Hit),uint64(1));
	Session->SetGameplayEnabled(true); Feedback->Emit(EMothFeedbackCue::Shot,FVector(0,0,300)); Session->ResetManagedObjects();
	TestEqual(TEXT("Retry clears transient presentation"),Feedback->GetLiveVisualCount(),0);
	TestTrue(TEXT("Retry clears managed device and marker owner"),Device->IsActorBeingDestroyed());
	return true;
}

namespace MothFeedbackExpiryTest
{
	class FTimeline final : public IAutomationLatentCommand
	{
	public:
		explicit FTimeline(FAutomationTestBase* Test):T(Test) {}
		bool Update() override
		{
			if (!Fixture)
			{
				Fixture=MakeUnique<FMothCombatTestWorld>(); auto* Session=Fixture->World->GetSubsystem<UDeviceLifecycleSubsystem>();
				FindFProperty<FFloatProperty>(Session->GetClass(),TEXT("DormantIdleLifetimeSeconds"))->SetPropertyValue_InContainer(Session,.4f);
				FindFProperty<FFloatProperty>(Session->GetClass(),TEXT("DormantExpiryWarningSeconds"))->SetPropertyValue_InContainer(Session,.2f);
				Player=Fixture->Spawn<AMothPlayerAutomationCharacter>(FVector(0,0,300));
				Device=Fixture->Spawn<ADeviceBase>(FVector(120,0,300));
				Device->GetPhysicsBody()->SetSimulatePhysics(false);
				return false;
			}
			UWorld* W=Fixture->World; if (W->GetTimerManager().HasBeenTickedThisFrame()) return false;
			W->Tick(LEVELTICK_All,.05f); auto* F=W->GetSubsystem<UMothFeedbackSubsystem>();
			if (++Frames>60) { T->AddError(TEXT("Expiry feedback timeline timed out")); return true; }
			if (Stage==0&&Device->IsExpiryWarningActive())
			{
				T->TestEqual(TEXT("Actual idle warning emits once"),F->GetCueCount(EMothFeedbackCue::Expiry),uint64(1));
				T->TestTrue(TEXT("Warning device remains pickable"),Player->TryPickupDevice(Device));
				HeldRemaining=Device->GetRemainingIdleSeconds(); Stage=1; HeldAt=W->GetTimeSeconds();
				TArray<UStaticMeshComponent*> Meshes; Device->GetComponents(Meshes);
				for (auto* Mesh:Meshes) if (Mesh->HasAnyFlags(RF_Transient)) T->TestFalse(TEXT("Held device hides default world glyph"),Mesh->IsVisible());
			}
			else if (Stage==1&&W->GetTimeSeconds()-HeldAt>.6)
			{
				T->TestEqual(TEXT("Held pauses the remaining idle budget"),Device->GetRemainingIdleSeconds(),HeldRemaining);
				T->TestEqual(TEXT("Held emits no repeat warning"),F->GetCueCount(EMothFeedbackCue::Expiry),uint64(1));
				T->TestTrue(TEXT("Drop resumes the actual dormant clock"),Player->TryReleaseHeldDevice(false)); Stage=2;
			}
			else if (Stage==2&&Device->GetDeviceState()==EDeviceState::Spent)
			{
				T->TestEqual(TEXT("Re-entering the warning band emits one new warning"),F->GetCueCount(EMothFeedbackCue::Expiry),uint64(2));
				T->TestEqual(TEXT("Real expiry emits Spent once"),F->GetCueCount(EMothFeedbackCue::Spent),uint64(1)); Stage=3;
			}
			else if (Stage==3&&Device->IsActorBeingDestroyed())
			{
				T->TestEqual(TEXT("Expiry leaves no permanent managed marker owner"),W->GetSubsystem<UDeviceLifecycleSubsystem>()->GetDeviceCount(),0);
				return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* T; TUniquePtr<FMothCombatTestWorld> Fixture;
		AMothPlayerAutomationCharacter* Player=nullptr; ADeviceBase* Device=nullptr;
		int32 Stage=0,Frames=0; float HeldRemaining=0; double HeldAt=0;
	};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFeedbackExpiryTest,"MothEffect.Feedback.ExpiryHeldDropAndSpent",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothFeedbackExpiryTest::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothFeedbackExpiryTest::FTimeline(this)); return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFeedbackReentryTest,"MothEffect.Feedback.SpawnObserverCannotResurrectClearedPresentation",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothFeedbackReentryTest::RunTest(const FString&)
{
	for (int32 Case=0; Case<3; ++Case)
	{
		FMothCombatTestWorld F; auto* Feedback=F.World->GetSubsystem<UMothFeedbackSubsystem>();
		auto* Session=F.World->GetSubsystem<UDeviceLifecycleSubsystem>();
		const FDelegateHandle Handle=F.World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([Feedback,Session,Case](AActor* Actor)
		{
			if (!Cast<AMothFeedbackVisual>(Actor)) return;
			if (Case==0) Session->SetGameplayEnabled(false);
			else if (Case==1) Session->ResetManagedObjects();
			else Feedback->ClearFeedback();
		}));
		Feedback->Emit(EMothFeedbackCue::Hit,FVector(0,0,300),FVector::ForwardVector,true);
		F.World->RemoveOnActorSpawnedHandler(Handle);
		TestEqual(TEXT("Spawn-triggered close/reset/clear leaves no tracked cue"),Feedback->GetLiveVisualCount(),0);
		TestEqual(TEXT("Spawn-triggered close/reset/clear leaves no voice"),Feedback->GetLiveAudioCount(),0);
		TestEqual(TEXT("Spawn-triggered close/reset/clear cannot restore screen alpha"),Feedback->GetHitMarkerAlpha(),0.f);
		for (TActorIterator<AMothFeedbackVisual> It(F.World); It; ++It)
			TestTrue(TEXT("Newborn untracked cue is destroyed too"),It->IsActorBeingDestroyed());
	}
	return true;
}
#endif
