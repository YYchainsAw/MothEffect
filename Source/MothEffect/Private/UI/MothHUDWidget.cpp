#include "UI/MothHUDWidget.h"
#include "UI/MothDeviceIcon.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HealthComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SafeZone.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Devices/DeviceBase.h"
#include "Engine/World.h"
#include "Framework/MothEffectGameMode.h"
#include "Framework/MothEffectPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Weapons/Rifle.h"

namespace MothUIStyle
{
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(20,40,45,244));
	const FLinearColor Slate = FLinearColor::FromSRGBColor(FColor(52,81,92));
	const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(242,239,224));
	const FLinearColor Brass = FLinearColor::FromSRGBColor(FColor(216,182,122));
	UTextBlock* Text(UWidgetTree* Tree, const TCHAR* Value, int Size = 24, FLinearColor Color = Paper)
	{
		auto* Widget = Tree->ConstructWidget<UTextBlock>();
		Widget->SetText(FText::FromString(Value));
		Widget->SetFont(FCoreStyle::GetDefaultFontStyle(Size>=34?"Bold":Size<=21?"BoldCondensed":"Regular", Size));
		Widget->SetColorAndOpacity(Color); Widget->SetAutoWrapText(true);
		return Widget;
	}
	UBorder* Panel(UWidgetTree* Tree, UWidget* Content, float Padding = 20)
	{
		auto* Widget = Tree->ConstructWidget<UBorder>();
		Widget->SetBrushColor(Ink); Widget->SetPadding(FMargin(Padding)); Widget->SetContent(Content); return Widget;
	}
	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Anchor, FVector2D Alignment, FVector2D Offset, FVector2D Size)
	{
		auto* Slot = Canvas->AddChildToCanvas(Widget); Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		Slot->SetAlignment(Alignment); Slot->SetPosition(Offset); Slot->SetSize(Size); return Slot;
	}
	void Row(UVerticalBox* Box, UWidget* Widget, float Gap = 8)
	{
		Box->AddChildToVerticalBox(Widget)->SetPadding(FMargin(0, 0, 0, Gap));
	}
	UButton* Button(UWidgetTree* Tree, const TCHAR* Label, UTextBlock*& OutLabel)
	{
		auto* Widget = Tree->ConstructWidget<UButton>();
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.SetResourceObject(nullptr); Style.Normal.DrawAs=ESlateBrushDrawType::Box; Style.Normal.TintColor=Slate;
		Style.Hovered=Style.Normal; Style.Hovered.TintColor=FLinearColor(.20f,.33f,.38f);
		Style.Pressed=Style.Normal; Style.Pressed.TintColor=FLinearColor(.10f,.16f,.19f);
		Style.NormalPadding=FMargin(22,14); Style.PressedPadding=FMargin(22,14);
		Widget->SetStyle(Style);
		OutLabel=Text(Tree, Label, 24); OutLabel->SetJustification(ETextJustify::Center); Widget->SetContent(OutLabel);
		return Widget;
	}
}

TSharedRef<SWidget> UMothHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildLayout();
	return Super::RebuildWidget();
}

void UMothHUDWidget::BuildLayout()
{
	using namespace MothUIStyle;
	SetIsFocusable(true);
	auto* Safe = WidgetTree->ConstructWidget<USafeZone>(); WidgetTree->RootWidget=Safe;
	auto* Root = WidgetTree->ConstructWidget<UOverlay>(); Safe->SetContent(Root);
	HUD=WidgetTree->ConstructWidget<UCanvasPanel>();
	auto* HUDSlot=Root->AddChildToOverlay(HUD); HUDSlot->SetHorizontalAlignment(HAlign_Fill); HUDSlot->SetVerticalAlignment(VAlign_Fill);
	HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	WaveText=Text(WidgetTree,TEXT(""),24,Brass);
	Place(HUD,Panel(WidgetTree,WaveText,14),{.5,0},{.5,0},{0,24},{740,76});
	auto* Vital=WidgetTree->ConstructWidget<UVerticalBox>();
	HealthText=Text(WidgetTree,TEXT(""),28); Row(Vital,HealthText);
	HealthBar=WidgetTree->ConstructWidget<UProgressBar>(); HealthBar->SetFillColorAndOpacity(Brass); Row(Vital,HealthBar);
	Row(Vital,Text(WidgetTree,TEXT("WASD 移动   SPACE 跳跃"),20));
	Place(HUD,Panel(WidgetTree,Vital),{0,1},{0,1},{24,-24},{380,152});
	auto* Weapon=WidgetTree->ConstructWidget<UVerticalBox>();
	AmmoText=Text(WidgetTree,TEXT(""),28); Row(Weapon,AmmoText);
	ReloadBar=WidgetTree->ConstructWidget<UProgressBar>(); ReloadBar->SetFillColorAndOpacity(Brass); Row(Weapon,ReloadBar);
	Row(Weapon,Text(WidgetTree,TEXT("R 换弹   ESC 暂停"),20));
	Place(HUD,Panel(WidgetTree,Weapon),{1,1},{1,1},{-24,-24},{380,152});
	auto* HeldRow=WidgetTree->ConstructWidget<UHorizontalBox>();
	HeldIcon=WidgetTree->ConstructWidget<UMothDeviceIcon>();
	HeldRow->AddChildToHorizontalBox(HeldIcon)->SetPadding(FMargin(0,0,12,0));
	HeldText=Text(WidgetTree,TEXT(""),22); HeldText->SetAutoWrapText(false); HeldText->SetWrapTextAt(560);
	HeldRow->AddChildToHorizontalBox(HeldText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Place(HUD,Panel(WidgetTree,HeldRow,14),{.5,1},{.5,1},{0,-24},{640,80});
	CrosshairText=Text(WidgetTree,TEXT("+"),28); CrosshairText->SetJustification(ETextJustify::Center);
	FFontOutlineSettings CrosshairOutline; CrosshairOutline.OutlineSize=1; CrosshairOutline.OutlineColor=Ink;
	CrosshairText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",28,CrosshairOutline));
	Place(HUD,CrosshairText,{.5,.5},{.5,.5},{0,0},{32,40});
	AimText=Text(WidgetTree,TEXT(""),20); AimText->SetJustification(ETextJustify::Center);
	AimText->SetAutoWrapText(false); AimText->SetWrapTextAt(624);
	AimPanel=Panel(WidgetTree,AimText,8);
	auto* AimWidth=WidgetTree->ConstructWidget<USizeBox>(); AimWidth->SetWidthOverride(640); AimWidth->SetContent(AimPanel);
	Place(HUD,AimWidth,{.5,.5},{.5,0},{0,28},{640,60})->SetAutoSize(true);
	NoticeText=Text(WidgetTree,TEXT(""),22,Brass); NoticeText->SetJustification(ETextJustify::Center);
	NoticeText->SetAutoWrapText(false); NoticeText->SetWrapTextAt(736);
	NoticePanel=Panel(WidgetTree,NoticeText,12);
	auto* NoticeWidth=WidgetTree->ConstructWidget<USizeBox>(); NoticeWidth->SetWidthOverride(760); NoticeWidth->SetContent(NoticePanel);
	Place(HUD,NoticeWidth,{.5,.5},{.5,0},{0,-140},{760,72})->SetAutoSize(true);
	auto* Teaching=WidgetTree->ConstructWidget<UVerticalBox>();
	TutorialStepText=Text(WidgetTree,TEXT(""),22,Brass); Row(Teaching,TutorialStepText);
	TutorialText=Text(WidgetTree,TEXT(""),22); TutorialText->SetAutoWrapText(false); TutorialText->SetWrapTextAt(760); Row(Teaching,TutorialText);
	UTextBlock* RetryLabel=nullptr;
	RetryButton=Button(WidgetTree,TEXT("重置本步道具"),RetryLabel); Row(Teaching,RetryButton,0);
	RetryLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",20));
	RetryButton->OnClicked.AddUniqueDynamic(this,&UMothHUDWidget::RetryClicked);
	auto* TeachingPanel=Panel(WidgetTree,Teaching);
	TeachingPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	// Text and retry visibility drive the panel's visibility in RenderState.
	auto* TeachingWidth=WidgetTree->ConstructWidget<USizeBox>(); TeachingWidth->SetWidthOverride(800); TeachingWidth->SetContent(TeachingPanel);
	Place(HUD,TeachingWidth,{.5,1},{.5,1},{0,-195},{800,258})->SetAutoSize(true);

	MenuContent=WidgetTree->ConstructWidget<UVerticalBox>();
	Row(MenuContent,Text(WidgetTree,TEXT("MOTH EFFECT"),46,Brass),8);
	MenuTitle=Text(WidgetTree,TEXT("飞蛾效应"),34); Row(MenuContent,MenuTitle,18);
	MenuDescription=Text(WidgetTree,TEXT(""),24); Row(MenuContent,MenuDescription,22);
	auto* IconRow=WidgetTree->ConstructWidget<UHorizontalBox>();
	for (EDeviceKind Kind : {EDeviceKind::Launcher,EDeviceKind::Bomb,EDeviceKind::Emitter})
	{
		auto* Icon=WidgetTree->ConstructWidget<UMothDeviceIcon>(); Icon->SetDeviceKind(Kind);
		IconRow->AddChildToHorizontalBox(Icon)->SetPadding(FMargin(0,0,8,0));
		auto* Name=Text(WidgetTree,*UMothDeviceIcon::GetDeviceName(Kind).ToString(),21,UMothDeviceIcon::GetDeviceColor(Kind));
		IconRow->AddChildToHorizontalBox(Name)->SetPadding(FMargin(0,5,24,0));
	}
	Row(MenuContent,IconRow,24);
	UTextBlock* PrimaryText=nullptr; UTextBlock* SecondaryText=nullptr;
	PrimaryButton=Button(WidgetTree,TEXT("开始教学"),PrimaryText); PrimaryLabel=PrimaryText; Row(MenuContent,PrimaryButton,12);
	SecondaryButton=Button(WidgetTree,TEXT("直接进入竞技场"),SecondaryText); SecondaryLabel=SecondaryText; Row(MenuContent,SecondaryButton,12);
	UTextBlock* QuitLabel=nullptr; QuitButton=Button(WidgetTree,TEXT("退出游戏"),QuitLabel); Row(MenuContent,QuitButton,0);
	PrimaryButton->OnClicked.AddUniqueDynamic(this,&UMothHUDWidget::PrimaryClicked);
	SecondaryButton->OnClicked.AddUniqueDynamic(this,&UMothHUDWidget::SecondaryClicked);
	QuitButton->OnClicked.AddUniqueDynamic(this,&UMothHUDWidget::QuitClicked);
	auto* Width=WidgetTree->ConstructWidget<USizeBox>(); Width->SetWidthOverride(700); Width->SetContent(Panel(WidgetTree,MenuContent,32));
	auto* MenuOverlay=WidgetTree->ConstructWidget<UOverlay>();
	auto* ContentSlot=MenuOverlay->AddChildToOverlay(Width); ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
	Menu=Panel(WidgetTree,MenuOverlay,24); Menu->SetBrushColor(FLinearColor(.008f,.018f,.022f,.88f));
	auto* MenuSlot=Root->AddChildToOverlay(Menu); MenuSlot->SetHorizontalAlignment(HAlign_Fill); MenuSlot->SetVerticalAlignment(VAlign_Fill);
}

void UMothHUDWidget::NativeConstruct() { Super::NativeConstruct(); RefreshState(); }
void UMothHUDWidget::NativeDestruct() { UnbindSources(); Super::NativeDestruct(); }
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
	if (bRendering) return;
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
	if (HUD) RenderState();
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
	FString Aim; FLinearColor AimColor=MothUIStyle::Paper;
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
					if (Player->GetPickupCandidate()==Device) Aim+=TEXT(" / E 拾取");
				}
		}
	}
	AimText->SetText(FText::FromString(Aim));
	AimPanel->SetVisibility(Aim.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
	AimText->SetColorAndOpacity(AimColor); CrosshairText->SetColorAndOpacity(AimColor);
	NoticeText->SetText(GetWorld()->GetTimeSeconds()<NoticeEndsAt?Notice:Tutorial.IsValid()?Tutorial->GetFeedback():FText::GetEmpty());
	NoticePanel->SetVisibility(NoticeText->GetText().IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
	const bool bTeaching=State.Page==EMothUIPage::Tutorial;
	TutorialText->GetParent()->GetParent()->GetParent()->SetVisibility(bTeaching?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
	TutorialText->SetText(Tutorial.IsValid()?Tutorial->GetInstruction():FText::GetEmpty());
	TutorialStepText->SetText(FText::FromString(FString::Printf(TEXT("教学  %d / 3   ·   实际完成操作后进入下一步"),static_cast<int32>(State.Tutorial))));
	if (RenderedPage!=State.Page)
	{
		RenderedPage=State.Page;
		const bool bTitle=State.Page==EMothUIPage::Title;
		const bool bPause=State.Page==EMothUIPage::Pause;
		const bool bComplete=State.Page==EMothUIPage::TutorialComplete;
		MenuTitle->SetText(FText::FromString(bTitle?TEXT("飞蛾效应"):bPause?TEXT("暂停"):bComplete?TEXT("教学完成"):
			State.Page==EMothUIPage::Victory?TEXT("竞技场胜利"):TEXT("本次挑战结束")));
		MenuDescription->SetText(FText::FromString(bTitle?TEXT("击中启动，方向锁定，机关互相接续。\n先学习三种机关，再迎战三波敌人。"):
			bPause?TEXT("继续后回到当前战斗。\n投掷后仍需松开左键，再重新按下才能射击。"):
			bComplete?TEXT("你已完成抬升、空中启动和间接连锁。\n进入竞技场时生命与弹药恢复，教学道具清空。"):
			State.Page==EMothUIPage::Victory?TEXT("三波敌人已清除。\n重新开始可返回入口，再次训练或挑战。"):
			TEXT("生命耗尽或离开了竞技场。\n重新开始，尝试用连锁改变战局。")));
		PrimaryLabel->SetText(FText::FromString(bTitle?TEXT("开始教学"):bPause?TEXT("继续游戏"):bComplete?TEXT("开始战斗"):TEXT("重新开始")));
		SecondaryLabel->SetText(FText::FromString(bTitle?TEXT("直接进入竞技场"):TEXT("返回入口")));
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
