#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/MothWaveDirector.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "Types/PlayerActionState.h"
#include "Types/EnemyHitFeedback.h"
#include "MothHUDWidget.generated.h"

class UTextBlock; class UProgressBar; class UMothTutorialConfig;
class UButton; class UMothDeviceIcon; class AMothEffectCharacter; class ARifle; class UHealthComponent;
class UMothFeedbackSubsystem; class APawn;

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
	UPROPERTY(BlueprintReadOnly) float ReloadProgress = 0;
	UPROPERTY(BlueprintReadOnly) bool bHeld = false;
	UPROPERTY(BlueprintReadOnly) EDeviceKind HeldKind = EDeviceKind::Launcher;
	UPROPERTY(BlueprintReadOnly) int32 Wave = 0;
	UPROPERTY(BlueprintReadOnly) int32 Alive = 0;
	UPROPERTY(BlueprintReadOnly) int32 Pending = 0;
	UPROPERTY(BlueprintReadOnly) float PreparationRemaining = 0;
	UPROPERTY(BlueprintReadOnly) bool bAimingAtDevice = false;
	UPROPERTY(BlueprintReadOnly) bool bCanPickup = false;
	UPROPERTY(BlueprintReadOnly) EDeviceKind AimedKind = EDeviceKind::Launcher;
	UPROPERTY(BlueprintReadOnly) FText InteractionPrompt;
	UPROPERTY(BlueprintReadOnly) FText Notice;
	UPROPERTY(BlueprintReadOnly) float DamageEdgeAlpha = 0;
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
	/** One accepted enemy damage result for this HUD's player; a kill replaces a normal hit. */
	UFUNCTION(BlueprintImplementableEvent, Category="Moth|Crosshair")
	void OnEnemyHitConfirmed(const FMothEnemyHitFeedback& Feedback);
	/** Clear user-owned crosshair animations/timers when the player/session changes. */
	UFUNCTION(BlueprintImplementableEvent, Category="Moth|Crosshair")
	void OnCrosshairFeedbackReset();
	/** Pause or resume user-owned widget animations without resetting their progress. */
	UFUNCTION(BlueprintImplementableEvent, Category="Moth|Crosshair")
	void OnCrosshairPauseChanged(bool bPaused);
	/** Hide the base crosshair on menus; a final kill can finish over the victory page. */
	UFUNCTION(BlueprintImplementableEvent, Category="Moth|Crosshair")
	void OnCrosshairPageChanged(EMothUIPage Page);
protected:
	virtual void NativePreConstruct() override;
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
	TWeakObjectPtr<UMothFeedbackSubsystem> CombatFeedback;
	bool bCrosshairPageAnnounced = false;
	bool bCrosshairPaused = false;
protected:
	/** Serialized compatibility only. All combat visuals belong to the Designer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Moth|Feedback", meta=(DeprecatedProperty, DeprecationMessage="Use Designer feedback widgets.")) bool bDrawDefaultCombatFeedback = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Moth|Feedback", meta=(DeprecatedProperty, DeprecationMessage="Use your crosshair Widget Blueprint.")) bool bDrawDefaultHitMarker = false;
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
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> WaveDetailText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidget)) TObjectPtr<UTextBlock> HeldText;
	/** Optional legacy fields retained for existing Blueprint graph references; never updated natively. */
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidgetOptional)) TObjectPtr<UMothDeviceIcon> HeldIcon;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> AimText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CrosshairText;
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> InteractionText;
	/** Designer supplies the texture, tint and position; native state supplies only its opacity. */
	UPROPERTY(BlueprintReadOnly, Category="Moth|Bindings", meta=(BindWidgetOptional)) TObjectPtr<UWidget> DamageOverlay;
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
	void BindButtons(bool bBind);
	void BindSources();
	void UnbindSources();
	void HandleEnemyHitConfirmed(APawn* InstigatorPawn, const FMothEnemyHitFeedback& Feedback);
	void RefreshInteractionState();
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
