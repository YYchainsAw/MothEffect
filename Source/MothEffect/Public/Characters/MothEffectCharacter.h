// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Interfaces/BallisticReactive.h"
#include "Types/PlayerActionState.h"
#include "MothEffectCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UHealthComponent;
class UAnimMontage;
class ARifle;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothPlayerActionChanged,
	EPlayerActionState, OldState, EPlayerActionState, NewState);

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 * Player movement, shoulder camera, rifle input, action state and health.
 * Carrying and throwing transitions are added in the device milestone.
 */
UCLASS(abstract)
class MOTHEFFECT_API AMothEffectCharacter : public ACharacter, public IBallisticReactive
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UHealthComponent> HealthComponent;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> PrimaryAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Weapon")
	TSubclassOf<ARifle> RifleClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Weapon")
	FName RifleAttachSocket = TEXT("WeaponSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Weapon")
	FTransform RifleRelativeTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Weapon")
	FVector RifleSafetyOriginOffset = FVector(0.0f, 0.0f, 30.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Player|Weapon")
	TObjectPtr<ARifle> Rifle;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Player|State")
	EPlayerActionState ActionState = EPlayerActionState::Ready;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Player|State")
	EPrimaryPressMode PrimaryPressMode = EPrimaryPressMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float WalkSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float AimMoveSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float SprintSpeed = 625.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float JumpVelocity = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Movement", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PlayerAirControl = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Camera", meta=(ClampMin="0.0", Units="cm"))
	float ShoulderArmLength = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Camera")
	FVector ShoulderSocketOffset = FVector(0.0f, 60.0f, 55.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Camera", meta=(ClampMin="5.0", ClampMax="170.0", Units="deg"))
	float HipFieldOfView = 85.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Camera", meta=(ClampMin="5.0", ClampMax="170.0", Units="deg"))
	float AimFieldOfView = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Camera", meta=(ClampMin="0.0", Units="s"))
	float AimBlendSeconds = 0.15f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Player|State")
	bool bIsAiming = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Player|State")
	bool bIsSprinting = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Player|State")
	float AimBlendAlpha = 0.0f;

	bool bSprintRequested = false;
	bool bSprintRequiresRelease = false;

public:

	/** Constructor */
	AMothEffectCharacter();	

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ApplyPlayerSettings();
	void RefreshMovementSpeed();
	void SpawnRifle();
	void SetActionState(EPlayerActionState NewState);
	void TryClearPrimaryReleaseGate();
	bool IsPrimaryButtonPhysicallyDown() const;

	UFUNCTION()
	void HandleShotFired(ARifle* FiredRifle);

	UFUNCTION()
	void HandleReloadStarted(ARifle* ReloadingRifle);

	UFUNCTION()
	void HandleReloadFinished(bool bCompleted);

	UFUNCTION()
	void HandlePlayerDied(AActor* Victim, AActor* SourceActor);

	UFUNCTION()
	void HandleRifleDestroyed(AActor* DestroyedActor);

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	UPROPERTY(BlueprintAssignable, Category="Player|State")
	FMothPlayerActionChanged OnPlayerActionStateChanged;

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoPrimaryStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoPrimaryEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoPrimaryCanceled();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoReloadStart();

	/** Flush/pause cancels the press and requires a real release; reload remains timed. */
	UFUNCTION(BlueprintCallable, Category="Player|Weapon")
	void CancelCombatInput();

	/** Disabling round gameplay cancels reload as well; pause uses CancelCombatInput instead. */
	UFUNCTION(BlueprintCallable, Category="Player|State")
	void SetGameplayEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Player|State")
	bool IsGameplayEnabled() const;

	UFUNCTION(BlueprintPure, Category="Player|State")
	EPlayerActionState GetActionState() const { return ActionState; }

	UFUNCTION(BlueprintPure, Category="Player|Weapon")
	ARifle* GetRifle() const { return Rifle.Get(); }

	UFUNCTION(BlueprintPure, Category="Player|Health")
	UHealthComponent* GetHealthComponent() const { return HealthComponent.Get(); }

	bool CanFireRifle() const;
	bool CanReloadRifle() const;
	bool GetRifleView(FVector& ViewLocation, FVector& ViewDirection) const;
	FVector GetRifleSafetyOrigin() const;
	virtual bool ReceiveBallisticHit_Implementation(const FHitContext& Context) override;

	UFUNCTION(BlueprintImplementableEvent, Category="Player|Health")
	void OnPlayerDied(AActor* SourceActor);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoAimStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoAimEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSprintStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSprintEnd();

	/** Used by aiming now and by accepted gunfire in the weapon milestone. */
	UFUNCTION(BlueprintCallable, Category="Player|Movement")
	void CancelSprintUntilRelease();

	/** Clears aiming/sprinting when input is flushed or this pawn ends play. */
	UFUNCTION(BlueprintCallable, Category="Player|Movement")
	void ResetMovementInput();

	UFUNCTION(BlueprintPure, Category="Player|State")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category="Player|State")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category="Player|State")
	float GetAimBlendAlpha() const { return AimBlendAlpha; }

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	bool bRequirePrimaryRelease = false;
	bool bGameplayEnabled = true;
	bool bEndingPlay = false;
};
