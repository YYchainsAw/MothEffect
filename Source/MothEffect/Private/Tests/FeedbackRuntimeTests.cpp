#if WITH_DEV_AUTOMATION_TESTS
#include "Feedback/MothFeedbackSubsystem.h"
#include "Feedback/MothFeedbackVisual.h"
#include "Devices/DeviceBase.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SceneComponent.h"
#include "Framework/MothEffectPlayerController.h"
#include "Framework/MothWaveDirector.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "Tutorial/MothTutorialTarget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Tests/PlayerAutomationCharacter.h"
#include "UI/MothHUDWidget.h"
#include "UnrealClient.h"
#include "Weapons/Rifle.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"

namespace MothFeedbackRuntime
{
	class FFlow final : public IAutomationLatentCommand
	{
	public:
		explicit FFlow(FAutomationTestBase* Test):T(Test) {}
		bool Update() override
		{
			UWorld* W=nullptr; for (const auto& C:GEngine->GetWorldContexts()) if (C.WorldType==EWorldType::Game) W=C.World();
			if (!W) { T->AddError(TEXT("Run feedback flow in actual -game L_Arena_01")); return true; }
			if (!Started) Started=FPlatformTime::Seconds();
			if (FPlatformTime::Seconds()-Started>35) { T->AddError(FString::Printf(TEXT("Feedback runtime timeout stage=%d"),Stage)); UGameplayStatics::SetGamePaused(W,false); return true; }
			auto* PC=Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(W,0));
			auto* F=W->GetSubsystem<UMothFeedbackSubsystem>();
			auto* S=W->GetSubsystem<UMothTutorialSubsystem>();
			AMothWaveDirector* D=nullptr; for (TActorIterator<AMothWaveDirector> It(W);It;++It) D=*It;
			if (!PC||!PC->GetGameplayUI()||!D) return false;
			if (Stage==-1) { if (Previous.Get()==W) return false; Stage=0; }
			if (FPlatformTime::Seconds()<Wait) return false;
			if (W->GetTimeSeconds()<WaitWorld) return false;
			if (Stage==0)
			{
				if (D->GetPhase()!=EMothFlowPhase::Ready) { Previous=W; PC->ReturnToTitle(); Stage=-1; return false; }
				PC->StartTeaching(); S->EndTutorial();
				if (APawn* Original=PC->GetPawn()) { Original->SetActorHiddenInGame(true); Original->SetActorEnableCollision(false); }
				FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Player=W->SpawnActor<AMothPlayerAutomationCharacter>(FVector(-2200,-1800,100),FRotator(0,90,0),Params);
				if (!T->TestNotNull(TEXT("Actual input fixture"),Player.Get())) return true;
				PC->Possess(Player.Get()); PC->SetViewTarget(Player.Get()); PC->SetControlRotation(FRotator(0,90,0)); D->SetPlayer(Player.Get());
				Player->SetInputFocused(true); S->StartTutorial(Player.Get(),D,PC->GetGameplayUI()->GetTutorialConfig()); PC->SyncUIInput();
				Rifle=Player->GetRifle(); F->ClearFeedback(); Wait=FPlatformTime::Seconds()+.5; Stage=1; return false;
			}
			if (Stage==1)
			{
				FVector View,Dir; Player->GetRifleView(View,Dir);
				FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Target=W->SpawnActor<AMothTutorialTarget>(View+Dir*500,FRotator::ZeroRotator,Params);
				BeforeShot=F->GetCueCount(EMothFeedbackCue::Shot); BeforeHit=F->GetCueCount(EMothFeedbackCue::Hit);
				T->AddExpectedError(TEXT("could not play on"),EAutomationExpectedErrorFlags::Contains,1);
				Player->SetPrimaryDown(true); Player->DoPrimaryStart(); Player->SetPrimaryDown(false); Player->DoPrimaryEnd();
				T->TestEqual(TEXT("Actual W01 shot emits one muzzle cue"),F->GetCueCount(EMothFeedbackCue::Shot),BeforeShot+1);
				T->TestEqual(TEXT("Actual W01 hit emits one confirmed cue"),F->GetCueCount(EMothFeedbackCue::Hit),BeforeHit+1);
				T->TestTrue(TEXT("Confirmed hit reaches HUD without layout edits"),F->GetHitMarkerAlpha()>0);
				Rifle->TryStartFire(); Rifle->StopFire();
				T->TestEqual(TEXT("Refire interval still prevents an immediate second muzzle cue"),F->GetCueCount(EMothFeedbackCue::Shot),BeforeShot+1);
				Capture(TEXT("01_ConfirmedHit.png")); Wait=FPlatformTime::Seconds()+.25; WaitWorld=W->GetTimeSeconds()+.24; Stage=2; return false;
			}
			if (Stage==2)
			{
				Target->Destroy();
				AActor* Wall=W->SpawnActor<AActor>(); Blocker=Wall;
				auto* Box=NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box); Box->SetBoxExtent(FVector(15));
				Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				Box->SetCollisionResponseToAllChannels(ECR_Block); Box->RegisterComponent();
				FVector Muzzle; Rifle->GetMuzzleLocation(Muzzle); Box->SetWorldLocation(Muzzle);
				const uint64 Impacts=F->GetCueCount(EMothFeedbackCue::Impact); BeforeShot=F->GetCueCount(EMothFeedbackCue::Shot);
				Player->SetPrimaryDown(true); Player->DoPrimaryStart(); Player->SetPrimaryDown(false); Player->DoPrimaryEnd();
				T->TestEqual(TEXT("Blocked muzzle consumes a real shot and emits muzzle cue"),F->GetCueCount(EMothFeedbackCue::Shot),BeforeShot+1);
				T->TestEqual(TEXT("Blocked muzzle emits no fake downstream impact"),F->GetCueCount(EMothFeedbackCue::Impact),Impacts);
				T->TestEqual(TEXT("Blocked shot gives no confirmed hit marker"),F->GetHitMarkerAlpha(),0.f);
				Wall->Destroy();
				FHitContext H; H.HitId=FGuid::NewGuid(); H.Damage=10; H.ImpactPoint=Player->GetActorLocation(); H.ShotDirection=FVector::BackwardVector;
				Player->GetHealthComponent()->ApplyHit(H); T->TestTrue(TEXT("Local accepted damage lights screen edges"),F->GetDamageEdgeAlpha()>0);
				Capture(TEXT("02_DamageEdges.png")); Wait=FPlatformTime::Seconds()+.4; Stage=3; return false;
			}
			if (Stage==3)
			{
				auto* Lift=S->GetPracticeDevice(EDeviceKind::Launcher); if (!T->TestNotNull(TEXT("Saved D01"),Lift)) return true;
				Lift->TryActivate(Hit(Lift));
				T->TestTrue(TEXT("Saved launcher commits pending before launch feedback"),Lift->IsActivationPending());
				WaitWorld=W->GetTimeSeconds()+Lift->ActivationDelaySeconds+.01; Stage=31; return false;
			}
			if (Stage==31)
			{
				T->TestTrue(TEXT("Saved launcher publishes successful launch feedback"),F->GetCueCount(EMothFeedbackCue::Launcher)>0);
				Capture(TEXT("03_Launcher.png")); Wait=FPlatformTime::Seconds()+.6; Stage=4; return false;
			}
			if (Stage==4)
			{
				auto* Bomb=S->GetPracticeDevice(EDeviceKind::Bomb); if (!T->TestNotNull(TEXT("Saved D02"),Bomb)) return true;
				Bomb->TryActivate(Hit(Bomb)); T->TestTrue(TEXT("Saved bomb waits before its explosion ring"),Bomb->IsActivationPending());
				WaitWorld=W->GetTimeSeconds()+Bomb->ActivationDelaySeconds+.01; Stage=41; return false;
			}
			if (Stage==41)
			{
				T->TestTrue(TEXT("Saved delayed bomb publishes explosion ring"),F->GetCueCount(EMothFeedbackCue::Bomb)>0);
				Capture(TEXT("04_Bomb.png")); Wait=FPlatformTime::Seconds()+.5; Stage=5; return false;
			}
			if (Stage==5)
			{
				auto* Class=LoadClass<ADeviceBase>(nullptr,TEXT("/Game/MothEffect/Devices/BP_DeviceEmitter.BP_DeviceEmitter_C"));
				FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Emitter=W->SpawnActor<ADeviceBase>(Class,FVector(-2200,-1300,160),FRotator::ZeroRotator,Params);
				if (!T->TestNotNull(TEXT("Saved D03 inherits native feedback"),Emitter.Get())) return true;
				Emitter->TryActivate(Hit(Emitter.Get(),FVector(1,0,.25))); Capture(TEXT("05_Emitter.png"));
				WaitWorld=W->GetTimeSeconds()+Emitter->ActivationDelaySeconds+.12; Stage=6; return false;
			}
			if (Stage==6)
			{
				T->TestTrue(TEXT("Real D03 produces pulse feedback"),F->GetCueCount(EMothFeedbackCue::EmitterShot)>0);
				F->ClearFeedback(); F->Emit(EMothFeedbackCue::Expiry,Emitter->GetActorLocation(),FVector::ForwardVector,true);
				const bool AudioEnabled=!FParse::Param(FCommandLine::Get(),TEXT("nosound"));
				if (AudioEnabled) T->TestTrue(TEXT("Real audio device creates a playing feedback voice"),F->GetLiveAudioCount()>0);
				PausedClock=W->GetTimeSeconds(); PausedVisuals=F->GetLiveVisualCount(); PausedAudio=F->GetLiveAudioCount(); PausedAlpha=F->GetHitMarkerAlpha();
				PC->ToggleMenuPause(); Wait=FPlatformTime::Seconds()+.5; Stage=7; return false;
			}
			if (Stage==7)
			{
				T->TestEqual(TEXT("Actual paused game clock frozen"),W->GetTimeSeconds(),PausedClock);
				T->TestEqual(TEXT("Paused visual survives its real-time lifetime"),F->GetLiveVisualCount(),PausedVisuals);
				T->TestEqual(TEXT("Paused sound survives its real-time duration"),F->GetLiveAudioCount(),PausedAudio);
				T->TestEqual(TEXT("Paused marker does not fade"),F->GetHitMarkerAlpha(),PausedAlpha);
				PC->ResumeGame(); Wait=FPlatformTime::Seconds()+.6; WaitWorld=W->GetTimeSeconds()+.3; Stage=8; return false;
			}
			if (Stage==8)
			{
				T->TestEqual(TEXT("Resumed screen cue expires by world time"),F->GetHitMarkerAlpha(),0.f);
				S->EndTutorial(); W->GetSubsystem<UDeviceLifecycleSubsystem>()->SetGameplayEnabled(false);
				T->TestEqual(TEXT("Terminal cleanup leaves no visual"),F->GetLiveVisualCount(),0);
				T->TestEqual(TEXT("Terminal cleanup leaves no voice"),F->GetLiveAudioCount(),0);
				T->AddInfo(FString::Printf(TEXT("T19 feedback runtime: shot=%llu hit=%llu launcher=%llu bomb=%llu emitter=%llu pulse=%llu audioAtPause=%d"),
					F->GetCueCount(EMothFeedbackCue::Shot),F->GetCueCount(EMothFeedbackCue::Hit),F->GetCueCount(EMothFeedbackCue::Launcher),
					F->GetCueCount(EMothFeedbackCue::Bomb),F->GetCueCount(EMothFeedbackCue::Emitter),F->GetCueCount(EMothFeedbackCue::EmitterShot),PausedAudio));
				PC->ReturnToTitle(); return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* T; int32 Stage=0, PausedVisuals=0, PausedAudio=0;
		double Started=0,Wait=0,WaitWorld=0,PausedClock=0; float PausedAlpha=0; uint64 BeforeShot=0,BeforeHit=0;
		TWeakObjectPtr<UWorld> Previous;
		TWeakObjectPtr<AMothPlayerAutomationCharacter> Player;
		TWeakObjectPtr<ARifle> Rifle; TWeakObjectPtr<AMothTutorialTarget> Target;
		TWeakObjectPtr<ADeviceBase> Emitter; TWeakObjectPtr<AActor> Blocker;
		FHitContext Hit(ADeviceBase* Device,FVector Direction=FVector::ForwardVector) const
		{ FHitContext H; H.HitId=FGuid::NewGuid(); H.SourceActor=Rifle.Get(); H.InstigatorPawn=Player.Get(); H.ImpactPoint=Device->GetActorLocation(); H.ShotDirection=Direction; H.Damage=20; return H; }
		void Capture(const TCHAR* Name)
		{ if (FParse::Param(FCommandLine::Get(),TEXT("MothFeedbackScreens"))) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Automation/T19/Screens")/Name,true,false); }
	};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothFeedbackRuntimeTest,"MothEffect.FeedbackRuntime.ActualActionsPauseAndCleanup",
	EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FMothFeedbackRuntimeTest::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothFeedbackRuntime::FFlow(this)); return true; }
#endif
