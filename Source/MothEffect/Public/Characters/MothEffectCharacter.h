// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MothEffectCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 * Player movement, shoulder camera and aiming/sprinting input.
 * Gameplay actions and animation presentation are added in later milestones.
 */
UCLASS(abstract)
class AMothEffectCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float WalkSpeed = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float AimMoveSpeed = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Player|Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float SprintSpeed = 650.0f;

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

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

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
};

