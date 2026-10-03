// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/MothEffectCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MothEffect.h"

AMothEffectCharacter::AMothEffectCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Keep the body upright and face the camera's horizontal aim direction.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = JumpVelocity;
	GetCharacterMovement()->AirControl = PlayerAirControl;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = ShoulderArmLength;
	CameraBoom->SocketOffset = ShoulderSocketOffset;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(HipFieldOfView);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AMothEffectCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Apply the new settings after template Blueprint component overrides load.
	ApplyPlayerSettings();
}

void AMothEffectCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float TargetAlpha = bIsAiming ? 1.0f : 0.0f;
	AimBlendAlpha = AimBlendSeconds > UE_SMALL_NUMBER
		? FMath::FInterpConstantTo(AimBlendAlpha, TargetAlpha, DeltaSeconds, 1.0f / AimBlendSeconds)
		: TargetAlpha;
	FollowCamera->SetFieldOfView(FMath::Lerp(HipFieldOfView, AimFieldOfView, AimBlendAlpha));
}

void AMothEffectCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetMovementInput();
	Super::EndPlay(EndPlayReason);
}

void AMothEffectCharacter::ApplyPlayerSettings()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->JumpZVelocity = FMath::Max(0.0f, JumpVelocity);
	GetCharacterMovement()->AirControl = FMath::Clamp(PlayerAirControl, 0.0f, 1.0f);
	CameraBoom->TargetArmLength = FMath::Max(0.0f, ShoulderArmLength);
	CameraBoom->SocketOffset = ShoulderSocketOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	FollowCamera->bUsePawnControlRotation = false;
	AimBlendAlpha = 0.0f;
	FollowCamera->SetFieldOfView(HipFieldOfView);
	ResetMovementInput();
}

void AMothEffectCharacter::RefreshMovementSpeed()
{
	bIsSprinting = bSprintRequested && !bIsAiming;
	const float DesiredSpeed = bIsAiming ? AimMoveSpeed : (bIsSprinting ? SprintSpeed : WalkSpeed);
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.0f, DesiredSpeed);
}

void AMothEffectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMothEffectCharacter::DoJumpStart);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMothEffectCharacter::DoJumpEnd);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &AMothEffectCharacter::DoJumpEnd);
		}

		// Moving
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMothEffectCharacter::Move);
		}
		if (MouseLookAction)
		{
			EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMothEffectCharacter::Look);
		}

		// Looking
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMothEffectCharacter::Look);
		}

		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AMothEffectCharacter::DoAimStart);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AMothEffectCharacter::DoAimEnd);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &AMothEffectCharacter::DoAimEnd);
		}
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AMothEffectCharacter::DoSprintStart);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMothEffectCharacter::DoSprintEnd);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AMothEffectCharacter::DoSprintEnd);
		}
		if (!AimAction || !SprintAction)
		{
			UE_LOG(LogMothEffect, Warning, TEXT("Assign AimAction and SprintAction in the player Blueprint and map them in IMC_MothGameplay."));
		}
	}
	else
	{
		UE_LOG(LogMothEffect, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMothEffectCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMothEffectCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMothEffectCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMothEffectCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMothEffectCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AMothEffectCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AMothEffectCharacter::DoAimStart()
{
	bIsAiming = true;
	CancelSprintUntilRelease();
}

void AMothEffectCharacter::DoAimEnd()
{
	bIsAiming = false;
	RefreshMovementSpeed();
}

void AMothEffectCharacter::DoSprintStart()
{
	if (bIsAiming || bSprintRequiresRelease)
	{
		bSprintRequiresRelease = true;
		return;
	}

	bSprintRequested = true;
	RefreshMovementSpeed();
}

void AMothEffectCharacter::DoSprintEnd()
{
	bSprintRequested = false;
	bSprintRequiresRelease = false;
	RefreshMovementSpeed();
}

void AMothEffectCharacter::CancelSprintUntilRelease()
{
	bSprintRequiresRelease |= bSprintRequested;
	bSprintRequested = false;
	RefreshMovementSpeed();
}

void AMothEffectCharacter::ResetMovementInput()
{
	bIsAiming = false;
	bSprintRequested = false;
	bSprintRequiresRelease = false;
	StopJumping();
	RefreshMovementSpeed();
}
