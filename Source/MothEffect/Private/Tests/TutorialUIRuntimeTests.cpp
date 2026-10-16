#if WITH_DEV_AUTOMATION_TESTS
#include "UI/MothHUDWidget.h"
#include "Framework/MothEffectPlayerController.h"
#include "Framework/MothWaveDirector.h"
#include "Framework/MothGameInstance.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/HealthComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

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
			FString Directory=TEXT("Automation/ReleaseFlow/Screens");
			FParse::Value(FCommandLine::Get(),TEXT("MothUIScreenDir="),Directory);
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/Directory/Name,true,false);
		}
	}
	class FBasicGuideFlow final:public IAutomationLatentCommand
	{
	public:
		explicit FBasicGuideFlow(FAutomationTestBase* InTest):T(InTest) {}
		bool Update() override
		{
			if (!Started) Started=FPlatformTime::Seconds();
			if (FPlatformTime::Seconds()-Started>45)
			{
				T->AddError(FString::Printf(TEXT("Basic guide and settings flow timeout stage=%d"),Stage));
				if (auto* Current=World()) if (auto* PC=Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(Current,0))) PC->ReturnToTitle();
				return true;
			}
			UWorld* W=World(); if (!W) return false; // OpenLevel may briefly have no gameplay world.
			auto* PC=Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(W,0));
			auto* UI=PC?PC->GetGameplayUI():nullptr;
			auto* D=Director(W);
			auto* S=W->GetSubsystem<UMothTutorialSubsystem>();
			if (!PC||!UI||!D||!S) return false;
			if (Stage==-1) { if (W==Previous.Get()) return false; Stage=0; }
			if (FPlatformTime::Seconds()<Wait) return false;
			if (Stage==0)
			{
				if (D->GetPhase()!=EMothFlowPhase::Ready||UI->GetHUDState().Page!=EMothUIPage::Title)
				{ PC->ReturnToTitle(); Previous=W; Stage=-1; return false; }
				T->TestEqual(TEXT("Initial HUD reports full health"),UI->GetHUDState().Health,100.f);
				T->TestEqual(TEXT("Initial HUD reports a full magazine"),UI->GetHUDState().Ammo,24);
				T->TestTrue(TEXT("Title owns cursor and blocks pawn movement and look"),PC->bShowMouseCursor&&PC->IsMoveInputIgnored()&&PC->IsLookInputIgnored());
				T->TestTrue(TEXT("Title primary action owns keyboard focus"),UI->GetMenuFocusTarget()->HasUserFocus(PC));
				Capture(TEXT("01_Title.png")); Wait=FPlatformTime::Seconds()+.3; Stage=1; return false;
			}
			if (Stage==1)
			{
				if (!Click(UI,TEXT("SecondaryButton"))) return true;
				T->TestTrue(TEXT("Title settings button opens Settings without starting combat"),UI->GetHUDState().Page==EMothUIPage::Settings&&D->GetPhase()==EMothFlowPhase::Ready&&D->GetAliveCount()==0&&D->GetPendingCount()==0);
				T->TestTrue(TEXT("Settings keeps menu input and focuses its cancel action"),PC->bShowMouseCursor&&PC->IsMoveInputIgnored()&&UI->GetMenuFocusTarget()->HasUserFocus(PC));
				T->TestTrue(TEXT("Saved settings panel is visible"),Visible(UI,TEXT("SettingsPanel")));
				Capture(TEXT("02_TitleSettings.png")); Wait=FPlatformTime::Seconds()+.3; Stage=2; return false;
			}
			if (Stage==2)
			{
				if (!Click(UI,TEXT("CancelSettingsButton"))) return true;
				T->TestTrue(TEXT("Cancel from title settings restores Title"),UI->GetHUDState().Page==EMothUIPage::Title&&D->GetPhase()==EMothFlowPhase::Ready);
				if (!Click(UI,TEXT("PrimaryButton"))) return true;
				Previous=W; Stage=3; return false;
			}
			if (Stage==3&&W!=Previous.Get())
			{
				T->TestTrue(TEXT("Start travels to a fresh encounter with gameplay input"),D->GetPhase()==EMothFlowPhase::Preparing&&UI->GetHUDState().Page==EMothUIPage::Gameplay&&!PC->bShowMouseCursor&&!PC->IsMoveInputIgnored()&&!PC->IsLookInputIgnored());
				T->TestTrue(TEXT("Battle travel intent is consumed once"),!PC->GetGameInstance<UMothGameInstance>()->HasBattleAfterTravel());
				T->TestTrue(TEXT("First wave displays the compact basic guide"),Visible(UI,TEXT("GuidePanel")));
				auto* Guide=Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("GuideText")));
				T->TestTrue(TEXT("Basic guide contains player-facing instructions"),Guide&&!Guide->GetText().IsEmpty());
				T->TestEqual(TEXT("Dedicated tutorial remains inactive"),S->GetStep(),EMothTutorialStep::Inactive);
				T->TestEqual(TEXT("Basic guidance creates no tutorial-owned actors"),S->GetOwnedActorCount(),0);
				T->TestFalse(TEXT("Legacy teaching panel is hidden"),Visible(UI,TEXT("TeachingPanel")));
				Capture(TEXT("03_BasicGuide.png")); Wait=FPlatformTime::Seconds()+.3; Stage=4; return false;
			}
			if (Stage==4)
			{
				PC->ToggleMenuPause();
				T->TestTrue(TEXT("Escape freezes combat and opens Pause with menu input"),W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Pause&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored());
				PauseTime=W->GetTimeSeconds(); RunTime=D->GetRunElapsedSeconds();
				Capture(TEXT("04_Pause.png")); Wait=FPlatformTime::Seconds()+.3; Stage=5; return false;
			}
			if (Stage==5)
			{
				T->TestEqual(TEXT("Paused world clock remains frozen while Slate renders"),W->GetTimeSeconds(),PauseTime);
				T->TestEqual(TEXT("Pause also freezes encounter statistics"),D->GetRunElapsedSeconds(),RunTime);
				if (!Click(UI,TEXT("SecondaryButton"))) return true;
				T->TestTrue(TEXT("Pause settings opens without resuming combat"),W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Settings&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored());
				Capture(TEXT("05_PauseSettings.png")); Wait=FPlatformTime::Seconds()+.3; Stage=6; return false;
			}
			if (Stage==6)
			{
				T->TestEqual(TEXT("Settings browsing preserves the paused game clock"),W->GetTimeSeconds(),PauseTime);
				if (!Click(UI,TEXT("CancelSettingsButton"))) return true;
				T->TestTrue(TEXT("Cancel returns to Pause and keeps combat frozen"),W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Pause&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored());
				if (!Click(UI,TEXT("PrimaryButton"))) return true;
				T->TestTrue(TEXT("Resume restores Gameplay and player controls"),!W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Gameplay&&!PC->bShowMouseCursor&&!PC->IsMoveInputIgnored()&&!PC->IsLookInputIgnored());
				T->TestEqual(TEXT("Menu navigation never enters dedicated teaching"),S->GetStep(),EMothTutorialStep::Inactive);
				T->TestEqual(TEXT("Resume has no tutorial actor residue"),S->GetOwnedActorCount(),0);
				Capture(TEXT("06_ResumedBattle.png")); Wait=FPlatformTime::Seconds()+.3; Stage=7; return false;
			}
			if (Stage==7) { PC->ReturnToTitle(); Previous=W; Stage=8; return false; }
			if (Stage==8&&W!=Previous.Get())
			{
				T->TestTrue(TEXT("Acceptance cleanup returns a fresh Title"),D->GetPhase()==EMothFlowPhase::Ready&&UI->GetHUDState().Page==EMothUIPage::Title);
				return true;
			}
			return false;
		}
	private:
		bool Click(UMothHUDWidget* UI,const TCHAR* Name)
		{
			auto* Button=Cast<UButton>(UI->GetWidgetFromName(Name));
			if (!T->TestNotNull(*FString::Printf(TEXT("Saved bound button %s"),Name),Button)) return false;
			Button->OnClicked.Broadcast();
			return true;
		}
		bool Visible(UMothHUDWidget* UI,const TCHAR* Name)
		{
			auto* Widget=UI->GetWidgetFromName(Name);
			return Widget&&Widget->GetVisibility()!=ESlateVisibility::Collapsed&&Widget->GetVisibility()!=ESlateVisibility::Hidden;
		}
		FAutomationTestBase* T; int32 Stage=0; double Started=0,Wait=0,PauseTime=0; float RunTime=0;
		TWeakObjectPtr<UWorld> Previous;
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
				T->TestTrue(TEXT("Battle begins with correct health/ammo and no held prop"),UI->GetHUDState().Health==100&&UI->GetHUDState().Ammo==24&&!UI->GetHUDState().bHeld);
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
				T->TestTrue(TEXT("Return from results restores Title with menu input"),D->GetPhase()==EMothFlowPhase::Ready&&UI->GetHUDState().Page==EMothUIPage::Title&&PC->bShowMouseCursor);
				T->TestTrue(TEXT("Returning to title resets health, ammo, waves and pending enemies"),UI->GetHUDState().Health==100&&UI->GetHUDState().Ammo==24&&UI->GetHUDState().Wave==0&&UI->GetHUDState().Pending==0&&UI->GetHUDState().Alive==0);
				return true;
			}
			return false;
		}
	private:
		FAutomationTestBase* T;int32 Stage=0;double Started=0,Wait=0;TWeakObjectPtr<UWorld> Previous;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothTutorialActionFlow,"MothEffect.UIRuntime.BasicGuideAndPauseSettings",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FMothTutorialActionFlow::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothTutorialRuntimeTest::FBasicGuideFlow(this));return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothMenuTravel,"MothEffect.UIRuntime.StartBattleDefeatAndRestart",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FMothMenuTravel::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothTutorialRuntimeTest::FMenuTravel(this));return true; }
#endif
