// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/MothEffectCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "MothEffect.h"
#include "Weapons/Rifle.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

AMothEffectCharacter::AMothEffectCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	RifleComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("RifleComponent"));
	RifleComponent->SetupAttachment(GetMesh(), RifleAttachSocket);
	RifleComponent->SetChildActorOwnerOnCreation(true);

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

void AMothEffectCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureRifleComponent();
}

void AMothEffectCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Apply the new settings after template Blueprint component overrides load.
	ApplyPlayerSettings();
	HealthComponent->OnDied.AddDynamic(this, &AMothEffectCharacter::HandlePlayerDied);
	RifleComponent->OnChildActorCreated().AddUObject(this, &AMothEffectCharacter::HandleRifleCreated);
	ConfigureRifleComponent();
	// Registration normally creates the child before this actor's BeginPlay.
	HandleRifleCreated(RifleComponent->GetChildActor());
	if (!IsValid(Rifle))
	{
		UE_LOG(LogMothEffect, Warning, TEXT("Assign RifleClass=BP_Rifle and a valid RifleAttachSocket in the player Blueprint."));
	}
}

void AMothEffectCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TryClearPrimaryReleaseGate();

	const float TargetAlpha = bIsAiming ? 1.0f : 0.0f;
	AimBlendAlpha = AimBlendSeconds > UE_SMALL_NUMBER
		? FMath::FInterpConstantTo(AimBlendAlpha, TargetAlpha, DeltaSeconds, 1.0f / AimBlendSeconds)
		: TargetAlpha;
	FollowCamera->SetFieldOfView(FMath::Lerp(HipFieldOfView, AimFieldOfView, AimBlendAlpha));
}

void AMothEffectCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	RifleComponent->OnChildActorCreated().RemoveAll(this);
	CancelCombatInput();
	HealthComponent->OnDied.RemoveDynamic(this, &AMothEffectCharacter::HandlePlayerDied);
	UnbindRifle();
	// The child actor component owns destruction of its rifle.
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
		if (PrimaryAction)
		{
			EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AMothEffectCharacter::DoPrimaryStart);
			EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &AMothEffectCharacter::DoPrimaryEnd);
			EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Canceled, this, &AMothEffectCharacter::DoPrimaryCanceled);
		}
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AMothEffectCharacter::DoReloadStart);
		}
		if (!AimAction || !SprintAction)
		{
			UE_LOG(LogMothEffect, Warning, TEXT("Assign AimAction and SprintAction in the player Blueprint and map them in IMC_MothGameplay."));
		}
		if (!PrimaryAction || !ReloadAction)
		{
			UE_LOG(LogMothEffect, Warning, TEXT("Assign PrimaryAction and ReloadAction in the player Blueprint and map them in IMC_MothGameplay."));
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
	if (GetController() != nullptr && IsGameplayEnabled())
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
	if (GetController() != nullptr && IsGameplayEnabled())
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMothEffectCharacter::DoJumpStart()
{
	// signal the character to jump
	if (IsGameplayEnabled())
	{
		Jump();
	}
}

void AMothEffectCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AMothEffectCharacter::DoAimStart()
{
	if (!IsGameplayEnabled())
	{
		return;
	}
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
	if (!IsGameplayEnabled() || bIsAiming || bSprintRequiresRelease)
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

void AMothEffectCharacter::ConfigureRifleComponent()
{
	if (bEndingPlay)
	{
		return;
	}
	RifleComponent->SetChildActorOwnerOnCreation(true);
	if (RifleComponent->GetAttachParent() != GetMesh() || RifleComponent->GetAttachSocketName() != RifleAttachSocket)
	{
		RifleComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RifleAttachSocket);
	}
	// Same-class SetChildActorClass recreates registered children; do not call it on viewport drags.
	if (RifleComponent->GetChildActorClass().Get() != RifleClass.Get())
	{
		RifleComponent->SetChildActorClass(RifleClass.Get());
	}
	// The component transform is the editable offset. Never overwrite it during construction.
}

void AMothEffectCharacter::HandleRifleCreated(AActor* ChildActor)
{
	ARifle* CreatedRifle = Cast<ARifle>(ChildActor);
	if (bEndingPlay || !IsValid(CreatedRifle) || CreatedRifle == Rifle.Get())
	{
		return;
	}
	if (!GetMesh()->DoesSocketExist(RifleAttachSocket))
	{
		UE_LOG(LogMothEffect, Warning, TEXT("RifleAttachSocket '%s' does not exist on the player Mesh."), *RifleAttachSocket.ToString());
		return;
	}
	if (IsValid(Rifle))
	{
		CancelCombatInput();
		UnbindRifle();
	}
	if (ActionState == EPlayerActionState::Reloading)
	{
		HandleReloadFinished(false);
	}
	if (bEndingPlay || !IsValid(CreatedRifle))
	{
		return;
	}
	Rifle = CreatedRifle;
	Rifle->SetOwner(this);
	Rifle->SetInstigator(this);
	Rifle->OnShotFired.AddUniqueDynamic(this, &AMothEffectCharacter::HandleShotFired);
	Rifle->OnReloadStarted.AddUniqueDynamic(this, &AMothEffectCharacter::HandleReloadStarted);
	Rifle->OnReloadFinished.AddUniqueDynamic(this, &AMothEffectCharacter::HandleReloadFinished);
	Rifle->OnDestroyed.AddUniqueDynamic(this, &AMothEffectCharacter::HandleRifleDestroyed);
}

void AMothEffectCharacter::UnbindRifle()
{
	if (IsValid(Rifle))
	{
		Rifle->OnShotFired.RemoveDynamic(this, &AMothEffectCharacter::HandleShotFired);
		Rifle->OnReloadStarted.RemoveDynamic(this, &AMothEffectCharacter::HandleReloadStarted);
		Rifle->OnReloadFinished.RemoveDynamic(this, &AMothEffectCharacter::HandleReloadFinished);
		Rifle->OnDestroyed.RemoveDynamic(this, &AMothEffectCharacter::HandleRifleDestroyed);
		Rifle->StopFire();
		Rifle->CancelReload();
	}
	Rifle = nullptr;
}

bool AMothEffectCharacter::IsGameplayEnabled() const
{
	return bGameplayEnabled && !bEndingPlay && ActionState != EPlayerActionState::Dead
		&& HealthComponent->IsAlive() && GetWorld() && !GetWorld()->IsPaused();
}

bool AMothEffectCharacter::CanFireRifle() const
{
	return IsGameplayEnabled() && ActionState == EPlayerActionState::Ready
		&& PrimaryPressMode == EPrimaryPressMode::Fire && !bRequirePrimaryRelease;
}

bool AMothEffectCharacter::CanReloadRifle() const
{
	return IsGameplayEnabled() && ActionState == EPlayerActionState::Ready;
}

bool AMothEffectCharacter::GetRifleView(FVector& ViewLocation, FVector& ViewDirection) const
{
	if (!IsGameplayEnabled())
	{
		return false;
	}
	FRotator ViewRotation;
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		ViewLocation = FollowCamera->GetComponentLocation();
		ViewRotation = FollowCamera->GetComponentRotation();
	}
	ViewDirection = ViewRotation.Vector().GetSafeNormal();
	return !ViewLocation.ContainsNaN() && !ViewDirection.IsNearlyZero();
}

FVector AMothEffectCharacter::GetRifleSafetyOrigin() const
{
	return GetActorLocation() + GetActorRotation().RotateVector(RifleSafetyOriginOffset);
}

void AMothEffectCharacter::DoPrimaryStart()
{
	if (bRequirePrimaryRelease || !IsGameplayEnabled() || ActionState != EPlayerActionState::Ready || !IsValid(Rifle))
	{
		PrimaryPressMode = EPrimaryPressMode::Blocked;
		return;
	}
	PrimaryPressMode = EPrimaryPressMode::Fire;
	if (!Rifle->TryStartFire())
	{
		PrimaryPressMode = EPrimaryPressMode::Blocked;
	}
}

void AMothEffectCharacter::DoPrimaryEnd()
{
	if (IsValid(Rifle))
	{
		Rifle->StopFire();
	}
	if (bRequirePrimaryRelease)
	{
		TryClearPrimaryReleaseGate();
		return;
	}
	if (IsPrimaryButtonPhysicallyDown())
	{
		CancelCombatInput();
		return;
	}
	bRequirePrimaryRelease = false;
	PrimaryPressMode = EPrimaryPressMode::None;
}

void AMothEffectCharacter::DoPrimaryCanceled()
{
	CancelCombatInput();
}

void AMothEffectCharacter::CancelCombatInput()
{
	PrimaryPressMode = EPrimaryPressMode::Blocked;
	bRequirePrimaryRelease = true;
	if (IsValid(Rifle))
	{
		Rifle->StopFire();
	}
}

bool AMothEffectCharacter::IsPrimaryButtonPhysicallyDown() const
{
#if PLATFORM_WINDOWS
	// PlayerInput keys are cleared by FlushPressedKeys; poll hardware for a real release.
	return (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
#else
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	return PlayerController && PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
#endif
}

void AMothEffectCharacter::TryClearPrimaryReleaseGate()
{
	if (!bRequirePrimaryRelease || bEndingPlay || !IsGameplayEnabled())
	{
		return;
	}
	if (FSlateApplication::IsInitialized() && !FSlateApplication::Get().IsActive())
	{
		return;
	}
	if (!IsPrimaryButtonPhysicallyDown())
	{
		bRequirePrimaryRelease = false;
		PrimaryPressMode = EPrimaryPressMode::None;
	}
}

void AMothEffectCharacter::DoReloadStart()
{
	if (CanReloadRifle() && IsValid(Rifle))
	{
		Rifle->TryBeginReload();
	}
}

void AMothEffectCharacter::SetActionState(EPlayerActionState NewState)
{
	if (ActionState == NewState)
	{
		return;
	}
	const EPlayerActionState OldState = ActionState;
	ActionState = NewState;
	OnPlayerActionStateChanged.Broadcast(OldState, NewState);
}

void AMothEffectCharacter::HandleShotFired(ARifle* FiredRifle)
{
	if (bEndingPlay || FiredRifle != Rifle.Get() || !IsGameplayEnabled() || ActionState != EPlayerActionState::Ready)
	{
		return;
	}
	if (FireMontage)
	{
		PlayAnimMontage(FireMontage);
	}
}

void AMothEffectCharacter::HandleReloadStarted(ARifle* ReloadingRifle)
{
	if (bEndingPlay || ReloadingRifle != Rifle.Get())
	{
		return;
	}
	CancelCombatInput();
	SetActionState(EPlayerActionState::Reloading);
	if (ReloadMontage && IsGameplayEnabled() && IsValid(Rifle) && Rifle->IsReloading())
	{
		const float PlayRate = ReloadMontage->GetPlayLength() / FMath::Max(0.01f, Rifle->GetReloadSeconds());
		PlayAnimMontage(ReloadMontage, FMath::Max(UE_KINDA_SMALL_NUMBER, PlayRate));
	}
}

void AMothEffectCharacter::HandleReloadFinished(bool bCompleted)
{
	if (bEndingPlay || ActionState != EPlayerActionState::Reloading)
	{
		return;
	}
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (ReloadMontage)
		{
			AnimInstance->Montage_Stop(0.1f, ReloadMontage);
		}
	}
	SetActionState(EPlayerActionState::Ready);
}

void AMothEffectCharacter::SetGameplayEnabled(bool bEnabled)
{
	bGameplayEnabled = bEnabled;
	if (!bEnabled)
	{
		CancelCombatInput();
		ResetMovementInput();
		if (IsValid(Rifle))
		{
			Rifle->CancelReload();
		}
	}
}

bool AMothEffectCharacter::ReceiveBallisticHit_Implementation(const FHitContext& Context)
{
	return IsGameplayEnabled() && HealthComponent->ApplyHit(Context);
}

void AMothEffectCharacter::HandlePlayerDied(AActor* Victim, AActor* SourceActor)
{
	if (bEndingPlay || Victim != this || ActionState == EPlayerActionState::Dead)
	{
		return;
	}
	SetActionState(EPlayerActionState::Dead);
	SetGameplayEnabled(false);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.1f);
	}
	OnPlayerDied(SourceActor);
}

void AMothEffectCharacter::HandleRifleDestroyed(AActor* DestroyedActor)
{
	if (bEndingPlay || DestroyedActor != Rifle.Get())
	{
		return;
	}
	Rifle = nullptr;
	CancelCombatInput();
	if (ActionState == EPlayerActionState::Reloading)
	{
		HandleReloadFinished(false);
	}
}
