#if WITH_DEV_AUTOMATION_TESTS
#include "UI/MothHUDWidget.h"
#include "UI/MothLoadingSubsystem.h"
#include "Art/MothMenuCharacter.h"
#include "Framework/MothEffectPlayerController.h"
#include "Framework/MothWaveDirector.h"
#include "Framework/MothGameInstance.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "Characters/MothEffectCharacter.h"
#include "AI/EnemyBase.h"
#include "Devices/DeviceBase.h"
#include "Weapons/Rifle.h"
#include "Components/HealthComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace MothMenuRuntimeTest
{
	UWorld* World()
	{
		for (const auto& Context:GEngine->GetWorldContexts()) if (Context.WorldType==EWorldType::Game) return Context.World();
		return nullptr;
	}
	template<typename T> int32 ActorCount(UWorld* W)
	{
		int32 Count=0; for (TActorIterator<T> It(W);It;++It) if (!It->IsActorBeingDestroyed()) ++Count; return Count;
	}
	AMothWaveDirector* Director(UWorld* W)
	{
		for (TActorIterator<AMothWaveDirector> It(W);It;++It) if (!It->IsActorBeingDestroyed()) return *It; return nullptr;
	}
	void Capture(const TCHAR* Name)
	{
		if (FParse::Param(FCommandLine::Get(),TEXT("MothUIScreens")))
		{
			FString Directory=TEXT("Automation/MainMenuFlow/Screens");
			FParse::Value(FCommandLine::Get(),TEXT("MothUIScreenDir="),Directory);
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/Directory/Name,true,false);
		}
	}
	bool Click(FAutomationTestBase* T,UMothHUDWidget* UI,const TCHAR* Name)
	{
		if (UI->GetHUDState().Page==EMothUIPage::Title)
		{
			if (FCString::Strcmp(Name,TEXT("PrimaryButton"))==0) Name=TEXT("FrontStartButton");
			else if (FCString::Strcmp(Name,TEXT("SecondaryButton"))==0) Name=TEXT("FrontSettingsButton");
			else if (FCString::Strcmp(Name,TEXT("CreditsButton"))==0) Name=TEXT("FrontCreditsButton");
		}
		auto* Button=Cast<UButton>(UI->GetWidgetFromName(Name));
		if (!T->TestNotNull(*FString::Printf(TEXT("Saved bound button %s"),Name),Button)) return false;
		if (!T->TestTrue(TEXT("Only a visible enabled action is clicked"),Button->IsVisible()&&Button->GetIsEnabled())) return false;
		Button->OnClicked.Broadcast(); return true;
	}
	bool Visible(UMothHUDWidget* UI,const TCHAR* Name)
	{
		auto* Widget=UI->GetWidgetFromName(Name);
		return Widget&&Widget->GetVisibility()!=ESlateVisibility::Collapsed&&Widget->GetVisibility()!=ESlateVisibility::Hidden;
	}
	void CheckSingleHUD(FAutomationTestBase* T,UWorld* W,AMothEffectPlayerController* PC)
	{
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(W,Widgets,UMothHUDWidget::StaticClass(),false);
		int32 Count=0; for (const auto* Widget:Widgets) if (Widget->GetOwningPlayer()==PC&&Widget->IsInViewport()) ++Count;
		T->TestEqual(TEXT("Exactly one active player HUD survives each real map travel"),Count,1);
	}
	void CheckMenu(FAutomationTestBase* T,UWorld* W,AMothEffectPlayerController* PC,UMothHUDWidget* UI)
	{
		T->TestEqual(TEXT("Title belongs to the independent saved menu map"),UGameplayStatics::GetCurrentLevelName(W,true),FString(TEXT("L_MainMenu")));
		T->TestEqual(TEXT("Independent menu presents Title without a gameplay frame"),UI->GetHUDState().Page,EMothUIPage::Title);
		T->TestNull(TEXT("Menu controller has no playable or spectator pawn"),PC->GetPawn());
		T->TestEqual(TEXT("Menu spawns no pawn, including an unpossessed spectator"),ActorCount<APawn>(W),0);
		T->TestEqual(TEXT("Menu has no player character"),ActorCount<AMothEffectCharacter>(W),0);
		T->TestEqual(TEXT("Menu has no enemies"),ActorCount<AEnemyBase>(W),0);
		T->TestEqual(TEXT("Menu has no gameplay devices"),ActorCount<ADeviceBase>(W),0);
		T->TestEqual(TEXT("Menu has no wave director"),ActorCount<AMothWaveDirector>(W),0);
		T->TestEqual(TEXT("One animated player likeness belongs to the menu"),ActorCount<AMothMenuCharacter>(W),1);
		T->TestTrue(TEXT("Title exposes the supplied logo and right-side actions"),Visible(UI,TEXT("FrontLogo"))&&Visible(UI,TEXT("FrontEndPanel"))&&Visible(UI,TEXT("FrontStartButton")));
		T->TestFalse(TEXT("Title removes the old fullscreen shade"),Visible(UI,TEXT("MenuShade")));
		T->TestFalse(TEXT("Title removes the old menu card"),Visible(UI,TEXT("MenuCard")));
		auto* Camera=Cast<ACameraActor>(PC->GetViewTarget());
		T->TestTrue(TEXT("Menu uses its marked fixed camera"),Camera&&Camera->ActorHasTag(TEXT("MothMenuCamera")));
		T->TestTrue(TEXT("Title owns cursor and blocks movement and look"),PC->bShowMouseCursor&&PC->IsMoveInputIgnored()&&PC->IsLookInputIgnored());
		auto* Focus=UI->GetMenuFocusTarget();
		T->TestTrue(TEXT("Title primary action owns keyboard focus"),Focus&&Focus->HasUserFocus(PC));
		T->TestFalse(TEXT("Combat HUD remains hidden throughout title"),Visible(UI,TEXT("HUD")));
		T->TestFalse(TEXT("Basic guide remains hidden throughout title"),Visible(UI,TEXT("GuidePanel")));
		T->TestFalse(TEXT("No battle travel is pending in menu"),PC->GetGameInstance<UMothGameInstance>()->HasBattleAfterTravel());
		CheckSingleHUD(T,W,PC);
	}
	void CheckBattle(FAutomationTestBase* T,UWorld* W,AMothEffectPlayerController* PC,UMothHUDWidget* UI,AMothWaveDirector* D)
	{
		T->TestEqual(TEXT("Battle travels to the saved SoulCity arena"),UGameplayStatics::GetCurrentLevelName(W,true),FString(TEXT("L_Arena_SoulCity")));
		T->TestTrue(TEXT("First visible battle HUD is Gameplay in first-wave preparation"),D->GetPhase()==EMothFlowPhase::Preparing&&D->GetWaveNumber()==1&&UI->GetHUDState().Page==EMothUIPage::Gameplay);
		T->TestTrue(TEXT("Battle owns player input"),!PC->bShowMouseCursor&&!PC->IsMoveInputIgnored()&&!PC->IsLookInputIgnored());
		T->TestFalse(TEXT("Battle travel intent is consumed once"),PC->GetGameInstance<UMothGameInstance>()->HasBattleAfterTravel());
		T->TestTrue(TEXT("Battle starts with full player health and ammunition"),UI->GetHUDState().Health==100&&UI->GetHUDState().Ammo==24&&!UI->GetHUDState().bHeld);
		const auto* Loading=PC->GetGameInstance()->GetSubsystem<UMothLoadingSubsystem>();
		T->TestTrue(TEXT("Start has displayed and dismissed a readable loading page"),Loading&&!Loading->IsLoading()&&Loading->GetLastDisplaySeconds()>=.79f);
		T->TestTrue(TEXT("A real Game map load starts MoviePlayer presentation"),Loading&&Loading->WasMoviePlayerUsed());
		CheckSingleHUD(T,W,PC);
	}

	class FBasicGuideFlow final:public IAutomationLatentCommand
	{
	public:
		explicit FBasicGuideFlow(FAutomationTestBase* InTest):T(InTest) {}
		bool Update() override
		{
			if (!Started) Started=FPlatformTime::Seconds();
			if (FPlatformTime::Seconds()-Started>90) { T->AddError(FString::Printf(TEXT("Independent menu/settings flow timeout stage=%d"),Stage)); return true; }
			UWorld* W=World(); if (!W) return false;
			if (auto* Loading=W->GetGameInstance()->GetSubsystem<UMothLoadingSubsystem>(); Loading&&Loading->IsLoading())
			{
				if (!bCapturedLoading) { Capture(TEXT("12_Loading.png")); bCapturedLoading=true; }
				return false;
			}
			auto* PC=Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(W,0));
			auto* UI=PC?PC->GetGameplayUI():nullptr; if (!PC||!UI) return false;
			if (Stage==-1) { if (W==Previous.Get()) return false; Stage=0; }
			if (FPlatformTime::Seconds()<Wait) return false;
			auto* D=Director(W);
			if (Stage==0)
			{
				if (UGameplayStatics::GetCurrentLevelName(W,true)!=TEXT("L_MainMenu")) { PC->ReturnToTitle(); Previous=W; Stage=-1; return false; }
				CheckMenu(T,W,PC,UI); Capture(TEXT("01_IndependentTitle.png")); Wait=FPlatformTime::Seconds()+.4; Stage=1; return false;
			}
			if (Stage==1)
			{
				CheckMenu(T,W,PC,UI); if (!Click(T,UI,TEXT("SecondaryButton"))) return true;
				T->TestTrue(TEXT("Settings works without a combat pawn or director"),UI->GetHUDState().Page==EMothUIPage::Settings&&!PC->GetPawn()&&!D&&Visible(UI,TEXT("SettingsPanel")));
				T->TestTrue(TEXT("Menu settings focuses its cancel action"),UI->GetMenuFocusTarget()&&UI->GetMenuFocusTarget()->HasUserFocus(PC));
				Capture(TEXT("02_MenuSettings.png")); Wait=FPlatformTime::Seconds()+.3; Stage=2; return false;
			}
			if (Stage==2)
			{
				if (!Click(T,UI,TEXT("CancelSettingsButton"))) return true; CheckMenu(T,W,PC,UI);
				if (!Click(T,UI,TEXT("CreditsButton"))) return true;
				T->TestTrue(TEXT("Credits works without a combat world"),UI->GetHUDState().Page==EMothUIPage::Credits&&Visible(UI,TEXT("CreditsPanel"))&&!D&&!PC->GetPawn());
				Capture(TEXT("03_MenuCredits.png")); Wait=FPlatformTime::Seconds()+.3; Stage=3; return false;
			}
			if (Stage==3)
			{
				if (!Click(T,UI,TEXT("CreditsBackButton"))) return true; CheckMenu(T,W,PC,UI);
				if (!Click(T,UI,TEXT("PrimaryButton"))) return true; Previous=W; Stage=4; return false;
			}
			if (Stage==4&&W!=Previous.Get())
			{
				T->TestFalse(TEXT("Starting battle never exposes another Title frame"),UI->GetHUDState().Page==EMothUIPage::Title);
				auto* P=Cast<AMothEffectCharacter>(PC->GetPawn()); if (!D||!P||!P->GetRifle()) return false;
				CheckBattle(T,W,PC,UI,D);
				T->TestTrue(TEXT("First wave displays a compact basic guide"),Visible(UI,TEXT("GuidePanel")));
				auto* Guide=Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("GuideText")));
				T->TestTrue(TEXT("Basic guide contains player-facing instructions"),Guide&&!Guide->GetText().IsEmpty());
				auto* S=W->GetSubsystem<UMothTutorialSubsystem>();
				T->TestTrue(TEXT("No dedicated tutorial or tutorial-owned actors start"),S&&S->GetStep()==EMothTutorialStep::Inactive&&S->GetOwnedActorCount()==0&&!Visible(UI,TEXT("TeachingPanel")));
				Capture(TEXT("04_BasicGuide.png")); Wait=FPlatformTime::Seconds()+.3; Stage=5; return false;
			}
			if (Stage==5)
			{
				PC->ToggleMenuPause(); T->TestTrue(TEXT("Escape freezes battle and opens Pause"),W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Pause&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored());
				PauseTime=W->GetTimeSeconds(); RunTime=D->GetRunElapsedSeconds(); Capture(TEXT("05_Pause.png")); Wait=FPlatformTime::Seconds()+.3; Stage=6; return false;
			}
			if (Stage==6)
			{
				T->TestEqual(TEXT("Paused world clock remains frozen while Slate renders"),W->GetTimeSeconds(),PauseTime);
				T->TestEqual(TEXT("Pause freezes run statistics"),D->GetRunElapsedSeconds(),RunTime);
				if (!Click(T,UI,TEXT("SecondaryButton"))) return true;
				T->TestTrue(TEXT("Pause settings preserves world pause"),W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Settings);
				Capture(TEXT("06_PauseSettings.png")); Wait=FPlatformTime::Seconds()+.3; Stage=7; return false;
			}
			if (Stage==7)
			{
				T->TestEqual(TEXT("Settings browsing preserves paused clock"),W->GetTimeSeconds(),PauseTime);
				if (!Click(T,UI,TEXT("CancelSettingsButton"))) return true;
				T->TestTrue(TEXT("Cancel returns to Pause"),W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Pause);
				if (!Click(T,UI,TEXT("PrimaryButton"))) return true;
				T->TestTrue(TEXT("Resume restores Gameplay input"),!W->IsPaused()&&UI->GetHUDState().Page==EMothUIPage::Gameplay&&!PC->bShowMouseCursor&&!PC->IsMoveInputIgnored());
				PC->ToggleMenuPause(); if (!Click(T,UI,TEXT("RestartButton"))) return true; Previous=W; Stage=8; return false;
			}
			if (Stage==8&&W!=Previous.Get())
			{
				T->TestFalse(TEXT("Pause Restart never exposes Title"),UI->GetHUDState().Page==EMothUIPage::Title);
				if (!D||!PC->GetPawn()) return false; CheckBattle(T,W,PC,UI,D);
				T->TestFalse(TEXT("Pause Restart clears pause"),W->IsPaused());
				PC->ToggleMenuPause(); if (!Click(T,UI,TEXT("TitleButton"))) return true; Previous=W; Stage=9; return false;
			}
			if (Stage==9&&W!=Previous.Get()) { CheckMenu(T,W,PC,UI); return true; }
			return false;
		}
	private:
		FAutomationTestBase* T; int32 Stage=0; double Started=0,Wait=0,PauseTime=0; float RunTime=0; bool bCapturedLoading=false; TWeakObjectPtr<UWorld> Previous;
	};

	class FMenuTravel final:public IAutomationLatentCommand
	{
	public:
		explicit FMenuTravel(FAutomationTestBase* InTest):T(InTest) {}
		bool Update() override
		{
			if (!Started) Started=FPlatformTime::Seconds();
			if (FPlatformTime::Seconds()-Started>90) { T->AddError(FString::Printf(TEXT("Independent menu battle/results travel timeout stage=%d"),Stage)); return true; }
			UWorld* W=World(); if (!W) return false;
			if (auto* Loading=W->GetGameInstance()->GetSubsystem<UMothLoadingSubsystem>(); Loading&&Loading->IsLoading()) return false;
			auto* PC=Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(W,0));
			auto* UI=PC?PC->GetGameplayUI():nullptr; if (!PC||!UI) return false;
			auto* D=Director(W); if (FPlatformTime::Seconds()<Wait) return false;
			if (Stage==-1) { if (Previous.Get()==W) return false; Stage=0; }
			if (Stage==0)
			{
				if (UGameplayStatics::GetCurrentLevelName(W,true)!=TEXT("L_MainMenu")) { PC->ReturnToTitle(); Previous=W; Stage=-1; return false; }
				CheckMenu(T,W,PC,UI); if (!Click(T,UI,TEXT("PrimaryButton"))) return true; Previous=W; Stage=1; return false;
			}
			if ((Stage==1||Stage==4||Stage==7)&&W!=Previous.Get())
			{
				T->TestFalse(TEXT("Start/replay never shows a Title frame inside the arena"),UI->GetHUDState().Page==EMothUIPage::Title);
				auto* P=Cast<AMothEffectCharacter>(PC->GetPawn()); if (!D||!P||!P->GetRifle()) return false;
				CheckBattle(T,W,PC,UI,D);
				if (Stage==7) { PC->ReturnToTitle(); Previous=W; Stage=8; return false; }
				Capture(Stage==1?TEXT("07_BattleHUD.png"):TEXT("09_ReplayHUD.png")); Wait=FPlatformTime::Seconds()+.4; Stage=Stage==1?2:5; return false;
			}
			if (Stage==2||Stage==5)
			{
				auto* P=Cast<AMothEffectCharacter>(PC->GetPawn());
				FHitContext Hit; Hit.HitId=FGuid::NewGuid(); Hit.Damage=20; Hit.ShotDirection=FVector::ForwardVector;
				P->GetHealthComponent()->ApplyHit(Hit); T->TestEqual(TEXT("Health updates live HUD immediately"),UI->GetHUDState().Health,80.f);
				Hit.HitId=FGuid::NewGuid(); Hit.Damage=100; P->GetHealthComponent()->ApplyHit(Hit); UI->RefreshState(); PC->SyncUIInput();
				T->TestTrue(TEXT("Lethal damage commits Defeat with menu input and no pending enemies"),D->GetPhase()==EMothFlowPhase::Defeat&&D->GetPendingCount()==0&&UI->GetHUDState().Page==EMothUIPage::Defeat&&PC->bShowMouseCursor&&PC->IsMoveInputIgnored());
				Capture(Stage==2?TEXT("08_Defeat.png"):TEXT("10_ReplayDefeat.png")); Wait=FPlatformTime::Seconds()+.4; Stage=Stage==2?3:6; return false;
			}
			if (Stage==3) { if (!Click(T,UI,TEXT("PrimaryButton"))) return true; Previous=W; Stage=4; return false; }
			if (Stage==6) { if (!Click(T,UI,TEXT("SecondaryButton"))) return true; Previous=W; Stage=9; return false; }
			if (Stage==9&&W!=Previous.Get())
			{
				CheckMenu(T,W,PC,UI); Capture(TEXT("11_ResultReturnMenu.png"));
				if (!Click(T,UI,TEXT("PrimaryButton"))) return true; Previous=W; Stage=7; return false;
			}
			if (Stage==8&&W!=Previous.Get()) { CheckMenu(T,W,PC,UI); return true; }
			return false;
		}
	private:
		FAutomationTestBase* T; int32 Stage=0; double Started=0,Wait=0; TWeakObjectPtr<UWorld> Previous;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothTutorialActionFlow,"MothEffect.UIRuntime.BasicGuideAndPauseSettings",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FMothTutorialActionFlow::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothMenuRuntimeTest::FBasicGuideFlow(this)); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothMenuTravel,"MothEffect.UIRuntime.StartBattleDefeatAndRestart",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FMothMenuTravel::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothMenuRuntimeTest::FMenuTravel(this)); return true; }
#endif
