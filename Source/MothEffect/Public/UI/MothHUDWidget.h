#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/MothWaveDirector.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "Types/PlayerActionState.h"
#include "MothHUDWidget.generated.h"

class UTextBlock; class UProgressBar; class UVerticalBox; class UBorder; class UCanvasPanel;
class UButton; class UMothDeviceIcon; class AMothEffectCharacter; class ARifle; class UHealthComponent;

UENUM(BlueprintType)
enum class EMothUIPage : uint8 { Title, Gameplay, Tutorial, Pause, TutorialComplete, Victory, Defeat };

USTRUCT(BlueprintType)
struct FMothHUDState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) float Health = 0;
	UPROPERTY(BlueprintReadOnly) float MaxHealth = 0;
	UPROPERTY(BlueprintReadOnly) int32 Ammo = 0;
	UPROPERTY(BlueprintReadOnly) int32 MagazineSize = 0;
	UPROPERTY(BlueprintReadOnly) bool bReloading = false;
	UPROPERTY(BlueprintReadOnly) bool bHeld = false;
	UPROPERTY(BlueprintReadOnly) EDeviceKind HeldKind = EDeviceKind::Launcher;
	UPROPERTY(BlueprintReadOnly) int32 Wave = 0;
	UPROPERTY(BlueprintReadOnly) int32 Alive = 0;
	UPROPERTY(BlueprintReadOnly) int32 Pending = 0;
	UPROPERTY(BlueprintReadOnly) EMothUIPage Page = EMothUIPage::Title;
	UPROPERTY(BlueprintReadOnly) EMothTutorialStep Tutorial = EMothTutorialStep::Inactive;
};

/** Native UMG layout, optional saved Widget Blueprint entry. Getters initialise, signals keep it current. */
UCLASS(Blueprintable)
class MOTHEFFECT_API UMothHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void RefreshState();
	const FMothHUDState& GetHUDState() const { return State; }
	UWidget* GetMenuFocusTarget() const;
	bool IsMenuVisible() const;
	void ShowInteractionNotice(const FText& Message);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
	FMothHUDState State;
	TWeakObjectPtr<AMothEffectCharacter> Player;
	TWeakObjectPtr<ARifle> Rifle;
	TWeakObjectPtr<AMothWaveDirector> Director;
	TWeakObjectPtr<UMothTutorialSubsystem> Tutorial;
	UPROPERTY() TObjectPtr<UCanvasPanel> HUD;
	UPROPERTY() TObjectPtr<UBorder> Menu;
	UPROPERTY() TObjectPtr<UVerticalBox> MenuContent;
	UPROPERTY() TObjectPtr<UTextBlock> HealthText;
	UPROPERTY() TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY() TObjectPtr<UTextBlock> AmmoText;
	UPROPERTY() TObjectPtr<UProgressBar> ReloadBar;
	UPROPERTY() TObjectPtr<UTextBlock> WaveText;
	UPROPERTY() TObjectPtr<UTextBlock> HeldText;
	UPROPERTY() TObjectPtr<UMothDeviceIcon> HeldIcon;
	UPROPERTY() TObjectPtr<UTextBlock> AimText;
	UPROPERTY() TObjectPtr<UTextBlock> TutorialText;
	UPROPERTY() TObjectPtr<UTextBlock> NoticeText;
	UPROPERTY() TObjectPtr<UTextBlock> MenuTitle;
	UPROPERTY() TObjectPtr<UTextBlock> MenuDescription;
	UPROPERTY() TObjectPtr<UButton> PrimaryButton;
	UPROPERTY() TObjectPtr<UTextBlock> PrimaryLabel;
	UPROPERTY() TObjectPtr<UButton> SecondaryButton;
	UPROPERTY() TObjectPtr<UTextBlock> SecondaryLabel;
	UPROPERTY() TObjectPtr<UButton> QuitButton;
	UPROPERTY() TObjectPtr<UButton> RetryButton;
	UPROPERTY() TObjectPtr<UTextBlock> TutorialStepText;
	FText Notice;
	double NoticeEndsAt = 0;
	EMothUIPage RenderedPage = EMothUIPage::Defeat;
	bool bRendering = false;
	void BuildLayout();
	void BindSources();
	void UnbindSources();
	void RenderState();
	UFUNCTION() void HandleHealth(float Current, float Max);
	UFUNCTION() void HandleAmmo(int32 Current, int32 Max);
	UFUNCTION() void HandleAction(EPlayerActionState Old, EPlayerActionState New);
	UFUNCTION() void HandleFlow(EMothFlowPhase Phase, int32 Wave, int32 Alive, int32 Pending);
	UFUNCTION() void HandleTutorial(EMothTutorialStep Step);
	UFUNCTION() void HandleEmpty(ARifle* Source);
	UFUNCTION() void HandleBlocked(ARifle* Source);
	UFUNCTION() void PrimaryClicked();
	UFUNCTION() void SecondaryClicked();
	UFUNCTION() void QuitClicked();
	UFUNCTION() void RetryClicked();
};
