// Copyright Epic Games, Inc. All Rights Reserved.


#include "Framework/MothEffectPlayerController.h"
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
#include "Engine/World.h"

void AMothEffectPlayerController::BeginPlay()
{
	Super::BeginPlay();

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

void AMothEffectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

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
	if (GEngine) GEngine->AddOnScreenDebugMessage(7111, 6.0f, FColor::Cyan, FString::Printf(TEXT("Enemies alive=%d (T14 debug, wave limit belongs to T16)"), Alive));
#endif
}
