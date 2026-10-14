#include "UI/MothHUDWidget.h"
#include "UI/MothDeviceIcon.h"
#include "Blueprint/WidgetTree.h"
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
#include "MothHUDLayout.h"
#include "Tutorial/MothTutorialConfig.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

UMothHUDWidget::UMothHUDWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
	TutorialConfig = TSoftObjectPtr<UMothTutorialConfig>(FSoftObjectPath(TEXT("/Game/MothEffect/UI/DA_MothTutorial.DA_MothTutorial")));
	TitleCopy = {FText::FromString(TEXT("飞蛾效应")), FText::FromString(TEXT("击中启动，方向锁定，机关互相接续。\n先学习三种机关，再迎战三波敌人。")), FText::FromString(TEXT("开始教学")), FText::FromString(TEXT("直接进入竞技场"))};
	PauseCopy = {FText::FromString(TEXT("暂停")), FText::FromString(TEXT("继续后回到当前战斗。\n投掷后仍需松开左键，再重新按下才能射击。")), FText::FromString(TEXT("继续游戏")), FText::FromString(TEXT("返回入口"))};
	TutorialCompleteCopy = {FText::FromString(TEXT("教学完成")), FText::FromString(TEXT("你已完成抬升、空中启动和间接连锁。\n进入竞技场时生命与弹药恢复，教学道具清空。")), FText::FromString(TEXT("开始战斗")), FText::FromString(TEXT("返回入口"))};
	VictoryCopy = {FText::FromString(TEXT("竞技场胜利")), FText::FromString(TEXT("三波敌人已清除。\n重新开始可返回入口，再次训练或挑战。")), FText::FromString(TEXT("重新开始")), FText::GetEmpty()};
	DefeatCopy = {FText::FromString(TEXT("本次挑战结束")), FText::FromString(TEXT("生命耗尽或离开了竞技场。\n重新开始，尝试用连锁改变战局。")), FText::FromString(TEXT("重新开始")), FText::GetEmpty()};
}

UMothTutorialConfig* UMothHUDWidget::GetTutorialConfig() const
{
	if (auto* Config = TutorialConfig.LoadSynchronous()) return Config;
	return GetMutableDefault<UMothTutorialConfig>();
}
#include "Weapons/Rifle.h"

TSharedRef<SWidget> UMothHUDWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget) BuildLayout();
    return Super::RebuildWidget();
}

void UMothHUDWidget::BuildLayout()
{
	MothHUDLayout::Build(WidgetTree);
	// Native fallback has no generated class to assign BindWidget fields.
#define MOTH_BIND(Type, Name) Name = Cast<Type>(WidgetTree->FindWidget(TEXT(#Name)))
	MOTH_BIND(UWidget,HUD); MOTH_BIND(UWidget,Menu); MOTH_BIND(UWidget,TeachingPanel);
	MOTH_BIND(UTextBlock,HealthText); MOTH_BIND(UProgressBar,HealthBar);
	MOTH_BIND(UTextBlock,AmmoText); MOTH_BIND(UProgressBar,ReloadBar); MOTH_BIND(UTextBlock,WaveText);
	MOTH_BIND(UTextBlock,HeldText); MOTH_BIND(UMothDeviceIcon,HeldIcon);
	MOTH_BIND(UTextBlock,AimText); MOTH_BIND(UTextBlock,CrosshairText);
	MOTH_BIND(UTextBlock,TutorialText); MOTH_BIND(UTextBlock,TutorialStepText);
	MOTH_BIND(UTextBlock,NoticeText); MOTH_BIND(UWidget,NoticePanel); MOTH_BIND(UWidget,AimPanel);
	MOTH_BIND(UTextBlock,MenuTitle); MOTH_BIND(UTextBlock,MenuDescription);
	MOTH_BIND(UButton,PrimaryButton); MOTH_BIND(UTextBlock,PrimaryLabel);
	MOTH_BIND(UButton,SecondaryButton); MOTH_BIND(UTextBlock,SecondaryLabel);
	MOTH_BIND(UButton,QuitButton); MOTH_BIND(UButton,RetryButton);
#undef MOTH_BIND
}

void UMothHUDWidget::ResolveControls()
{
	bControlsReady = true;
	const UWidget* Controls[] = {HUD,Menu,TeachingPanel,HealthText,HealthBar,AmmoText,ReloadBar,WaveText,HeldText,HeldIcon,
		AimText,CrosshairText,TutorialText,TutorialStepText,NoticeText,NoticePanel,AimPanel,MenuTitle,MenuDescription,
		PrimaryButton,PrimaryLabel,SecondaryButton,SecondaryLabel,QuitButton,RetryButton};
	for (const UWidget* Control : Controls) bControlsReady &= IsValid(Control);
	if (bControlsReady)
	{
		DefaultAimColor = AimText->GetColorAndOpacity();
		DefaultCrosshairColor = CrosshairText->GetColorAndOpacity();
	}
	else UE_LOG(LogTemp, Error, TEXT("T18: %s is missing required Designer controls. Compile the Widget Blueprint to see BindWidget errors."), *GetName());
}

void UMothHUDWidget::NativePreConstruct()
{
	// CoreStyle's shared composite font is not serialized in a Designer asset.
	// Restore only a missing font source; retain all user-assigned fonts and styling.
	if (WidgetTree) WidgetTree->ForEachWidget([](UWidget* Widget)
	{
		if (auto* Text=Cast<UTextBlock>(Widget))
		{
			FSlateFontInfo Font=Text->GetFont();
			if (!Font.HasValidFont()) { Font.CompositeFont=FCoreStyle::GetDefaultFont(); Text->SetFont(Font); }
		}
	});
	ResolveControls(); bMenuRendered = false;
	if (IsDesignTime() && bControlsReady)
	{
		State.Health = State.MaxHealth = 100; State.Ammo = State.MagazineSize = 24;
		State.Wave = 1; State.Alive = 4; State.Pending = 6;
		State.Page = PreviewPage; State.Tutorial = PreviewTutorialStep;
		State.bHeld = bPreviewHeld; State.HeldKind = PreviewHeldKind;
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

int32 UMothHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Layer=Super::NativePaint(Args,Geometry,CullingRect,Elements,Layer,Style,bParentEnabled);
	const auto* Feedback=GetWorld()?GetWorld()->GetSubsystem<UMothFeedbackSubsystem>():nullptr;
	if (!bDrawDefaultCombatFeedback||IsDesignTime()||IsMenuVisible()||!Feedback) return Layer;
	const FVector2D Size=Geometry.GetLocalSize();
	const float Hit=Feedback->GetHitMarkerAlpha(), Damage=Feedback->GetDamageEdgeAlpha();
	if (Hit<=0&&Damage<=0) return Layer;
	++Layer;
	auto Line=[&](FVector2D A,FVector2D B,FLinearColor Color,float Width)
	{
		TArray<FVector2D> Points{A,B};
		FSlateDrawElement::MakeLines(Elements,Layer,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Width);
	};
	if (Hit>0)
	{
		FVector2D Center=Size*.5f;
		if (CrosshairText) { const auto& G=CrosshairText->GetCachedGeometry(); Center=Geometry.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()*.5f)); }
		for (int32 X : {-1,1}) for (int32 Y : {-1,1})
			Line(Center+FVector2D(X*9,Y*9),Center+FVector2D(X*15,Y*15),FLinearColor(1,1,1,Hit),2);
	}
	if (Damage>0)
	{
		const FLinearColor Red(1,.12f,.08f,Damage*.65f);
		Line(FVector2D(4,4),FVector2D(Size.X-4,4),Red,5);
		Line(FVector2D(4,4),FVector2D(4,Size.Y-4),Red,5);
		Line(FVector2D(Size.X-4,4),Size-FVector2D(4,4),Red,5);
		Line(FVector2D(4,Size.Y-4),Size-FVector2D(4,4),Red,5);
	}
	return Layer;
}

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
	Player.Reset(); Rifle.Reset(); Director.Reset(); Tutorial.Reset();
}

void UMothHUDWidget::BindSources()
{
	auto* NewPlayer=Cast<AMothEffectCharacter>(GetOwningPlayerPawn());
	auto* Mode=GetWorld()?Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode()):nullptr;
	auto* NewDirector=Mode?Mode->GetWaveDirector():nullptr;
	auto* NewRifle=NewPlayer?NewPlayer->GetRifle():nullptr;
	auto* NewTutorial=GetWorld()?GetWorld()->GetSubsystem<UMothTutorialSubsystem>():nullptr;
	if (Player.Get()==NewPlayer && Director.Get()==NewDirector && Rifle.Get()==NewRifle && Tutorial.Get()==NewTutorial) return;
	UnbindSources(); Player=NewPlayer; Director=NewDirector; Rifle=NewRifle; Tutorial=NewTutorial;
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
}

void UMothHUDWidget::RefreshState()
{
	if (bRendering || IsDesignTime()) return;
	TGuardValue<bool> Guard(bRendering,true); BindSources();
	State.Health=Player.IsValid()?Player->GetHealthComponent()->GetCurrentHealth():0;
	State.MaxHealth=Player.IsValid()?Player->GetHealthComponent()->GetMaxHealth():0;
	State.Ammo=Rifle.IsValid()?Rifle->GetAmmoInMagazine():0;
	State.MagazineSize=Rifle.IsValid()?Rifle->GetMagazineSize():0;
	State.bReloading=Rifle.IsValid()&&Rifle->IsReloading();
	State.bHeld=Player.IsValid()&&Player->IsCarryingDevice();
	if (State.bHeld) State.HeldKind=Player->GetHeldDevice()->GetDeviceKind();
	State.Wave=Director.IsValid()?Director->GetWaveNumber():0;
	State.Alive=Director.IsValid()?Director->GetAliveCount():0;
	State.Pending=Director.IsValid()?Director->GetPendingCount():0;
	State.Tutorial=Tutorial.IsValid()?Tutorial->GetStep():EMothTutorialStep::Inactive;
	const EMothFlowPhase Phase=Director.IsValid()?Director->GetPhase():EMothFlowPhase::Ready;
	if (Phase==EMothFlowPhase::Victory) State.Page=EMothUIPage::Victory;
	else if (Phase==EMothFlowPhase::Defeat) State.Page=EMothUIPage::Defeat;
	else if (GetWorld()&&GetWorld()->IsPaused()) State.Page=EMothUIPage::Pause;
	else if (Phase==EMothFlowPhase::Preparing||Phase==EMothFlowPhase::Fighting) State.Page=EMothUIPage::Gameplay;
	else if (State.Tutorial==EMothTutorialStep::Completed) State.Page=EMothUIPage::TutorialComplete;
	else if (Tutorial.IsValid()&&Tutorial->IsRunning()) State.Page=EMothUIPage::Tutorial;
	else State.Page=EMothUIPage::Title;
	if (bControlsReady) RenderState();
}

bool UMothHUDWidget::IsMenuVisible() const { return State.Page!=EMothUIPage::Gameplay&&State.Page!=EMothUIPage::Tutorial; }
UWidget* UMothHUDWidget::GetMenuFocusTarget() const { return PrimaryButton?static_cast<UWidget*>(PrimaryButton):const_cast<UMothHUDWidget*>(this); }

void UMothHUDWidget::RenderState()
{
	HUD->SetVisibility(IsMenuVisible()?ESlateVisibility::Collapsed:ESlateVisibility::SelfHitTestInvisible);
	Menu->SetVisibility(IsMenuVisible()?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
	HealthText->SetText(FText::FromString(FString::Printf(TEXT("生命  %.0f / %.0f"),State.Health,State.MaxHealth)));
	HealthBar->SetPercent(State.MaxHealth>0?State.Health/State.MaxHealth:0);
	AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%s  %02d / %02d"),State.bReloading?TEXT("换弹中"):TEXT("弹药"),State.Ammo,State.MagazineSize)));
	ReloadBar->SetVisibility(State.bReloading?ESlateVisibility::Visible:ESlateVisibility::Hidden);
	ReloadBar->SetPercent(Rifle.IsValid()?Rifle->GetReloadProgress():0);
	if (Director.IsValid()&&Director->GetPhase()==EMothFlowPhase::Preparing)
		WaveText->SetText(FText::FromString(FString::Printf(TEXT("第 %d / 3 波   ·   %.1f 秒后开始   ·   场内 %d / 待入场 %d"),State.Wave,Director->GetPreparationRemaining(),State.Alive,State.Pending)));
	else WaveText->SetText(FText::FromString(State.Page==EMothUIPage::Tutorial?TEXT("训练区   ·   射击启动 / 方向锁定 / 间接连锁"):
		FString::Printf(TEXT("第 %d / 3 波   ·   场内 %d / 待入场 %d"),State.Wave,State.Alive,State.Pending)));
	HeldIcon->SetVisibility(State.bHeld?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
	HeldIcon->SetDeviceKind(State.HeldKind);
	HeldText->SetText(FText::FromString(State.bHeld?UMothDeviceIcon::GetDeviceName(State.HeldKind).ToString()+TEXT("   左键投掷 / E 放下"):
		TEXT("空手   左键射击 / 对准道具按 E 拾取")));
	FString Aim; FSlateColor AimColor=DefaultAimColor, CrosshairColor=DefaultCrosshairColor;
	if (Player.IsValid()&&!State.bHeld)
	{
		FVector Origin,Direction;
		if (Player->GetRifleView(Origin,Direction))
		{
			FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(MothHUDDeviceAim),true,Player.Get());
			if (Rifle.IsValid()) Params.AddIgnoredActor(Rifle.Get());
			if (GetWorld()->LineTraceSingleByChannel(Hit,Origin,Origin+Direction*8000,
				Rifle.IsValid()?Rifle->GetWeaponTraceChannel():ECC_Visibility,Params))
				if (auto* Device=Cast<ADeviceBase>(Hit.GetActor()); Device&&Device->GetDeviceState()==EDeviceState::Dormant)
				{
					Aim=UMothDeviceIcon::GetDeviceName(Device->GetDeviceKind()).ToString()+TEXT(" · 射击启动");
					AimColor=UMothDeviceIcon::GetDeviceColor(Device->GetDeviceKind());
					CrosshairColor=UMothDeviceIcon::GetDeviceColor(Device->GetDeviceKind());
					if (Player->GetPickupCandidate()==Device) Aim+=TEXT(" / E 拾取");
				}
		}
	}
	AimText->SetText(FText::FromString(Aim));
	AimPanel->SetVisibility(Aim.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
	AimText->SetColorAndOpacity(AimColor); CrosshairText->SetColorAndOpacity(CrosshairColor);
	NoticeText->SetText(GetWorld()&&GetWorld()->GetTimeSeconds()<NoticeEndsAt?Notice:Tutorial.IsValid()?Tutorial->GetFeedback():FText::GetEmpty());
	NoticePanel->SetVisibility(NoticeText->GetText().IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
	const bool bTeaching=State.Page==EMothUIPage::Tutorial;
	TeachingPanel->SetVisibility(bTeaching?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
	if (IsDesignTime())
	{
		const auto* Config=GetTutorialConfig();
		TutorialText->SetText(State.Tutorial==EMothTutorialStep::BombAndLift?Config->BombAndLiftInstruction:
			State.Tutorial==EMothTutorialStep::AirborneEmitter?Config->AirborneInstruction:
			State.Tutorial==EMothTutorialStep::IndirectChain?Config->IndirectInstruction:FText::GetEmpty());
	}
	else TutorialText->SetText(Tutorial.IsValid()?Tutorial->GetInstruction():FText::GetEmpty());
	TutorialStepText->SetText(FText::FromString(FString::Printf(TEXT("教学  %d / 3   ·   实际完成操作后进入下一步"),static_cast<int32>(State.Tutorial))));
	if (!bMenuRendered || RenderedPage!=State.Page)
	{
		RenderedPage=State.Page; bMenuRendered=true;
		const bool bTitle=State.Page==EMothUIPage::Title;
		const bool bPause=State.Page==EMothUIPage::Pause;
		const bool bComplete=State.Page==EMothUIPage::TutorialComplete;
		const FMothMenuCopy& Copy=bTitle?TitleCopy:bPause?PauseCopy:bComplete?TutorialCompleteCopy:
			State.Page==EMothUIPage::Victory?VictoryCopy:DefeatCopy;
		MenuTitle->SetText(Copy.Title); MenuDescription->SetText(Copy.Description);
		PrimaryLabel->SetText(Copy.PrimaryAction); SecondaryLabel->SetText(Copy.SecondaryAction);
		SecondaryButton->SetVisibility(bTitle||bPause||bComplete?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
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
void UMothHUDWidget::HandleTutorial(EMothTutorialStep) { RefreshState(); }
void UMothHUDWidget::HandleEmpty(ARifle*) { ShowInteractionNotice(FText::FromString(TEXT("弹匣已空，按 R 换弹。"))); }
void UMothHUDWidget::HandleBlocked(ARifle*) { ShowInteractionNotice(FText::FromString(TEXT("枪口被遮挡，移开后再射击。"))); }

void UMothHUDWidget::PrimaryClicked()
{
	if (auto* Controller=Cast<AMothEffectPlayerController>(GetOwningPlayer()))
	{
		if (State.Page==EMothUIPage::Title) Controller->StartTeaching();
		else if (State.Page==EMothUIPage::Pause) Controller->ResumeGame();
		else if (State.Page==EMothUIPage::TutorialComplete) Controller->StartBattle();
		else if (State.Page==EMothUIPage::Victory||State.Page==EMothUIPage::Defeat) Controller->ReturnToTitle();
	}
}
void UMothHUDWidget::SecondaryClicked()
{
	if (auto* Controller=Cast<AMothEffectPlayerController>(GetOwningPlayer()))
		if (State.Page==EMothUIPage::Title) Controller->StartBattle(); else Controller->ReturnToTitle();
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
