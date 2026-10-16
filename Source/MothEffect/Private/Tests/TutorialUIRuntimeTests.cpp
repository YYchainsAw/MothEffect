#if WITH_DEV_AUTOMATION_TESTS
#include "UI/MothHUDWidget.h"
#include "Framework/MothEffectPlayerController.h"
#include "Framework/MothWaveDirector.h"
#include "Framework/MothGameInstance.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "Tutorial/MothTutorialConfig.h"
#include "Tutorial/MothTutorialTarget.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/HealthComponent.h"
#include "Components/Button.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Weapons/Rifle.h"
#include "Tests/PlayerAutomationCharacter.h"

namespace MothTutorialRuntimeTest
{
	UWorld* World()
	{
		for (const auto& Context:GEngine->GetWorldContexts()) if (Context.WorldType==EWorldType::Game) return Context.World();
		return nullptr;
	}
	AMothWaveDirector* Director(UWorld* W)
	{
		for (TActorIterator<AMothWaveDirector> It(W);It;++It) if (!It->IsActorBeingDestroyed()) return *It; return nullptr;
	}
	void Capture(const TCHAR* Name)
	{
		if (FParse::Param(FCommandLine::Get(),TEXT("MothUIScreens")))
		{
			FString Directory=TEXT("Automation/T18/Screens");
			FParse::Value(FCommandLine::Get(),TEXT("MothUIScreenDir="),Directory);
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/Directory/Name,true,false);
		}
	}
	FHitContext Shot(AMothEffectCharacter* P,ADeviceBase* D,FVector Direction=FVector::ForwardVector)
	{
		FHitContext H;H.HitId=FGuid::NewGuid();H.SourceActor=P->GetRifle();H.InstigatorPawn=P;
		H.ShotDirection=Direction;H.ImpactPoint=D->GetActorLocation();H.Damage=20; return H;
	}
	class FTutorialFlow final:public IAutomationLatentCommand
	{
	public:
		explicit FTutorialFlow(FAutomationTestBase* InTest):T(InTest) {}
		bool Update() override
		{
			UWorld* W=World(); if (!W) { T->AddError(TEXT("Run UI acceptance in actual -game L_Arena_01")); return true; }
			if (!Started) Started=FPlatformTime::Seconds();
			if (FPlatformTime::Seconds()-Started>45) { T->AddError(FString::Printf(TEXT("Tutorial flow timeout stage=%d step=%d"),Stage,static_cast<int32>(W->GetSubsystem<UMothTutorialSubsystem>()->GetStep()))); Cleanup(W); return true; }
			auto* PC=Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(W,0));
			auto* UI=PC?PC->GetGameplayUI():nullptr;
			auto* D=Director(W);
			auto* S=W->GetSubsystem<UMothTutorialSubsystem>();
			if (!PC||!UI||!D) return false;
			if (Stage==0)
			{
				if (D->GetPhase()!=EMothFlowPhase::Ready) { PC->ReturnToTitle(); Stage=-1; Previous=W; return false; }
				T->TestEqual(TEXT("Initial getter health is 100 before any hit"),UI->GetHUDState().Health,100.f);
				T->TestEqual(TEXT("Initial getter magazine is 24 before any ammo event"),UI->GetHUDState().Ammo,24);
				T->TestTrue(TEXT("Title owns cursor and blocks pawn input"),UI->GetHUDState().Page==EMothUIPage::Title&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored()&&PC->IsLookInputIgnored());
				T->TestTrue(TEXT("Title primary button has keyboard focus"),UI->GetMenuFocusTarget()->HasUserFocus(PC));
				Capture(TEXT("01_Title.png")); Wait=FPlatformTime::Seconds()+.6; Stage=1; return false;
			}
			if (Stage==-1) { if (W==Previous.Get()) return false; Stage=0; return false; }
			if (FPlatformTime::Seconds()<Wait) return false;
			if (Stage==1)
			{
				auto* Button=Cast<UButton>(UI->GetWidgetFromName(TEXT("SecondaryButton")));
				if (!T->TestNotNull(TEXT("Saved Designer teaching button"),Button)) return true;
				Button->OnClicked.Broadcast(); UI->RefreshState(); PC->SyncUIInput();
				T->TestTrue(TEXT("Start button enters real tutorial with gameplay input"),S->GetStep()==EMothTutorialStep::BombAndLift&&!PC->bShowMouseCursor&&!PC->IsMoveInputIgnored());
				Capture(TEXT("02_Tutorial.png")); Wait=FPlatformTime::Seconds()+.6; Stage=2; return false;
			}
			if (Stage==2)
			{
				PC->ToggleMenuPause();
				T->TestTrue(TEXT("Escape pause commits menu/cursor/ignored input"),W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Pause&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored());
				T->TestTrue(TEXT("Pause primary button owns keyboard focus"),UI->GetMenuFocusTarget()->HasUserFocus(PC));
				PauseTime=W->GetTimeSeconds(); Capture(TEXT("03_Pause.png")); Wait=FPlatformTime::Seconds()+.6; Stage=3; return false;
			}
			if (Stage==3)
			{
				T->TestEqual(TEXT("Paused world clock is frozen while Slate renders menu"),W->GetTimeSeconds(),PauseTime);
				Cast<UButton>(UI->GetWidgetFromName(TEXT("PrimaryButton")))->OnClicked.Broadcast();
				T->TestTrue(TEXT("Bound resume button restores tutorial and gameplay input"),!W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Tutorial&&!PC->bShowMouseCursor);
				// Replace only the controlled pawn for deterministic physical-button/focus input.
				Formal=Cast<AMothEffectCharacter>(PC->GetPawn());
				S->EndTutorial();
				FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Fixture=W->SpawnActor<AMothPlayerAutomationCharacter>(FVector(-2200,-1800,100),FRotator::ZeroRotator,Params);
				if (!T->TestNotNull(TEXT("Deterministic input fixture"),Fixture.Get())) return true;
				PC->Possess(Fixture.Get()); D->SetPlayer(Fixture.Get()); Fixture->SetInputFocused(true);
				auto* Config=DuplicateObject<UMothTutorialConfig>(UI->GetTutorialConfig(),GetTransientPackage());
				Config->BombLocation+=FVector(0,40,0); Config->BlastTargetLocation+=FVector(0,40,0);
				Config->BombAndLiftInstruction=FText::FromString(TEXT("Editable teaching instruction"));
				Config->ResetFeedback=FText::FromString(TEXT("Editable reset feedback"));
				const FVector ExpectedBomb=Config->BombLocation;
				T->TestTrue(TEXT("Tutorial accepts an edited teaching config"),S->StartTutorial(Fixture.Get(),D,Config));
				T->TestEqual(TEXT("Edited instruction is used"),S->GetInstruction().ToString(),Config->BombAndLiftInstruction.ToString());
				Config->BombLocation+=FVector(0,500,0); Config->BombAndLiftInstruction=FText::FromString(TEXT("Changed source asset"));
				auto* PreviousBomb=S->GetPracticeDevice(EDeviceKind::Bomb);
				Cast<UButton>(UI->GetWidgetFromName(TEXT("RetryButton")))->OnClicked.Broadcast();
				auto* ResetBomb=S->GetPracticeDevice(EDeviceKind::Bomb);
				T->TestTrue(TEXT("Bound retry button replaces owned practice objects"),ResetBomb&&ResetBomb!=PreviousBomb);
				if (ResetBomb) T->TestTrue(TEXT("Retry preserves the per-run position snapshot"),ResetBomb->GetActorLocation().Equals(ExpectedBomb,1.f));
				T->TestEqual(TEXT("Source edits cannot change the current instruction"),S->GetInstruction().ToString(),FString(TEXT("Editable teaching instruction")));
				T->TestEqual(TEXT("Bound retry reads editable feedback"),S->GetFeedback().ToString(),FString(TEXT("Editable reset feedback")));
				if (auto* Lift=S->GetPracticeDevice(EDeviceKind::Launcher)) Lift->TryActivate(Shot(Fixture.Get(),Lift));
				Wait=FPlatformTime::Seconds()+5; Stage=33; return false;
			}
			if (Stage==33)
			{
				T->TestEqual(TEXT("Completing lift alone cannot skip the blast goal"),S->GetStep(),EMothTutorialStep::BombAndLift);
				if (auto* Bomb=S->GetPracticeDevice(EDeviceKind::Bomb)) Bomb->TryActivate(Shot(Fixture.Get(),Bomb));
				Stage=4; return false;
			}
			if (Stage==4&&S->GetStep()==EMothTutorialStep::AirborneEmitter)
			{
				T->AddInfo(TEXT("Real blast kills character practice target; real D01 physics raises its box before step advances"));
				auto* E=S->GetPracticeDevice(EDeviceKind::Emitter); if (!T->TestNotNull(TEXT("Owned airborne practice emitter"),E)) return true;
				E->GetPhysicsBody()->SetSimulatePhysics(false); E->SetActorLocation(Fixture->GetActorLocation()+FVector(120,0,0));
				T->TestTrue(TEXT("Real native pickup accepts owned prop"),Fixture->TryPickupDevice(E));
				T->TestTrue(TEXT("E drop uses the real release path"),Fixture->TryReleaseHeldDevice(false));
				E->TryActivate(Shot(Fixture.Get(),E));
				FailedEmitter=E; Wait=FPlatformTime::Seconds()+.3; Stage=5; return false;
			}
			if (Stage==5)
			{
				T->TestTrue(TEXT("Dropped/ground activation cannot advance and creates a fresh retry prop"),S->GetStep()==EMothTutorialStep::AirborneEmitter&&S->GetPracticeDevice(EDeviceKind::Emitter)!=FailedEmitter.Get());
				auto* E=S->GetPracticeDevice(EDeviceKind::Emitter); if (!E) { Cleanup(W); return true; }
				E->GetPhysicsBody()->SetSimulatePhysics(false); E->SetActorLocation(Fixture->GetActorLocation()+FVector(120,0,0));
				T->TestTrue(TEXT("Retry prop can be physically held"),Fixture->TryPickupDevice(E));
				Fixture->SetPrimaryDown(false); Fixture->DoPrimaryEnd();
				Fixture->SetPrimaryDown(true); Fixture->DoPrimaryStart();
				T->TestFalse(TEXT("Throw commits release before tutorial evidence"),Fixture->IsCarryingDevice());
				GoodEmitter=E; ThrownAt=W->GetTimeSeconds(); Wait=0; Stage=55; return false;
			}
			if (Stage==55)
			{
				auto* E=S->GetPracticeDevice(EDeviceKind::Emitter);
				// Observe the real landing/refill, rather than assuming a fixed wall-clock flight time.
				if (E == GoodEmitter.Get() && W->GetTimeSeconds()-ThrownAt < 6) return false;
				T->TestTrue(TEXT("A missed throw landing refills promptly without advancing"),S->GetStep()==EMothTutorialStep::AirborneEmitter&&E&&E!=GoodEmitter.Get()&&!S->GetFeedback().IsEmpty());
				T->AddInfo(FString::Printf(TEXT("Missed-throw real refill observed after %.3fs gameplay time"),W->GetTimeSeconds()-ThrownAt));
				if (!E) { Cleanup(W);return true; }
				E->GetPhysicsBody()->SetSimulatePhysics(false); E->SetActorLocation(Fixture->GetActorLocation()+FVector(120,0,0));
				T->TestTrue(TEXT("Refill after a missed throw can be picked up"),Fixture->TryPickupDevice(E));
				Fixture->SetPrimaryDown(false); Fixture->DoPrimaryEnd();GoodEmitter=E;
				Capture(TEXT("07_HeldAndStep2.png"));Wait=FPlatformTime::Seconds()+.3;Stage=555;return false;
			}
			if (Stage==555)
			{
				T->TestTrue(TEXT("Committed held state and device kind update through action signal"),UI->GetHUDState().bHeld&&UI->GetHUDState().HeldKind==EDeviceKind::Emitter);
				Fixture->SetPrimaryDown(true);Fixture->DoPrimaryStart();Wait=FPlatformTime::Seconds()+.45;Stage=6;return false;
			}
			if (Stage==6)
			{
				const int32 Ammo=Fixture->GetRifle()->GetAmmoInMagazine();
				Fixture->DoPrimaryStart(); T->TestEqual(TEXT("Same held primary press cannot spend ammo after throwing"),Fixture->GetRifle()->GetAmmoInMagazine(),Ammo);
				Fixture->SetPrimaryDown(false); Fixture->DoPrimaryEnd();
				if (GoodEmitter.IsValid()) GoodEmitter->TryActivate(Shot(Fixture.Get(),GoodEmitter.Get(),FVector(1,0,.15)));
				Stage=7; return false;
			}
			if (Stage==7&&S->GetStep()==EMothTutorialStep::IndirectChain)
			{
				T->AddInfo(TEXT("Actual held-to-thrown event plus above-ground W01 evidence advances airborne step"));
				Capture(TEXT("08_IndirectStep3.png"));
				Fixture->SetPrimaryDown(false);Fixture->DoPrimaryEnd();Fixture->SetPrimaryDown(true);Fixture->DoPrimaryStart();
				Fixture->SetPrimaryDown(false);Fixture->DoPrimaryEnd();
				T->TestEqual(TEXT("Actual rifle shot updates the rebound HUD through its ammo signal"),UI->GetHUDState().Ammo,23);
				Fixture->DoReloadStart();
				T->TestTrue(TEXT("Real reload immediately updates HUD state"),Fixture->GetRifle()->IsReloading()&&UI->GetHUDState().bReloading);
				PC->ToggleMenuPause();ReloadProgress=Fixture->GetRifle()->GetReloadProgress();
				Wait=FPlatformTime::Seconds()+.8;Stage=70;return false;
			}
			if (Stage==70)
			{
				T->TestTrue(TEXT("The actual UI pause keeps a legal reload pending"),W->IsPaused()&&Fixture->GetRifle()->IsReloading());
				T->TestEqual(TEXT("Reload progress is frozen by the actual pause menu"),Fixture->GetRifle()->GetReloadProgress(),ReloadProgress);
				PC->ResumeGame();T->TestTrue(TEXT("UI resume preserves that same reload task"),Fixture->GetRifle()->IsReloading());
				Wait=FPlatformTime::Seconds()+1.4;Stage=71;return false;
			}
			if (Stage==71)
			{
				T->TestTrue(TEXT("The preserved reload completes and the HUD shows its new full magazine"),!Fixture->GetRifle()->IsReloading()&&UI->GetHUDState().Ammo==24&&!UI->GetHUDState().bReloading);
				auto* E=S->GetPracticeDevice(EDeviceKind::Emitter); auto* Bomb=S->GetPracticeDevice(EDeviceKind::Bomb);
				if (!E||!Bomb) { Cleanup(W);return true; }
				E->GetPhysicsBody()->SetSimulatePhysics(false); Bomb->GetPhysicsBody()->SetSimulatePhysics(false);
				T->TestTrue(TEXT("Launcher activation locks real +X chain direction"),E->TryActivate(Shot(Fixture.Get(),E)));
				Stage=8; return false;
			}
			if (Stage==8&&S->GetStep()==EMothTutorialStep::Completed)
			{
				UI->RefreshState();PC->SyncUIInput();
				T->TestEqual(TEXT("Completion clears every tutorial-owned actor"),S->GetOwnedActorCount(),0);
				T->TestTrue(TEXT("Natural D03 P01 -> D02 blast yields completion menu"),UI->GetHUDState().Page==EMothUIPage::TutorialComplete&&PC->bShowMouseCursor);
				Capture(TEXT("04_TutorialComplete.png")); Wait=FPlatformTime::Seconds()+.6; Stage=9; return false;
			}
			if (Stage==9)
			{
				T->TestTrue(TEXT("Signal-driven HUD follows a replacement pawn's initial ammo"),UI->GetHUDState().Ammo==Fixture->GetRifle()->GetAmmoInMagazine());
				Cleanup(W);PC->ReturnToTitle(); Stage=10; Previous=W; return false;
			}
			if (Stage==10&&W!=Previous.Get())
			{
				T->TestTrue(TEXT("Fresh world returns title with clean initial data"),UI->GetHUDState().Page==EMothUIPage::Title&&UI->GetHUDState().Health==100&&UI->GetHUDState().Ammo==24&&!UI->GetHUDState().bHeld);
				return true;
			}
			return false;
		}
	private:
		void Cleanup(UWorld* W)
		{
			W->GetSubsystem<UMothTutorialSubsystem>()->EndTutorial();
			if (auto* PC=UGameplayStatics::GetPlayerController(W,0);PC&&Formal.IsValid()) PC->Possess(Formal.Get());
			if (auto* D=Director(W);D&&Formal.IsValid()) D->SetPlayer(Formal.Get());
			if (Fixture.IsValid()) Fixture->Destroy(); Fixture.Reset();
		}
		FAutomationTestBase* T; int32 Stage=0; double Started=0,Wait=0,PauseTime=0,ThrownAt=0;float ReloadProgress=0;
		TWeakObjectPtr<UWorld> Previous; TWeakObjectPtr<AMothEffectCharacter> Formal;
		TWeakObjectPtr<AMothPlayerAutomationCharacter> Fixture; TWeakObjectPtr<ADeviceBase> FailedEmitter,GoodEmitter;
	};

	class FMenuTravel final:public IAutomationLatentCommand
	{
	public:
		explicit FMenuTravel(FAutomationTestBase* InTest):T(InTest) {}
		bool Update() override
		{
			UWorld* W=World();if (!W) { T->AddError(TEXT("No actual game world"));return true; }
			if (!Started) Started=FPlatformTime::Seconds();
			if (FPlatformTime::Seconds()-Started>35) { T->AddError(TEXT("Menu travel timed out"));return true; }
			auto* PC=Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(W,0));
			auto* UI=PC?PC->GetGameplayUI():nullptr; auto* D=Director(W);
			if (!UI||!D) return false;
			if (FPlatformTime::Seconds()<Wait) return false;
			if (Stage==0)
			{
				if (D->GetPhase()!=EMothFlowPhase::Ready) { PC->ReturnToTitle();Previous=W;Stage=-1;return false; }
				auto* Button=Cast<UButton>(UI->GetWidgetFromName(TEXT("PrimaryButton")));
				if (!T->TestNotNull(TEXT("Saved Designer battle button"),Button)) return true;
				Button->OnClicked.Broadcast();Previous=W;Stage=1;return false;
			}
			if (Stage==-1) { if (Previous.Get()==W) return false;Stage=0;return false; }
			if (Stage==1&&W!=Previous.Get())
			{
				T->TestTrue(TEXT("Start battle travels to fresh Preparing world with gameplay input"),D->GetPhase()==EMothFlowPhase::Preparing&&UI->GetHUDState().Page==EMothUIPage::Gameplay&&!PC->bShowMouseCursor);
				T->TestTrue(TEXT("One-shot intention consumed exactly once"),!PC->GetGameInstance<UMothGameInstance>()->HasBattleAfterTravel());
				T->TestTrue(TEXT("Battle begins with correct health/ammo and no held tutorial prop"),UI->GetHUDState().Health==100&&UI->GetHUDState().Ammo==24&&!UI->GetHUDState().bHeld);
				T->TestEqual(TEXT("No tutorial-owned props survived travel"),W->GetSubsystem<UMothTutorialSubsystem>()->GetOwnedActorCount(),0);
				Capture(TEXT("05_BattleHUD.png"));Wait=FPlatformTime::Seconds()+.6;Stage=2;return false;
			}
			if (Stage==2)
			{
				auto* P=Cast<AMothEffectCharacter>(PC->GetPawn());
				FHitContext Hit;Hit.HitId=FGuid::NewGuid();Hit.Damage=20;Hit.ShotDirection=FVector::ForwardVector;
				P->GetHealthComponent()->ApplyHit(Hit);
				T->TestEqual(TEXT("Health event updates the live HUD before its next tick"),UI->GetHUDState().Health,80.f);
				Hit.HitId=FGuid::NewGuid();Hit.Damage=100;P->GetHealthComponent()->ApplyHit(Hit);
				UI->RefreshState();PC->SyncUIInput();
				T->TestTrue(TEXT("Death commits Defeat, clears pending births and returns menu input"),D->GetPhase()==EMothFlowPhase::Defeat&&D->GetPendingCount()==0&&UI->GetHUDState().Page==EMothUIPage::Defeat&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored());
				Capture(TEXT("06_Defeat.png"));Wait=FPlatformTime::Seconds()+.6;Stage=3;return false;
			}
			if (Stage==3) { PC->ReturnToTitle();Previous=W;Stage=4;return false; }
			if (Stage==4&&W!=Previous.Get())
			{
				T->TestTrue(TEXT("Result restart returns Title, not another automatic battle"),D->GetPhase()==EMothFlowPhase::Ready&&UI->GetHUDState().Page==EMothUIPage::Title&&PC->bShowMouseCursor);
				T->TestTrue(TEXT("Result restart resets health, ammo, waves and pending enemies"),UI->GetHUDState().Health==100&&UI->GetHUDState().Ammo==24&&UI->GetHUDState().Wave==0&&UI->GetHUDState().Pending==0&&UI->GetHUDState().Alive==0);
				PC->StartTeaching();
				PC->GetPawn()->SetActorLocation(FVector(3100,0,100));
				Wait=FPlatformTime::Seconds()+.2;Stage=5;return false;
			}
			if (Stage==5)
			{
				T->TestTrue(TEXT("Tutorial bounds failure uses the shared defeat cleanup"),D->GetPhase()==EMothFlowPhase::Defeat&&UI->GetHUDState().Page==EMothUIPage::Defeat);
				T->TestEqual(TEXT("Tutorial result clears practice targets as well as devices"),W->GetSubsystem<UMothTutorialSubsystem>()->GetOwnedActorCount(),0);
				PC->ReturnToTitle();Previous=W;Stage=6;return false;
			}
			if (Stage==6&&W!=Previous.Get())
			{
				T->TestTrue(TEXT("Tutorial failure can restart into a clean title"),D->GetPhase()==EMothFlowPhase::Ready&&UI->GetHUDState().Health==100);
				return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* T;int32 Stage=0;double Started=0,Wait=0;TWeakObjectPtr<UWorld> Previous;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothTutorialActionFlow,"MothEffect.UIRuntime.ActionDrivenTutorialAndPause",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FMothTutorialActionFlow::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothTutorialRuntimeTest::FTutorialFlow(this));return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothMenuTravel,"MothEffect.UIRuntime.StartBattleDefeatAndRestart",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FMothMenuTravel::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothTutorialRuntimeTest::FMenuTravel(this));return true; }
#endif
