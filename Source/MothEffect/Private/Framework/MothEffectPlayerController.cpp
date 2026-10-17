// Copyright Epic Games, Inc. All Rights Reserved.


#include "Framework/MothEffectPlayerController.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "Framework/MothEffectGameMode.h"
#include "Framework/MothMainMenuGameMode.h"
#include "Framework/MothWaveDirector.h"
#include "Characters/MothEffectCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "MothEffect.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Devices/DeviceBase.h"
#include "Engine/Engine.h"
#include "AI/EnemyBase.h"
#include "EngineUtils.h"
#include "Components/HealthComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "Framework/MothGameInstance.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "UI/MothHUDWidget.h"
#include "UI/MothLoadingSubsystem.h"
#include "Weapons/Rifle.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Settings/MothUserSettings.h"
#include "Kismet/GameplayStatics.h"

AMothEffectPlayerController::AMothEffectPlayerController()
{
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
	GameplayUIClass = TSoftClassPtr<UMothHUDWidget>(FSoftObjectPath(TEXT("/Game/MothEffect/UI/WBP_MothHUD.WBP_MothHUD_C")));
}

void AMothEffectPlayerController::InitializeGameplayUI()
{
	if (bTravelRequested || GameplayUI || !IsLocalPlayerController() || !GetWorld()
		|| (GetWorld()->WorldType != EWorldType::Game && GetWorld()->WorldType != EWorldType::PIE)) return;
	const bool bMainMenu = Cast<AMothMainMenuGameMode>(GetWorld()->GetAuthGameMode()) != nullptr;
	if (!bMainMenu)
	{
		auto* ControlledCharacter = Cast<AMothEffectCharacter>(GetPawn());
		auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode());
		auto* Director = Mode ? Mode->GetWaveDirector() : nullptr;
		// Older isolated test maps retain their original input flow.
		if (!ControlledCharacter || !Director || !Director->HasActorBegunPlay()) return;
		if (!Director->GetPlayer() && !Director->SetPlayer(ControlledCharacter)) return;
		// Commit the encounter before showing its HUD, including direct arena PIE.
		if (Director->GetPhase() == EMothFlowPhase::Ready && !Mode->StartEncounter()) return;
		if (auto* Instance = GetGameInstance<UMothGameInstance>(); Instance && Instance->HasBattleAfterTravel())
			Instance->ConsumeBattleAfterTravel();
	}
	UClass* Class = GameplayUIClass.LoadSynchronous();
	if (!Class) return;
	GameplayUI = CreateWidget<UMothHUDWidget>(this, Class);
	if (!GameplayUI) return;
	GameplayUI->AddToPlayerScreen(10);
	GameplayUI->RefreshState(); SyncUIInput();
	if (auto* Instance = GetGameInstance())
		if (auto* Loading = Instance->GetSubsystem<UMothLoadingSubsystem>()) Loading->MarkWorldReady(GetWorld());
}

void AMothEffectPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	InitializeGameplayUI();
	// Travel intent belongs to the encounter across the saved UMG pages.
	if (!bTravelRequested)
		if (auto* Instance = GetGameInstance<UMothGameInstance>(); Instance && Instance->HasBattleAfterTravel())
			if (auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode()); Mode && Mode->StartEncounter())
				Instance->ConsumeBattleAfterTravel();
	if (GameplayUI && !bTravelRequested) { GameplayUI->RefreshState(); SyncUIInput(); }
}

void AMothEffectPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AppliedUIPage = -1;
	InitializeGameplayUI();
}

void AMothEffectPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	bTravelRequested = true;
	if (auto* Instance = GetGameInstance())
		if (auto* Loading = Instance->GetSubsystem<UMothLoadingSubsystem>()) Loading->OnLoadingFinished.RemoveAll(this);
	if (GameplayUI) GameplayUI->RemoveFromParent();
	GameplayUI = nullptr;
	Super::EndPlay(Reason);
}

void AMothEffectPlayerController::SyncUIInput()
{
	if (!GameplayUI || bTravelRequested) return;
	if (auto* Instance = GetGameInstance())
		if (auto* Loading = Instance->GetSubsystem<UMothLoadingSubsystem>(); Loading && Loading->IsLoading()) return;
	const EMothUIPage Page = GameplayUI->GetHUDState().Page;
	if (AppliedUIPage == static_cast<int32>(Page)) return;
	AppliedUIPage = static_cast<int32>(Page);
	const bool bMenu = GameplayUI->IsMenuVisible();
	if (auto* ControlledCharacter = Cast<AMothEffectCharacter>(GetPawn()))
	{
		ControlledCharacter->CancelCombatInput(); ControlledCharacter->ResetMovementInput();
		// Pause freezes legal tasks such as reload/recovery; it does not cancel them.
		if (!GetWorld()->IsPaused())
		{
			auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode());
			auto* Director = Mode ? Mode->GetWaveDirector() : nullptr;
			if (!Director || Director->GetPlayer() == ControlledCharacter) ControlledCharacter->SetGameplayEnabled(!bMenu);
		}
	}
	ResetIgnoreMoveInput(); ResetIgnoreLookInput();
	SetIgnoreMoveInput(bMenu); SetIgnoreLookInput(bMenu);
	bShowMouseCursor = bMenu;
	if (bMenu)
	{
		FInputModeUIOnly InputMode; InputMode.SetWidgetToFocus(GameplayUI->GetMenuFocusTarget()->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(InputMode);
		GameplayUI->GetMenuFocusTarget()->SetUserFocus(this);
	}
	else
	{
		FInputModeGameOnly InputMode; InputMode.SetConsumeCaptureMouseDown(true); SetInputMode(InputMode);
		UWidgetBlueprintLibrary::SetFocusToGameViewport();
	}
}

void AMothEffectPlayerController::StartTeaching()
{
	// Retain serialized Blueprint calls; the release flow uses in-battle hints.
	StartBattle();
}

void AMothEffectPlayerController::StartBattle()
{
	if (bTravelRequested || !GameplayUI || (GameplayUI->GetHUDState().Page != EMothUIPage::Title
		&& GameplayUI->GetHUDState().Page != EMothUIPage::TutorialComplete)) return;
	auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode());
	auto* Instance = GetGameInstance<UMothGameInstance>();
	if (!Instance) return;
	if (Cast<AMothMainMenuGameMode>(GetWorld()->GetAuthGameMode()))
	{
		Instance->QueueBattleAfterTravel();
		auto* Loading = Instance->GetSubsystem<UMothLoadingSubsystem>();
		if (Loading && Loading->BeginBattleTravel(this, UMothGameInstance::GetBattleMapName())) bTravelRequested = true;
		else Instance->CancelBattleAfterTravel();
		return;
	}
	if (!Mode) return;
	Instance->QueueBattleAfterTravel();
	if (Mode->RestartEncounter()) bTravelRequested = true;
	else Instance->CancelBattleAfterTravel();
}

void AMothEffectPlayerController::ReturnToTitle()
{
	if (bTravelRequested || !GetWorld()) return;
	if (Cast<AMothMainMenuGameMode>(GetWorld()->GetAuthGameMode())) return;
	if (auto* Instance = GetGameInstance<UMothGameInstance>()) Instance->CancelBattleAfterTravel();
	if (GetWorld()->IsPaused()) SetPause(false);
	bTravelRequested = true;
	UGameplayStatics::OpenLevel(this, UMothGameInstance::GetMainMenuMapName());
}

void AMothEffectPlayerController::RestartBattle()
{
	if (bTravelRequested || !GameplayUI || !GetWorld()) return;
	const EMothUIPage Page = GameplayUI->GetHUDState().Page;
	if (Page != EMothUIPage::Pause && Page != EMothUIPage::Victory && Page != EMothUIPage::Defeat) return;
	auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode());
	auto* Instance = GetGameInstance<UMothGameInstance>();
	if (!Mode || !Instance) return;
	Instance->QueueBattleAfterTravel();
	if (Mode->RestartEncounter()) bTravelRequested = true;
	else Instance->CancelBattleAfterTravel();
}

void AMothEffectPlayerController::ToggleMenuPause()
{
	if (!GameplayUI || bTravelRequested) return;
	if (auto* Instance = GetGameInstance())
		if (auto* Loading = Instance->GetSubsystem<UMothLoadingSubsystem>(); Loading && Loading->IsLoading()) return;
	const EMothUIPage Page = GameplayUI->GetHUDState().Page;
	if (Page == EMothUIPage::Settings || Page == EMothUIPage::Credits) GameplayUI->CloseMenuPanel();
	else if (Page == EMothUIPage::Pause) ResumeGame();
	else if (Page == EMothUIPage::Gameplay || Page == EMothUIPage::Tutorial) SetPause(true);
}

void AMothEffectPlayerController::ResumeGame() { if (GetWorld() && GetWorld()->IsPaused()) SetPause(false); }

void AMothEffectPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Instance = GetGameInstance())
		if (auto* Loading = Instance->GetSubsystem<UMothLoadingSubsystem>())
			Loading->OnLoadingFinished.AddUObject(this, &AMothEffectPlayerController::HandleLoadingFinished);
	if (Cast<AMothMainMenuGameMode>(GetWorld()->GetAuthGameMode()))
		if (auto* Instance = GetGameInstance<UMothGameInstance>()) Instance->CancelBattleAfterTravel();
	// UI initialization commits arena preparation after the director binds its player.
	if (auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode()))
		if (auto* Director = Mode->GetWaveDirector()) Director->SetAutoStartEnabled(false);
	if (auto* Settings = UMothUserSettings::Get()) Settings->ApplyAudioSettings(this);

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogMothEffect, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AMothEffectPlayerController::HandleLoadingFinished(bool bSucceeded)
{
	if (!bSucceeded)
	{
		bTravelRequested = false;
		if (auto* Instance = GetGameInstance<UMothGameInstance>()) Instance->CancelBattleAfterTravel();
		if (GameplayUI) GameplayUI->ShowInteractionNotice(FText::FromString(TEXT("暂时无法进入战场，请重试。")));
	}
	AppliedUIPage = -1;
	SyncUIInput();
}

void AMothEffectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent) InputComponent->BindKey(EKeys::Escape, IE_Pressed, this,
		&AMothEffectPlayerController::ToggleMenuPause).bExecuteWhenPaused = true;

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AMothEffectPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AMothEffectPlayerController::FlushPressedKeys()
{
	if (AMothEffectCharacter* PlayerCharacter = Cast<AMothEffectCharacter>(GetPawn()))
	{
		PlayerCharacter->CancelCombatInput();
		PlayerCharacter->ResetMovementInput();
	}
	Super::FlushPressedKeys();
}

bool AMothEffectPlayerController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate)
{
	const bool bSucceeded = Super::SetPause(bPause, CanUnpauseDelegate);
	if (bSucceeded && bPause)
	{
		if (AMothEffectCharacter* PlayerCharacter = Cast<AMothEffectCharacter>(GetPawn()))
		{
			PlayerCharacter->CancelCombatInput();
			PlayerCharacter->ResetMovementInput();
		}
	}
	if (bSucceeded && GameplayUI) { GameplayUI->RefreshState(); SyncUIInput(); }
	return bSucceeded;
}

void AMothEffectPlayerController::MothLifecycleStats()
{
#if !UE_BUILD_SHIPPING
	if (UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>())
	{
		const FString Status = FString::Printf(TEXT("Lifecycle devices=%d/%d emitters=%d/%d columns=%d/%d projectiles=%d/%d"),
			Lifecycle->GetDeviceCount(), Lifecycle->GetDeviceLimit(), Lifecycle->GetActiveCount(EDeviceKind::Emitter),
			Lifecycle->GetActiveLimit(EDeviceKind::Emitter), Lifecycle->GetActiveCount(EDeviceKind::Launcher),
			Lifecycle->GetActiveLimit(EDeviceKind::Launcher), Lifecycle->GetProjectileCount(), Lifecycle->GetProjectileLimit());
		UE_LOG(LogMothEffect, Log, TEXT("%s"), *Status);
		if (GEngine) GEngine->AddOnScreenDebugMessage(7110, 6.0f, FColor::Cyan, Status);
	}
#endif
}

void AMothEffectPlayerController::MothSpawnDevice(int32 Kind, int32 Count)
{
#if !UE_BUILD_SHIPPING
	if (!GetPawn() || Kind < 0 || Kind > 2 || Count <= 0) return;
	const TCHAR* ClassPath = Kind == 0 ? TEXT("/Game/MothEffect/Devices/BP_DeviceLauncher.BP_DeviceLauncher_C")
		: Kind == 1 ? TEXT("/Game/MothEffect/Devices/BP_DeviceBomb.BP_DeviceBomb_C")
		: TEXT("/Game/MothEffect/Devices/BP_DeviceEmitter.BP_DeviceEmitter_C");
	UClass* DeviceClass = LoadClass<ADeviceBase>(nullptr, ClassPath);
	UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!Lifecycle || !DeviceClass) return;
	int32 SpawnedCount = 0;
	for (int32 I = 0; I < FMath::Min(64, Count); ++I)
	{
		const float Side = Count == 1 ? 0.0f : (I % 6 - 2.5f) * 200.0f;
		const FVector Position = GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * (160.0 + I / 6 * 200.0)
			+ GetPawn()->GetActorRightVector() * Side + FVector::UpVector * 35.0;
		if (Lifecycle->SpawnDevice(DeviceClass, FTransform(FRotator::ZeroRotator, Position))) ++SpawnedCount;
	}
	UE_LOG(LogMothEffect, Log, TEXT("Lifecycle debug spawn kind=%d requested=%d admitted=%d"), Kind, Count, SpawnedCount);
	MothLifecycleStats();
#endif
}

void AMothEffectPlayerController::MothResetDevices()
{
#if !UE_BUILD_SHIPPING
	if (UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()) Lifecycle->ResetManagedObjects();
	MothLifecycleStats();
#endif
}

void AMothEffectPlayerController::MothSpawnEnemy()
{
#if !UE_BUILD_SHIPPING
	if (!GetPawn()) return;
	if (auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode()))
		if (auto* Director = Mode->GetWaveDirector(); Director && Director->GetPhase() != EMothFlowPhase::Ready) return;
	UClass* Class = LoadClass<AEnemyBase>(nullptr, TEXT("/Game/MothEffect/Enemies/BP_EnemyBase.BP_EnemyBase_C"));
	if (!Class) return;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	GetWorld()->SpawnActor<AEnemyBase>(Class, GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * 900.0f, FRotator::ZeroRotator, Params);
	MothEnemyStats();
#endif
}

void AMothEffectPlayerController::MothResetEnemies()
{
#if !UE_BUILD_SHIPPING
	TArray<TWeakObjectPtr<AEnemyBase>> Enemies;
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It) Enemies.Add(*It);
	for (const auto& Enemy : Enemies) if (Enemy.IsValid()) Enemy->Destroy();
	MothEnemyStats();
#endif
}

void AMothEffectPlayerController::MothSpawnDropEnemy(int32 Kind)
{
#if !UE_BUILD_SHIPPING
	if (!GetPawn() || Kind < 0 || Kind > 2) return;
	if (auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode()))
		if (auto* Director = Mode->GetWaveDirector(); Director && Director->GetPhase() != EMothFlowPhase::Ready) return;
	const TCHAR* Path = Kind == 0 ? TEXT("/Game/MothEffect/Enemies/BP_EnemyLauncher.BP_EnemyLauncher_C")
		: Kind == 1 ? TEXT("/Game/MothEffect/Enemies/BP_EnemyBomb.BP_EnemyBomb_C")
		: TEXT("/Game/MothEffect/Enemies/BP_EnemyEmitter.BP_EnemyEmitter_C");
	UClass* Class = LoadClass<AEnemyBase>(nullptr, Path);
	if (!Class) return;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	GetWorld()->SpawnActor<AEnemyBase>(Class, GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * 900.0f, FRotator::ZeroRotator, Params);
	MothEnemyStats();
#endif
}

void AMothEffectPlayerController::MothEnemyStats()
{
#if !UE_BUILD_SHIPPING
	int32 Alive = 0;
	for (TActorIterator<AEnemyBase> It(GetWorld()); It; ++It)
	{
		if (!It->IsActorBeingDestroyed() && It->GetHealthComponent()->IsAlive()) ++Alive;
		UE_LOG(LogMothEffect, Log, TEXT("Enemy stats %s phase=%s health=%.1f target=%s sight=%d attempts=%d spawned=%d paths=%d stalls=%d"),
			*It->GetName(), *UEnum::GetValueAsString(It->GetCombatPhase()), It->GetHealthComponent()->GetCurrentHealth(),
			*GetNameSafe(It->GetCombatTarget()), It->HasTargetSight(), It->GetShotAttempts(), It->GetProjectilesSpawned(), It->GetNavigationRequests(), It->GetStallRecoveries());
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(7111, 6.0f, FColor::Cyan, FString::Printf(TEXT("Enemies alive=%d"), Alive));
#endif
}

void AMothEffectPlayerController::MothFlowStats()
{
#if !UE_BUILD_SHIPPING
	if (auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode()))
		if (auto* D = Mode->GetWaveDirector())
		{
			const FString Status = FString::Printf(TEXT("Flow %s wave=%d/3 alive=%d pending=%d prepare=%.2f"),
				*UEnum::GetValueAsString(D->GetPhase()), D->GetWaveNumber(), D->GetAliveCount(), D->GetPendingCount(), D->GetPreparationRemaining());
			UE_LOG(LogMothEffect, Log, TEXT("%s"), *Status);
			if (GEngine) GEngine->AddOnScreenDebugMessage(7112, 8, FColor::Cyan, Status);
		}
#endif
}

void AMothEffectPlayerController::MothFeedbackStats()
{
	const auto* Feedback=GetWorld()?GetWorld()->GetSubsystem<UMothFeedbackSubsystem>():nullptr;
	if (!Feedback) return;
	UE_LOG(LogMothEffect,Log,TEXT("Feedback profile=%s visuals=%d voices=%d hitAlpha=%.2f damageAlpha=%.2f"),
		*GetNameSafe(Feedback->GetProfile()),Feedback->GetLiveVisualCount(),Feedback->GetLiveAudioCount(),Feedback->GetHitMarkerAlpha(),Feedback->GetDamageEdgeAlpha());
	for (uint8 I=0; I<=static_cast<uint8>(EMothFeedbackCue::Failure); ++I)
		UE_LOG(LogMothEffect,Log,TEXT("Feedback cue=%s committed=%llu"),*UEnum::GetValueAsString(static_cast<EMothFeedbackCue>(I)),Feedback->GetCueCount(static_cast<EMothFeedbackCue>(I)));
}

void AMothEffectPlayerController::MothStartEncounter()
{
#if !UE_BUILD_SHIPPING
	if (auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode())) Mode->StartEncounter();
	MothFlowStats();
#endif
}

void AMothEffectPlayerController::MothRestartEncounter()
{
#if !UE_BUILD_SHIPPING
	if (auto* Mode = Cast<AMothEffectGameMode>(GetWorld()->GetAuthGameMode())) Mode->RestartEncounter();
#endif
}
