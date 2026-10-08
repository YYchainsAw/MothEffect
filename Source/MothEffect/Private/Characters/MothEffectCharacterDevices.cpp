#include "Characters/MothEffectCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "Devices/DeviceReleaseSafety.h"
#include "Engine/World.h"
#include "MothEffect.h"
#include "TimerManager.h"
#include "Weapons/Rifle.h"

void AMothEffectCharacter::ConfigureDeviceHoldPoint()
{
	DeviceHoldPoint->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, DeviceAttachSocket);
	// The component's Blueprint transform is the editable carry offset.
}

bool AMothEffectCharacter::IsCarryingDevice() const
{
	return IsValid(HeldDevice) && HeldDevice->GetDeviceState() == EDeviceState::Held
		&& HeldDevice->GetHolder() == this;
}

void AMothEffectCharacter::RefreshCarryPresentation()
{
	if (IsValid(Rifle))
	{
		Rifle->SetActorHiddenInGame(IsCarryingDevice());
	}
}

bool AMothEffectCharacter::RejectDeviceInteraction(const TCHAR* Reason)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogMothEffect, Log, TEXT("Player %s device interaction rejected: %s"), *GetName(), Reason);
#endif
	OnDeviceInteractionRejected(FText::FromString(Reason));
	return false;
}

void AMothEffectCharacter::DoInteract()
{
	if (!IsGameplayEnabled() || bDeviceInteractionInProgress)
	{
		return;
	}
	if (ActionState == EPlayerActionState::Carrying)
	{
		TryReleaseHeldDevice(false);
		return;
	}
	if (ActionState != EPlayerActionState::Ready && ActionState != EPlayerActionState::Reloading)
	{
		RejectDeviceInteraction(TEXT("当前动作尚未结束，暂时不能拾取。"));
		return;
	}
	FVector ViewLocation, ViewDirection;
	if (!GetRifleView(ViewLocation, ViewDirection))
	{
		return;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothDevicePickupCandidate), true, this);
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, true, true);
	Params.AddIgnoredActors(AttachedActors);
	// First blocking hit prevents selecting a device behind a wall or another blocker.
	const float ViewRange = FMath::Max(0.0f, PickupRangeCm) + FVector::Distance(ViewLocation, GetActorLocation());
	FHitResult Hit;
	const ECollisionChannel Channel = IsValid(Rifle) ? Rifle->GetWeaponTraceChannel() : ECC_Visibility;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation,
		ViewLocation + ViewDirection * ViewRange, Channel, Params))
	{
		RejectDeviceInteraction(TEXT("准星处没有可拾取道具。"));
		return;
	}
	TryPickupDevice(Cast<ADeviceBase>(Hit.GetActor()));
}

bool AMothEffectCharacter::TryPickupDevice(ADeviceBase* Device)
{
	if (!IsGameplayEnabled() || !HasActorBegunPlay() || bDeviceInteractionInProgress
		|| (ActionState != EPlayerActionState::Ready && ActionState != EPlayerActionState::Reloading)
		|| IsValid(HeldDevice))
	{
		return RejectDeviceInteraction(TEXT("当前不能拾取；只能携带一件道具。"));
	}
	if (!IsValid(Device) || Device->GetWorld() != GetWorld() || !Device->CanBePickedUp())
	{
		return RejectDeviceInteraction(TEXT("只能拾取尚未启动的道具。"));
	}
	if (!FMath::IsFinite(PickupRangeCm) || PickupRangeCm <= 0.0f
		|| FVector::DistSquared(GetActorLocation(), Device->GetActorLocation()) > FMath::Square(PickupRangeCm))
	{
		return RejectDeviceInteraction(TEXT("道具超出拾取距离。"));
	}
	if (!GetMesh()->DoesSocketExist(DeviceAttachSocket))
	{
		return RejectDeviceInteraction(TEXT("持物挂点不存在，请配置 Device Attach Socket。"));
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothDevicePickupOcclusion), true, this);
	Params.AddIgnoredActor(Device);
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, true, true);
	Params.AddIgnoredActors(AttachedActors);
	const ECollisionChannel Channel = IsValid(Rifle) ? Rifle->GetWeaponTraceChannel() : ECC_Visibility;
	FHitResult Hit;
	FVector ViewLocation, ViewDirection;
	if (!GetRifleView(ViewLocation, ViewDirection)
		|| GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, Device->GetActorLocation(), Channel, Params)
		|| GetWorld()->LineTraceSingleByChannel(Hit, GetRifleSafetyOrigin(), Device->GetActorLocation(), Channel, Params))
	{
		return RejectDeviceInteraction(TEXT("道具被遮挡，无法拾取。"));
	}

	TGuardValue<bool> InteractionGuard(bDeviceInteractionInProgress, true);
	if (!Device->CommitHeld(this, DeviceHoldPoint))
	{
		return RejectDeviceInteraction(TEXT("道具已失效，无法拾取。"));
	}
	HeldDevice = Device;
	HeldDeviceStateHandle = Device->OnDeviceStateChangedNative.AddUObject(
		this, &AMothEffectCharacter::HandleHeldDeviceStateChanged);
	const EPlayerActionState OldState = ActionState;
	ActionState = EPlayerActionState::Carrying;
	CancelCombatInput();
	RefreshCarryPresentation();
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (FireMontage) AnimInstance->Montage_Stop(0.1f, FireMontage);
		if (ReloadMontage) AnimInstance->Montage_Stop(0.1f, ReloadMontage);
	}
	// Reload callbacks see Carrying and cannot overwrite it or refill cancelled ammunition.
	if (IsValid(Rifle)) Rifle->CancelReload();
	if (IsValid(Device) && IsCarryingDevice())
	{
		Device->NotifyStateChanged(EDeviceState::Dormant, EDeviceState::Held);
	}
	if (ActionState == EPlayerActionState::Carrying && IsCarryingDevice())
	{
		OnPlayerActionStateChanged.Broadcast(OldState, ActionState);
	}
	return true;
}

bool AMothEffectCharacter::TryReleaseHeldDevice(bool bThrow)
{
	if (!IsGameplayEnabled() || bDeviceInteractionInProgress
		|| ActionState != EPlayerActionState::Carrying || !IsCarryingDevice())
	{
		return false;
	}
	TGuardValue<bool> InteractionGuard(bDeviceInteractionInProgress, true);
	ADeviceBase* Device = HeldDevice.Get();
	FVector AimPoint, ViewLocation, ViewDirection;
	if (bThrow && (!IsValid(Rifle) || !Rifle->GetAimTarget(AimPoint, ViewLocation, ViewDirection)))
	{
		return RejectDeviceInteraction(TEXT("无法确定投掷方向。"));
	}
	if (!bThrow && !GetRifleView(ViewLocation, ViewDirection))
	{
		return false;
	}
	FVector Forward = FVector(ViewDirection.X, ViewDirection.Y, 0.0f).GetSafeNormal();
	if (Forward.IsNearlyZero()) Forward = GetActorForwardVector().GetSafeNormal2D();
	USphereComponent* Body = Device->GetPhysicsBody();
	const float Radius = Body->GetScaledSphereRadius();
	const float OutsideDistance = GetCapsuleComponent()->GetScaledCapsuleRadius() + Radius + 1.0f;
	if (!FMath::IsFinite(Radius) || Radius <= 0.0f
		|| !FMath::IsFinite(DeviceReleaseForwardOffset) || DeviceReleaseForwardOffset < OutsideDistance
		|| !FMath::IsFinite(DeviceReleaseUpOffset))
	{
		return RejectDeviceInteraction(TEXT("释放位置必须位于角色身体外侧。"));
	}
	const FVector Origin = GetActorLocation() + FVector::UpVector * DeviceReleaseUpOffset;
	const FVector OutsideStart = Origin + Forward * OutsideDistance;
	const FVector ReleaseLocation = Origin + Forward * DeviceReleaseForwardOffset;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothDeviceRelease), false, this);
	Params.AddIgnoredActor(Device);
	if (IsValid(Rifle)) Params.AddIgnoredActor(Rifle);
	// Guard the short path to the capsule exterior too: a thin wall cannot fall between the two origins.
	if (!MothDeviceRelease::IsPathClear(GetWorld(), Body, Origin, OutsideStart, Params)
		|| !MothDeviceRelease::IsPathClear(GetWorld(), Body, OutsideStart, ReleaseLocation, Params))
	{
		return RejectDeviceInteraction(TEXT("前方空间不足，仍保持携带；移开后重试。"));
	}
	FVector Velocity = FVector::ZeroVector;
	if (bThrow)
	{
		const FVector Direction = (AimPoint - ReleaseLocation).GetSafeNormal();
		if (Direction.IsNearlyZero() || FVector::DotProduct(Direction, ViewDirection) <= 0.0f
			|| !FMath::IsFinite(ThrowForwardVelocity) || !FMath::IsFinite(ThrowUpwardVelocity)
			|| !FMath::IsFinite(MinGunRecoverySeconds) || MinGunRecoverySeconds <= 0.0f)
		{
			return RejectDeviceInteraction(TEXT("投掷目标被近处遮挡或投掷参数无效。"));
		}
		Velocity = Direction * FMath::Max(0.0f, ThrowForwardVelocity)
			+ FVector::UpVector * FMath::Max(0.0f, ThrowUpwardVelocity);
	}
	if (!Device->CommitReleased(this, ReleaseLocation, Velocity))
	{
		return RejectDeviceInteraction(TEXT("道具暂时不能释放，仍保持携带。"));
	}
	ClearHeldDevice();
	const EPlayerActionState NewState = bThrow ? EPlayerActionState::ThrowRecovery : EPlayerActionState::Ready;
	if (bThrow)
	{
		PrimaryPressMode = EPrimaryPressMode::Throw;
		bRequirePrimaryRelease = true;
		GetWorld()->GetTimerManager().SetTimer(ThrowRecoveryTimer,
			this, &AMothEffectCharacter::FinishThrowRecovery, MinGunRecoverySeconds, false);
	}
	SetActionState(NewState);
	if (IsValid(Device) && Device->GetDeviceState() == EDeviceState::Dormant)
	{
		Device->NotifyStateChanged(EDeviceState::Held, EDeviceState::Dormant);
	}
	if (IsGameplayEnabled() && ActionState == NewState && IsValid(Device))
	{
		if (bThrow && ThrowMontage) PlayAnimMontage(ThrowMontage);
		OnDeviceReleased(Device, bThrow);
	}
	return true;
}

void AMothEffectCharacter::FinishThrowRecovery()
{
	// World timers freeze during pause; completion changes action permission only, never fires.
	if (IsGameplayEnabled() && ActionState == EPlayerActionState::ThrowRecovery)
	{
		SetActionState(EPlayerActionState::Ready);
	}
}

void AMothEffectCharacter::ClearHeldDevice()
{
	if (HeldDevice)
	{
		HeldDevice->OnDeviceStateChangedNative.Remove(HeldDeviceStateHandle);
	}
	HeldDeviceStateHandle.Reset();
	HeldDevice = nullptr;
	RefreshCarryPresentation();
}

void AMothEffectCharacter::HandleHeldDeviceStateChanged(ADeviceBase* Device, EDeviceState OldState, EDeviceState NewState)
{
	if (Device != HeldDevice.Get() || IsCarryingDevice())
	{
		return;
	}
	ClearHeldDevice();
	CancelCombatInput();
	if (!bEndingPlay && ActionState == EPlayerActionState::Carrying)
	{
		SetActionState(EPlayerActionState::Ready);
	}
}

void AMothEffectCharacter::CancelDeviceTasks()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(ThrowRecoveryTimer);
	ADeviceBase* Device = HeldDevice.Get();
	ClearHeldDevice();
	if (IsValid(Device)) Device->DestroyDevice();
	if (!bEndingPlay && ActionState != EPlayerActionState::Dead
		&& (ActionState == EPlayerActionState::Carrying || ActionState == EPlayerActionState::ThrowRecovery))
	{
		SetActionState(EPlayerActionState::Ready);
	}
}
