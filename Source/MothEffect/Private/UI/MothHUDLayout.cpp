#include "MothHUDLayout.h"
#include "UI/MothDeviceIcon.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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
#include "Styling/CoreStyle.h"


namespace MothUIStyle
{
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(20,40,45,244));
	const FLinearColor Slate = FLinearColor::FromSRGBColor(FColor(52,81,92));
	const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(242,239,224));
	const FLinearColor Brass = FLinearColor::FromSRGBColor(FColor(216,182,122));
	UTextBlock* Text(UWidgetTree* Tree, const TCHAR* Value, int Size = 24, FLinearColor Color = Paper, FName Name = NAME_None)
	{
		auto* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Widget->SetText(FText::FromString(Value));
		Widget->SetFont(FCoreStyle::GetDefaultFontStyle(Size>=34?"Bold":Size<=21?"BoldCondensed":"Regular", Size));
		Widget->SetColorAndOpacity(Color); Widget->SetAutoWrapText(false);
		return Widget;
	}
	UBorder* Panel(UWidgetTree* Tree, UWidget* Content, float Padding = 20, FName Name = NAME_None)
	{
		auto* Widget = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
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
	UButton* Button(UWidgetTree* Tree, const TCHAR* Label, UTextBlock*& OutLabel, FName Name, FName LabelName)
	{
		auto* Widget = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.SetResourceObject(nullptr); Style.Normal.DrawAs=ESlateBrushDrawType::Box; Style.Normal.TintColor=Slate;
		Style.Hovered=Style.Normal; Style.Hovered.TintColor=FLinearColor(.20f,.33f,.38f);
		Style.Pressed=Style.Normal; Style.Pressed.TintColor=FLinearColor(.10f,.16f,.19f);
		Style.NormalPadding=FMargin(22,14); Style.PressedPadding=FMargin(22,14);
		Widget->SetStyle(Style);
		OutLabel=Text(Tree, Label, 24, Paper, LabelName); OutLabel->SetJustification(ETextJustify::Center); Widget->SetContent(OutLabel);
		return Widget;
	}
}

void MothHUDLayout::Build(UWidgetTree* Tree)
{
	using namespace MothUIStyle;
	auto* Safe = Tree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("SafeZone")); Tree->RootWidget=Safe;
	auto* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay")); Safe->SetContent(Root);
	auto* HUD=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUD"));
	auto* HUDSlot=Root->AddChildToOverlay(HUD); HUDSlot->SetHorizontalAlignment(HAlign_Fill); HUDSlot->SetVerticalAlignment(VAlign_Fill);
	HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	auto* WaveText=Text(Tree,TEXT("第 1 / 3 波   ·   场内 4 / 待入场 6"),24,Brass,TEXT("WaveText"));
	Place(HUD,Panel(Tree,WaveText,14,TEXT("WavePanel")),{.5,0},{.5,0},{0,24},{740,76});
	auto* Vital=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VitalContent"));
	auto* HealthText=Text(Tree,TEXT("生命  100 / 100"),28,Paper,TEXT("HealthText")); Row(Vital,HealthText);
	auto* HealthBar=Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar")); HealthBar->SetFillColorAndOpacity(Brass); HealthBar->SetPercent(1); Row(Vital,HealthBar);
	Row(Vital,Text(Tree,TEXT("WASD 移动   SPACE 跳跃"),20,Paper,TEXT("MovementHint")));
	Place(HUD,Panel(Tree,Vital,20,TEXT("VitalPanel")),{0,1},{0,1},{24,-24},{380,152});
	auto* Weapon=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeaponContent"));
	auto* AmmoText=Text(Tree,TEXT("弹药  24 / 24"),28,Paper,TEXT("AmmoText")); Row(Weapon,AmmoText);
	auto* ReloadBar=Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ReloadBar")); ReloadBar->SetFillColorAndOpacity(Brass); Row(Weapon,ReloadBar);
	Row(Weapon,Text(Tree,TEXT("R 换弹   ESC 暂停"),20,Paper,TEXT("ReloadHint")));
	Place(HUD,Panel(Tree,Weapon,20,TEXT("WeaponPanel")),{1,1},{1,1},{-24,-24},{380,152});
	auto* HeldRow=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeldContent"));
	auto* HeldIcon=Tree->ConstructWidget<UMothDeviceIcon>(UMothDeviceIcon::StaticClass(), TEXT("HeldIcon"));
	HeldRow->AddChildToHorizontalBox(HeldIcon)->SetPadding(FMargin(0,0,12,0));
	auto* HeldText=Text(Tree,TEXT("空手   左键射击 / 对准道具按 E 拾取"),22,Paper,TEXT("HeldText")); HeldText->SetAutoWrapText(false); HeldText->SetWrapTextAt(560);
	HeldRow->AddChildToHorizontalBox(HeldText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Place(HUD,Panel(Tree,HeldRow,14,TEXT("HeldPanel")),{.5,1},{.5,1},{0,-24},{640,80});
	auto* CrosshairText=Text(Tree,TEXT("+"),28,Paper,TEXT("CrosshairText")); CrosshairText->SetJustification(ETextJustify::Center);
	FFontOutlineSettings CrosshairOutline; CrosshairOutline.OutlineSize=1; CrosshairOutline.OutlineColor=Ink;
	CrosshairText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",28,CrosshairOutline));
	Place(HUD,CrosshairText,{.5,.5},{.5,.5},{0,0},{32,40});
	auto* AimText=Text(Tree,TEXT("发射器 · 射击启动 / E 拾取"),20,Paper,TEXT("AimText")); AimText->SetJustification(ETextJustify::Center);
	AimText->SetAutoWrapText(false); AimText->SetWrapTextAt(624);
	auto* AimPanel=Panel(Tree,AimText,8,TEXT("AimPanel"));
	auto* AimWidth=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AimWidth")); AimWidth->SetWidthOverride(640); AimWidth->SetContent(AimPanel);
	Place(HUD,AimWidth,{.5,.5},{.5,0},{0,28},{640,60})->SetAutoSize(true);
	auto* NoticeText=Text(Tree,TEXT("已补充发射器；松开左键后，再试一次。"),22,Brass,TEXT("NoticeText")); NoticeText->SetJustification(ETextJustify::Center);
	NoticeText->SetAutoWrapText(false); NoticeText->SetWrapTextAt(736);
	auto* NoticePanel=Panel(Tree,NoticeText,12,TEXT("NoticePanel"));
	auto* NoticeWidth=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NoticeWidth")); NoticeWidth->SetWidthOverride(760); NoticeWidth->SetContent(NoticePanel);
	Place(HUD,NoticeWidth,{.5,.5},{.5,0},{0,-140},{760,72})->SetAutoSize(true);
	auto* Teaching=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TeachingContent"));
	auto* TutorialStepText=Text(Tree,TEXT("教学  1 / 3   ·   实际完成操作后进入下一步"),22,Brass,TEXT("TutorialStepText")); Row(Teaching,TutorialStepText);
	auto* TutorialText=Text(Tree,TEXT("射击橙色爆裂器，炸毁方块；再射击青色抬升器，把箱子抬起。"),22,Paper,TEXT("TutorialText")); TutorialText->SetAutoWrapText(false); TutorialText->SetWrapTextAt(760); Row(Teaching,TutorialText);
	UTextBlock* RetryLabel=nullptr;
	auto* RetryButton=Button(Tree,TEXT("重置本步道具"),RetryLabel,TEXT("RetryButton"),TEXT("RetryLabel")); Row(Teaching,RetryButton,0);
	RetryLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",20));
	auto* TeachingPanel=Panel(Tree,Teaching,20,TEXT("TeachingPanel"));
	TeachingPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	// The named TeachingPanel can move freely in Designer.
	auto* TeachingWidth=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TeachingWidth")); TeachingWidth->SetWidthOverride(800); TeachingWidth->SetContent(TeachingPanel);
	Place(HUD,TeachingWidth,{.5,1},{.5,1},{0,-195},{800,258})->SetAutoSize(true);

	auto* MenuContent=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContent"));
	Row(MenuContent,Text(Tree,TEXT("MOTH EFFECT"),46,Brass,TEXT("BrandText")),8);
	auto* MenuTitle=Text(Tree,TEXT("飞蛾效应"),34,Paper,TEXT("MenuTitle")); Row(MenuContent,MenuTitle,18);
	auto* MenuDescription=Text(Tree,TEXT("击中启动，方向锁定，机关互相接续。"),24,Paper,TEXT("MenuDescription")); MenuDescription->SetWrapTextAt(636); Row(MenuContent,MenuDescription,22);
	auto* IconRow=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DeviceLegend"));
	for (EDeviceKind Kind : {EDeviceKind::Launcher,EDeviceKind::Bomb,EDeviceKind::Emitter})
	{
		auto* Icon=Tree->ConstructWidget<UMothDeviceIcon>(UMothDeviceIcon::StaticClass(), FName(*FString::Printf(TEXT("LegendIcon_%d"),static_cast<int32>(Kind)))); Icon->SetDeviceKind(Kind);
		IconRow->AddChildToHorizontalBox(Icon)->SetPadding(FMargin(0,0,8,0));
		auto* Name=Text(Tree,*UMothDeviceIcon::GetDeviceName(Kind).ToString(),21,UMothDeviceIcon::GetDeviceColor(Kind),FName(*FString::Printf(TEXT("LegendLabel_%d"),static_cast<int32>(Kind))));
		IconRow->AddChildToHorizontalBox(Name)->SetPadding(FMargin(0,5,24,0));
	}
	Row(MenuContent,IconRow,24);
	UTextBlock* PrimaryText=nullptr; UTextBlock* SecondaryText=nullptr;
	auto* PrimaryButton=Button(Tree,TEXT("开始教学"),PrimaryText,TEXT("PrimaryButton"),TEXT("PrimaryLabel")); Row(MenuContent,PrimaryButton,12);
	auto* SecondaryButton=Button(Tree,TEXT("直接进入竞技场"),SecondaryText,TEXT("SecondaryButton"),TEXT("SecondaryLabel")); Row(MenuContent,SecondaryButton,12);
	UTextBlock* QuitLabel=nullptr; auto* QuitButton=Button(Tree,TEXT("退出游戏"),QuitLabel,TEXT("QuitButton"),TEXT("QuitLabel")); Row(MenuContent,QuitButton,0);
	auto* Width=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuWidth")); Width->SetWidthOverride(700); Width->SetContent(Panel(Tree,MenuContent,32,TEXT("MenuCard")));
	auto* MenuOverlay=Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MenuAlignment"));
	auto* ContentSlot=MenuOverlay->AddChildToOverlay(Width); ContentSlot->SetHorizontalAlignment(HAlign_Center); ContentSlot->SetVerticalAlignment(VAlign_Center);
	auto* Menu=Panel(Tree,MenuOverlay,24,TEXT("Menu")); Menu->SetBrushColor(FLinearColor(.008f,.018f,.022f,.88f));
	auto* MenuSlot=Root->AddChildToOverlay(Menu); MenuSlot->SetHorizontalAlignment(HAlign_Fill); MenuSlot->SetVerticalAlignment(VAlign_Fill);
}
