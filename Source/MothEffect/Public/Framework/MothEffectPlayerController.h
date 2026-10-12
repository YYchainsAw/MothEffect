// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MothEffectPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UMothHUDWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AMothEffectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMothEffectPlayerController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	UFUNCTION(BlueprintCallable, Category="Moth|UI") void StartTeaching();
	UFUNCTION(BlueprintCallable, Category="Moth|UI") void StartBattle();
	UFUNCTION(BlueprintCallable, Category="Moth|UI") void ToggleMenuPause();
	UFUNCTION(BlueprintCallable, Category="Moth|UI") void ResumeGame();
	UFUNCTION(BlueprintCallable, Category="Moth|UI") void ReturnToTitle();
	UMothHUDWidget* GetGameplayUI() const { return GameplayUI; }
	void SyncUIInput();

	virtual void FlushPressedKeys() override;
	virtual bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause()) override;
	/** Development-only console helpers; do not form part of the shipping player flow. */
	UFUNCTION(Exec)
	void MothLifecycleStats();
	UFUNCTION(Exec)
	void MothSpawnDevice(int32 Kind = 2, int32 Count = 1);
	UFUNCTION(Exec)
	void MothResetDevices();
	UFUNCTION(Exec)
	void MothSpawnEnemy();
	UFUNCTION(Exec)
	void MothSpawnDropEnemy(int32 Kind);
	UFUNCTION(Exec)
	void MothResetEnemies();
	UFUNCTION(Exec)
	void MothEnemyStats();
	UFUNCTION(Exec)
	void MothFlowStats();
	UFUNCTION(Exec)
	void MothStartEncounter();
	UFUNCTION(Exec)
	void MothRestartEncounter();
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	UPROPERTY() TObjectPtr<UMothHUDWidget> GameplayUI;
	UPROPERTY(EditDefaultsOnly, Category="Moth|UI") TSoftClassPtr<UMothHUDWidget> GameplayUIClass;
	bool bTravelRequested = false;
	int32 AppliedUIPage = -1;
	void InitializeGameplayUI();

};
