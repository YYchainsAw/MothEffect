#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/MothWaveDirector.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "Types/PlayerActionState.h"
#include "MothHUDWidget.generated.h"

class UTextBlock; class UProgressBar; class UMothTutorialConfig;
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

USTRUCT(BlueprintType)
struct FMothMenuCopy
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText PrimaryAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText SecondaryAction;
};

/** Designer owns presentation; native bindings own state and button actions. */
UCLASS(Blueprintable)
class MOTHEFFECT_API UMothHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UMothHUDWidget(const FObjectInitializer& ObjectInitializer);
	void RefreshState();
	UFUNCTION(BlueprintPure, Category="Moth|UI") FMothHUDState GetHUDState() const { return State; }
	UMothTutorialConfig* GetTutorialConfig() const;
	UWidget* GetMenuFocusTarget() const;
	bool IsMenuVisible() const;
	void ShowInteractionNotice(const FText& Message);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
	FMothHUDState State;
	TWeakObjectPtr<AMothEffectCharacter> Player;
	TWeakObjectPtr<ARifle> Rifle;
	TWeakObjectPtr<AMothWaveDirector> Director;
	TWeakObjectPtr<UMothTutorialSubsystem> Tutorial;
	bool bControlsReady = false;
	FSlateColor DefaultAimColor, DefaultCrosshairColor;
protected:
	/** Disable when replacing these two screen cues in your Widget Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Moth|Feedback") bool bDrawDefaultCombatFeedback = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moth|Teaching") TSoftObjectPtr<UMothTutorialConfig> TutorialConfig;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Moth|Menu Copy") FMothMenuCopy TitleCopy;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Moth|Menu Copy") FMothMenuCopy PauseCopy;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Moth|Menu Copy") FMothMenuCopy TutorialCompleteCopy;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Moth|Menu Copy") FMothMenuCopy VictoryCopy;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Moth|Menu Copy") FMothMenuCopy DefeatCopy;
	/** Preview only: never alters a gameplay world or starts a tutorial. */
	UPROPERTY(EditAnywhere, Category="Moth|Designer Preview") EMothUIPage PreviewPage = EMothUIPage::Title;
	UPROPERTY(EditAnywhere, Category="Moth|Designer Preview") EMothTutorialStep PreviewTutorialStep = EMothTutorialStep::BombAndLift;
	UPROPERTY(EditAnywhere, Category="Moth|Designer Preview") bool bPreviewHeld = false;
	UPROPERTY(EditAnywhere, Category="Moth|Designer Preview") EDeviceKind PreviewHeldKind = EDeviceKind::Emitter;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UWidget> HUD;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UWidget> Menu;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UWidget> TeachingPanel;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> AmmoText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UProgressBar> ReloadBar;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> WaveText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> HeldText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UMothDeviceIcon> HeldIcon;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> AimText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> CrosshairText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> TutorialText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> NoticeText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UWidget> NoticePanel;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UWidget> AimPanel;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> MenuTitle;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> MenuDescription;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UButton> PrimaryButton;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> PrimaryLabel;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UButton> SecondaryButton;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> SecondaryLabel;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UButton> QuitButton;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UButton> RetryButton;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> TutorialStepText;
private:
	FText Notice;
	double NoticeEndsAt = 0;
	EMothUIPage RenderedPage = EMothUIPage::Defeat;
	bool bMenuRendered = false;
	bool bRendering = false;
	void BuildLayout();
	void ResolveControls();
	void BindButtons(bool bBind);
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
