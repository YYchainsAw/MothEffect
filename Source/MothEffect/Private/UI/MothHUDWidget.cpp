#include "UI/MothHUDWidget.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/Button.h"
#include "Components/HealthComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Devices/DeviceBase.h"
#include "Engine/World.h"
#include "Framework/MothEffectGameMode.h"
#include "Framework/MothEffectPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Tutorial/MothTutorialConfig.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Weapons/Rifle.h"

namespace
{
FText DeviceName(EDeviceKind Kind)
{
	return FText::FromString(Kind == EDeviceKind::Launcher ? TEXT("抬升器")
		: Kind == EDeviceKind::Bomb ? TEXT("爆裂器") : TEXT("发射器"));
}
}

UMothHUDWidget::UMothHUDWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
	TutorialConfig = TSoftObjectPtr<UMothTutorialConfig>(FSoftObjectPath(TEXT("/Game/MothEffect/UI/DA_MothTutorial.DA_MothTutorial")));
	TitleCopy = {FText::FromString(TEXT("飞蛾效应")), FText::FromString(TEXT("让机关接续，让战局转向。")), FText::FromString(TEXT("开始战斗")), FText::FromString(TEXT("操作教学"))};
	PauseCopy = {FText::FromString(TEXT("暂停")), FText::GetEmpty(), FText::FromString(TEXT("继续游戏")), FText::FromString(TEXT("返回主菜单"))};
	TutorialCompleteCopy = {FText::FromString(TEXT("教学完成")), FText::FromString(TEXT("准备好迎战三波敌人。")), FText::FromString(TEXT("开始战斗")), FText::FromString(TEXT("返回主菜单"))};
	VictoryCopy = {FText::FromString(TEXT("战斗胜利")), FText::FromString(TEXT("三波敌人已全部清除。")), FText::FromString(TEXT("返回主菜单")), FText::GetEmpty()};
	DefeatCopy = {FText::FromString(TEXT("挑战结束")), FText::FromString(TEXT("重整装备，再试一次。")), FText::FromString(TEXT("返回主菜单")), FText::GetEmpty()};
}

UMothTutorialConfig* UMothHUDWidget::GetTutorialConfig() const
{
	if (auto* Config = TutorialConfig.LoadSynchronous()) return Config;
	return GetMutableDefault<UMothTutorialConfig>();
}
void UMothHUDWidget::NativePreConstruct()
{
	bMenuRendered = false;
	if (IsDesignTime())
	{
		State.Health = State.MaxHealth = 100; State.Ammo = State.MagazineSize = 24;
		State.Wave = 1; State.Alive = 4; State.Pending = 6;
		State.Page = PreviewPage; State.Tutorial = PreviewTutorialStep;
		State.bHeld = bPreviewHeld; State.HeldKind = PreviewHeldKind;
		State.ReloadProgress = 0; State.PreparationRemaining = 0;
		State.InteractionPrompt = FText::GetEmpty(); State.Notice = FText::GetEmpty(); State.DamageEdgeAlpha = 0;
		RenderState();
	}
	Super::NativePreConstruct();
}

void UMothHUDWidget::BindButtons(bool bBind)
{
#define MOTH_BUTTON(Button, Handler) if (Button) { if (bBind) Button->OnClicked.AddUniqueDynamic(this,&UMothHUDWidget::Handler); else Button->OnClicked.RemoveDynamic(this,&UMothHUDWidget::Handler); }
	MOTH_BUTTON(PrimaryButton,PrimaryClicked); MOTH_BUTTON(SecondaryButton,SecondaryClicked);
	MOTH_BUTTON(QuitButton,QuitClicked); MOTH_BUTTON(RetryButton,RetryClicked);
#undef MOTH_BUTTON
}

void UMothHUDWidget::NativeConstruct() { Super::NativeConstruct(); BindButtons(true); RefreshState(); }
void UMothHUDWidget::NativeDestruct() { BindButtons(false); UnbindSources(); Super::NativeDestruct(); }
void UMothHUDWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds) { Super::NativeTick(Geometry,DeltaSeconds); RefreshState(); }

void UMothHUDWidget::UnbindSources()
{
	if (Player.IsValid())
	{
		Player->GetHealthComponent()->OnHealthChanged.RemoveDynamic(this,&UMothHUDWidget::HandleHealth);
		Player->OnPlayerActionStateChanged.RemoveDynamic(this,&UMothHUDWidget::HandleAction);
		Player->OnInteractionRejectedNative.RemoveAll(this);
	}
	if (Rifle.IsValid())
	{
		Rifle->OnAmmoChanged.RemoveDynamic(this,&UMothHUDWidget::HandleAmmo);
		Rifle->OnEmptyMagazine.RemoveDynamic(this,&UMothHUDWidget::HandleEmpty);
		Rifle->OnMuzzleBlocked.RemoveDynamic(this,&UMothHUDWidget::HandleBlocked);
	}
	if (Director.IsValid()) Director->OnFlowChanged.RemoveDynamic(this,&UMothHUDWidget::HandleFlow);
	if (Tutorial.IsValid()) Tutorial->OnStepChanged.RemoveDynamic(this,&UMothHUDWidget::HandleTutorial);
	if (CombatFeedback.IsValid()) CombatFeedback->OnEnemyHitConfirmedNative.RemoveAll(this);
	Player.Reset(); Rifle.Reset(); Director.Reset(); Tutorial.Reset(); CombatFeedback.Reset();
	Notice=FText::GetEmpty(); NoticeEndsAt=0;
	bCrosshairPageAnnounced = false;
	bCrosshairPaused = false;
	OnCrosshairFeedbackReset();
}

void UMothHUDWidget::BindSources()
{
	auto* NewPlayer=Cast<AMothEffectCharacter>(GetOwningPlayerPawn());
	auto* Mode=GetWorld()?Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode()):nullptr;
	auto* NewDirector=Mode?Mode->GetWaveDirector():nullptr;
	auto* NewRifle=NewPlayer?NewPlayer->GetRifle():nullptr;
	auto* NewTutorial=GetWorld()?GetWorld()->GetSubsystem<UMothTutorialSubsystem>():nullptr;
	auto* NewFeedback=GetWorld()?GetWorld()->GetSubsystem<UMothFeedbackSubsystem>():nullptr;
	if (Player.Get()==NewPlayer && Director.Get()==NewDirector && Rifle.Get()==NewRifle
		&& Tutorial.Get()==NewTutorial && CombatFeedback.Get()==NewFeedback) return;
	UnbindSources(); Player=NewPlayer; Director=NewDirector; Rifle=NewRifle; Tutorial=NewTutorial; CombatFeedback=NewFeedback;
	if (Player.IsValid())
	{
		Player->GetHealthComponent()->OnHealthChanged.AddUniqueDynamic(this,&UMothHUDWidget::HandleHealth);
		Player->OnPlayerActionStateChanged.AddUniqueDynamic(this,&UMothHUDWidget::HandleAction);
		Player->OnInteractionRejectedNative.AddUObject(this,&UMothHUDWidget::ShowInteractionNotice);
	}
	if (Rifle.IsValid())
	{
		Rifle->OnAmmoChanged.AddUniqueDynamic(this,&UMothHUDWidget::HandleAmmo);
		Rifle->OnEmptyMagazine.AddUniqueDynamic(this,&UMothHUDWidget::HandleEmpty);
		Rifle->OnMuzzleBlocked.AddUniqueDynamic(this,&UMothHUDWidget::HandleBlocked);
	}
	if (Director.IsValid()) Director->OnFlowChanged.AddUniqueDynamic(this,&UMothHUDWidget::HandleFlow);
	if (Tutorial.IsValid()) Tutorial->OnStepChanged.AddUniqueDynamic(this,&UMothHUDWidget::HandleTutorial);
	if (CombatFeedback.IsValid())
		CombatFeedback->OnEnemyHitConfirmedNative.AddUObject(this,&UMothHUDWidget::HandleEnemyHitConfirmed);
}

void UMothHUDWidget::HandleEnemyHitConfirmed(APawn* InstigatorPawn, const FMothEnemyHitFeedback& Feedback)
{
	if (IsDesignTime() || !IsValid(InstigatorPawn) || InstigatorPawn != GetOwningPlayerPawn()) return;
	UWorld* World = GetWorld();
	const auto* Session = World ? World->GetSubsystem<UDeviceLifecycleSubsystem>() : nullptr;
	if (!World || World->IsPaused() || !CombatFeedback.IsValid() || CombatFeedback->GetWorld() != World
		|| !Session || !Session->IsGenerationCurrent(Feedback.LifecycleGeneration)) return;
	OnEnemyHitConfirmed(Feedback);
}

void UMothHUDWidget::RefreshState()
{
	if (bRendering || IsDesignTime()) return;
	TGuardValue<bool> Guard(bRendering,true); BindSources();
	const EMothUIPage PreviousPage = State.Page;
	State.Health=Player.IsValid()?Player->GetHealthComponent()->GetCurrentHealth():0;
	State.MaxHealth=Player.IsValid()?Player->GetHealthComponent()->GetMaxHealth():0;
	State.Ammo=Rifle.IsValid()?Rifle->GetAmmoInMagazine():0;
	State.MagazineSize=Rifle.IsValid()?Rifle->GetMagazineSize():0;
	State.bReloading=Rifle.IsValid()&&Rifle->IsReloading();
	State.ReloadProgress=Rifle.IsValid()?Rifle->GetReloadProgress():0;
	State.bHeld=Player.IsValid()&&Player->IsCarryingDevice();
	if (State.bHeld) State.HeldKind=Player->GetHeldDevice()->GetDeviceKind();
	State.Wave=Director.IsValid()?Director->GetWaveNumber():0;
	State.Alive=Director.IsValid()?Director->GetAliveCount():0;
	State.Pending=Director.IsValid()?Director->GetPendingCount():0;
	State.PreparationRemaining=Director.IsValid()?Director->GetPreparationRemaining():0;
	State.Tutorial=Tutorial.IsValid()?Tutorial->GetStep():EMothTutorialStep::Inactive;
	const EMothFlowPhase Phase=Director.IsValid()?Director->GetPhase():EMothFlowPhase::Ready;
	if (Phase==EMothFlowPhase::Victory) State.Page=EMothUIPage::Victory;
	else if (Phase==EMothFlowPhase::Defeat) State.Page=EMothUIPage::Defeat;
	else if (GetWorld()&&GetWorld()->IsPaused()) State.Page=EMothUIPage::Pause;
	else if (Phase==EMothFlowPhase::Preparing||Phase==EMothFlowPhase::Fighting) State.Page=EMothUIPage::Gameplay;
	else if (State.Tutorial==EMothTutorialStep::Completed) State.Page=EMothUIPage::TutorialComplete;
	else if (Tutorial.IsValid()&&Tutorial->IsRunning()) State.Page=EMothUIPage::Tutorial;
	else State.Page=EMothUIPage::Title;
	RefreshInteractionState();
	State.Notice=GetWorld()&&GetWorld()->GetTimeSeconds()<NoticeEndsAt?Notice:
		Tutorial.IsValid()?Tutorial->GetFeedback():FText::GetEmpty();
	State.DamageEdgeAlpha=!IsMenuVisible()&&CombatFeedback.IsValid()?CombatFeedback->GetDamageEdgeAlpha():0;
	RenderState();
	const bool bPaused = GetWorld() && GetWorld()->IsPaused();
	if (bCrosshairPaused != bPaused)
	{
		bCrosshairPaused = bPaused;
		OnCrosshairPauseChanged(bPaused);
	}
	if (!bCrosshairPageAnnounced || PreviousPage != State.Page)
	{
		bCrosshairPageAnnounced = true;
		if (State.Page == EMothUIPage::Title || State.Page == EMothUIPage::Defeat
			|| State.Page == EMothUIPage::TutorialComplete) OnCrosshairFeedbackReset();
		OnCrosshairPageChanged(State.Page);
	}
}

bool UMothHUDWidget::IsMenuVisible() const { return State.Page!=EMothUIPage::Gameplay&&State.Page!=EMothUIPage::Tutorial; }
UWidget* UMothHUDWidget::GetMenuFocusTarget() const { return PrimaryButton?static_cast<UWidget*>(PrimaryButton):const_cast<UMothHUDWidget*>(this); }

void UMothHUDWidget::RefreshInteractionState()
{
	State.bAimingAtDevice=false; State.bCanPickup=false; State.InteractionPrompt=FText::GetEmpty();
	if (!Player.IsValid()||State.bHeld||IsMenuVisible()) return;
	FVector Origin,Direction;
	if (Player->GetRifleView(Origin,Direction))
	{
		FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(MothHUDDeviceAim),true,Player.Get());
		if (Rifle.IsValid()) Params.AddIgnoredActor(Rifle.Get());
		if (GetWorld()->LineTraceSingleByChannel(Hit,Origin,Origin+Direction*8000,
			Rifle.IsValid()?Rifle->GetWeaponTraceChannel():ECC_Visibility,Params))
			if (const auto* Device=Cast<ADeviceBase>(Hit.GetActor()); Device&&Device->GetDeviceState()==EDeviceState::Dormant)
			{
				State.bAimingAtDevice=true; State.AimedKind=Device->GetDeviceKind();
				State.bCanPickup=Player->GetPickupCandidate()==Device;
				State.InteractionPrompt=FText::FromString(DeviceName(State.AimedKind).ToString()+
					(State.bCanPickup?TEXT(" · 拾取 / 射击启动"):TEXT(" · 射击启动")));
			}
	}
}

void UMothHUDWidget::RenderState()
{
	// A saved Designer tree is the only presentation. Missing optional controls
	// never trigger a fallback tree, font replacement, native paint or glyph.
	if (HUD) HUD->SetVisibility(IsMenuVisible()?ESlateVisibility::Collapsed:ESlateVisibility::SelfHitTestInvisible);
	if (Menu) Menu->SetVisibility(IsMenuVisible()?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
	if (HealthText) HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"),State.Health,State.MaxHealth)));
	if (HealthBar) HealthBar->SetPercent(State.MaxHealth>0?FMath::Clamp(State.Health/State.MaxHealth,0.f,1.f):0);
	if (AmmoText) AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"),State.Ammo,State.MagazineSize)));
	if (ReloadBar)
	{
		ReloadBar->SetVisibility(State.bReloading?ESlateVisibility::Visible:ESlateVisibility::Hidden);
		ReloadBar->SetPercent(State.ReloadProgress);
	}
	if (WaveText) WaveText->SetText(FText::FromString(State.Page==EMothUIPage::Tutorial?TEXT("操作教学"):
		FString::Printf(TEXT("第%d波 / 3"),State.Wave)));
	if (WaveDetailText)
	{
		const bool bPreparing=Director.IsValid()&&Director->GetPhase()==EMothFlowPhase::Preparing;
		WaveDetailText->SetText(State.Page==EMothUIPage::Tutorial?FText::GetEmpty():
			FText::FromString(bPreparing?FString::Printf(TEXT("%.1f 秒后开始"),State.PreparationRemaining):
				FString::Printf(TEXT("剩余 %d"),State.Alive+State.Pending)));
	}
	if (HeldText) HeldText->SetText(State.bHeld?DeviceName(State.HeldKind):FText::GetEmpty());
	if (InteractionText) InteractionText->SetText(State.InteractionPrompt);
	if (AimPanel) AimPanel->SetVisibility(State.InteractionPrompt.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
	if (NoticeText) NoticeText->SetText(State.Notice);
	if (NoticePanel) NoticePanel->SetVisibility(State.Notice.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
	if (DamageOverlay)
	{
		DamageOverlay->SetRenderOpacity(FMath::Clamp(State.DamageEdgeAlpha,0.f,1.f));
		DamageOverlay->SetVisibility(State.DamageEdgeAlpha>0?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
	}
	const bool bTeaching=State.Page==EMothUIPage::Tutorial;
	if (TeachingPanel) TeachingPanel->SetVisibility(bTeaching?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
	if (TutorialText && IsDesignTime())
	{
		const auto* Config=GetTutorialConfig();
		TutorialText->SetText(State.Tutorial==EMothTutorialStep::BombAndLift?Config->BombAndLiftInstruction:
			State.Tutorial==EMothTutorialStep::AirborneEmitter?Config->AirborneInstruction:
			State.Tutorial==EMothTutorialStep::IndirectChain?Config->IndirectInstruction:FText::GetEmpty());
	}
	else if (TutorialText) TutorialText->SetText(Tutorial.IsValid()?Tutorial->GetInstruction():FText::GetEmpty());
	if (TutorialStepText) TutorialStepText->SetText(FText::FromString(FString::Printf(TEXT("教学 %d / 3"),static_cast<int32>(State.Tutorial))));
	if (!bMenuRendered || RenderedPage!=State.Page)
	{
		RenderedPage=State.Page; bMenuRendered=true;
		const bool bTitle=State.Page==EMothUIPage::Title;
		const bool bPause=State.Page==EMothUIPage::Pause;
		const bool bComplete=State.Page==EMothUIPage::TutorialComplete;
		const FMothMenuCopy& Copy=bTitle?TitleCopy:bPause?PauseCopy:bComplete?TutorialCompleteCopy:
			State.Page==EMothUIPage::Victory?VictoryCopy:DefeatCopy;
		if (MenuTitle) MenuTitle->SetText(Copy.Title);
		if (MenuDescription) MenuDescription->SetText(Copy.Description);
		if (PrimaryLabel) PrimaryLabel->SetText(Copy.PrimaryAction);
		if (SecondaryLabel) SecondaryLabel->SetText(Copy.SecondaryAction);
		if (SecondaryButton) SecondaryButton->SetVisibility(bTitle||bPause||bComplete?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
	}
}

void UMothHUDWidget::ShowInteractionNotice(const FText& Message)
{
	Notice=Message.ToString().Contains(TEXT("Socket"))?FText::FromString(TEXT("暂时无法携带这个道具。")):Message;
	NoticeEndsAt=GetWorld()?GetWorld()->GetTimeSeconds()+3:0;
}
void UMothHUDWidget::HandleHealth(float,float) { RefreshState(); }
void UMothHUDWidget::HandleAmmo(int32,int32) { RefreshState(); }
void UMothHUDWidget::HandleAction(EPlayerActionState,EPlayerActionState) { RefreshState(); }
void UMothHUDWidget::HandleFlow(EMothFlowPhase,int32,int32,int32) { RefreshState(); }
void UMothHUDWidget::HandleTutorial(EMothTutorialStep)
{
	// Retrying the same step keeps the Tutorial page, so page changes cannot clear it.
	OnCrosshairFeedbackReset();
	RefreshState();
}
void UMothHUDWidget::HandleEmpty(ARifle*) { ShowInteractionNotice(FText::FromString(TEXT("弹匣已空，按 R 换弹。"))); }
void UMothHUDWidget::HandleBlocked(ARifle*) { ShowInteractionNotice(FText::FromString(TEXT("枪口被遮挡，移开后再射击。"))); }

void UMothHUDWidget::PrimaryClicked()
{
	if (auto* Controller=Cast<AMothEffectPlayerController>(GetOwningPlayer()))
	{
		if (State.Page==EMothUIPage::Title) Controller->StartBattle();
		else if (State.Page==EMothUIPage::Pause) Controller->ResumeGame();
		else if (State.Page==EMothUIPage::TutorialComplete) Controller->StartBattle();
		else if (State.Page==EMothUIPage::Victory||State.Page==EMothUIPage::Defeat) Controller->ReturnToTitle();
	}
}
void UMothHUDWidget::SecondaryClicked()
{
	if (auto* Controller=Cast<AMothEffectPlayerController>(GetOwningPlayer()))
		if (State.Page==EMothUIPage::Title) Controller->StartTeaching(); else Controller->ReturnToTitle();
}
void UMothHUDWidget::RetryClicked() { if (Tutorial.IsValid()) Tutorial->RetryStep(); }
void UMothHUDWidget::QuitClicked() { UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false); }
FReply UMothHUDWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
	if (Event.GetKey()==EKeys::Escape)
	{
		if (!Event.IsRepeat()) if (auto* Controller=Cast<AMothEffectPlayerController>(GetOwningPlayer())) Controller->ToggleMenuPause();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry,Event);
}
