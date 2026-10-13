#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "Combat/RuleProjectile.h"
#include "Devices/DeviceBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "MothEffect.h"

namespace
{
	bool IsLive(const AActor* Actor) { return IsValid(Actor) && !Actor->IsActorBeingDestroyed(); }
	bool IsCounted(EDeviceState State)
	{
		return State == EDeviceState::Dormant || State == EDeviceState::Held || State == EDeviceState::Active;
	}
}

bool UDeviceLifecycleSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

TStatId UDeviceLifecycleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDeviceLifecycleSubsystem, STATGROUP_Tickables);
}

int32 UDeviceLifecycleSubsystem::GetDeviceCount() const
{
	int32 Count = PendingSpawns;
	for (const FDeviceRecord& Record : Devices)
		if (IsLive(Record.Device.Get()) && IsCounted(Record.Device->GetDeviceState())) ++Count;
	return Count;
}

int32 UDeviceLifecycleSubsystem::GetActiveCount(EDeviceKind Kind) const
{
	int32 Count = 0;
	for (const FDeviceRecord& Record : Devices)
		if (IsLive(Record.Device.Get()) && Record.Device->GetDeviceKind() == Kind
			&& (Record.Device->GetDeviceState() == EDeviceState::Active || Record.bActivationReserved)) ++Count;
	return Count;
}

int32 UDeviceLifecycleSubsystem::GetProjectileCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ARuleProjectile>& Projectile : Projectiles)
		if (IsLive(Projectile.Get()) && !Projectile->HasProcessedImpact()) ++Count;
	return Count;
}

void UDeviceLifecycleSubsystem::NotifyRejected(EMothCapacityKind Kind)
{
	// Throttle presentation, never admission. Both debug feedback and future HUD use this event.
	const int32 Index = static_cast<int32>(Kind);
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastCapacityNotice[Index] >= 0.0 && Now - LastCapacityNotice[Index] < 1.0) return;
	LastCapacityNotice[Index] = Now;
	OnCapacityRejected.Broadcast(Kind);
#if !UE_BUILD_SHIPPING
	const FString Message = FString::Printf(TEXT("CAPACITY FULL: %s"), *UEnum::GetValueAsString(Kind));
	UE_LOG(LogMothEffect, Log, TEXT("%s devices=%d emitters=%d columns=%d projectiles=%d"),
		*Message, GetDeviceCount(), GetActiveCount(EDeviceKind::Emitter), GetActiveCount(EDeviceKind::Launcher), GetProjectileCount());
	if (GEngine) GEngine->AddOnScreenDebugMessage(7100 + Index, 2.0f, FColor::Yellow, Message);
#endif
}

bool UDeviceLifecycleSubsystem::MakeRoomForDevice()
{
	if (!IsGameplayEnabled() || bChangingCapacity || GetWorld()->IsPaused()) return false;
	TGuardValue<bool> Guard(bChangingCapacity, true);
	const uint64 Generation = LifecycleGeneration;
	if (GetDeviceCount() < FMath::Max(1, MaxDevicesTotal)) return true;
	TWeakObjectPtr<ADeviceBase> Oldest;
	uint64 Order = MAX_uint64;
	for (const FDeviceRecord& Record : Devices)
		if (IsLive(Record.Device.Get()) && Record.Device->GetDeviceState() == EDeviceState::Dormant
			&& Record.CreatedOrder < Order) { Oldest = Record.Device; Order = Record.CreatedOrder; }
	if (Oldest.IsValid()) Oldest->DestroyDevice();
	if (Generation != LifecycleGeneration) return false;
	if (!IsGameplayEnabled() || GetDeviceCount() >= FMath::Max(1, MaxDevicesTotal))
	{
		NotifyRejected(EMothCapacityKind::Devices);
		return false;
	}
	return true;
}

ADeviceBase* UDeviceLifecycleSubsystem::SpawnDevice(TSubclassOf<ADeviceBase> DeviceClass, FTransform Transform)
{
	return SpawnDeviceInternal(DeviceClass, Transform, LifecycleGeneration, [] { return true; });
}

ADeviceBase* UDeviceLifecycleSubsystem::SpawnDeviceForGeneration(TSubclassOf<ADeviceBase> DeviceClass,
	FTransform Transform, uint64 Generation, TFunctionRef<bool()> ValidateBirthplace)
{
	return SpawnDeviceInternal(DeviceClass, Transform, Generation, ValidateBirthplace);
}

ADeviceBase* UDeviceLifecycleSubsystem::SpawnDeviceInternal(TSubclassOf<ADeviceBase> DeviceClass,
	FTransform Transform, uint64 Generation, TFunctionRef<bool()> ValidateBirthplace)
{
	if (!DeviceClass || DeviceClass->HasAnyClassFlags(CLASS_Abstract) || Transform.ContainsNaN()
		|| !GetWorld()->HasBegunPlay() || !IsGenerationCurrent(Generation) || !ValidateBirthplace()
		|| !IsGenerationCurrent(Generation) || !MakeRoomForDevice() || !IsGenerationCurrent(Generation)
		|| !ValidateBirthplace() || !IsGenerationCurrent(Generation)) return nullptr;
	++PendingSpawns;
	ADeviceBase* Device = GetWorld()->SpawnActorDeferred<ADeviceBase>(DeviceClass, Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Device) { --PendingSpawns; return nullptr; }
	ReservedSpawns.Add(Device);
	Device->FinishSpawning(Transform);
	if (ReservedSpawns.Remove(Device)) --PendingSpawns;
	if (!IsGenerationCurrent(Generation)) { if (IsLive(Device)) Device->DestroyDevice(); return nullptr; }
	return IsLive(Device) && Device->HasActorBegunPlay() ? Device : nullptr;
}

bool UDeviceLifecycleSubsystem::RegisterDevice(ADeviceBase* Device)
{
	if (!IsLive(Device) || Device->GetWorld() != GetWorld() || !IsGameplayEnabled()) return false;
	if (Devices.ContainsByPredicate([Device](const FDeviceRecord& R) { return R.Device == Device; })) return true;
	const bool bReserved = ReservedSpawns.Remove(Device) > 0;
	if (bReserved) --PendingSpawns;
	if ((!bReserved && !MakeRoomForDevice()) || !IsLive(Device) || !IsGameplayEnabled()) return false;
	FDeviceRecord Record;
	Record.Device = Device;
	Record.CreatedOrder = ++NextOrder;
	Devices.Add(Record);
	return true;
}

void UDeviceLifecycleSubsystem::UnregisterDevice(ADeviceBase* Device)
{
	Devices.RemoveAllSwap([Device](const FDeviceRecord& R) { return R.Device == Device; });
	if (ReservedSpawns.Remove(Device)) --PendingSpawns;
}

bool UDeviceLifecycleSubsystem::ReserveActivation(ADeviceBase* Device)
{
	if (!IsLive(Device) || !IsGameplayEnabled() || bChangingCapacity || GetWorld()->IsPaused()) return false;
	FDeviceRecord* Record = Devices.FindByPredicate([Device](const FDeviceRecord& R) { return R.Device == Device; });
	if (!Record || Record->bActivationReserved || Device->GetDeviceState() != EDeviceState::Dormant) return false;
	const EDeviceKind Kind = Device->GetDeviceKind();
	const int32 Limit = Kind == EDeviceKind::Emitter ? FMath::Max(1, MaxActiveEmitters)
		: Kind == EDeviceKind::Launcher ? FMath::Max(1, MaxActiveColumns) : 0;
	TGuardValue<bool> Guard(bChangingCapacity, true);
	if (Limit && GetActiveCount(Kind) >= Limit)
	{
		TWeakObjectPtr<ADeviceBase> Oldest;
		uint64 Order = MAX_uint64;
		for (const FDeviceRecord& R : Devices)
			if (IsLive(R.Device.Get()) && R.Device->GetDeviceKind() == Kind
				&& R.Device->GetDeviceState() == EDeviceState::Active && R.ActiveOrder < Order)
			{ Oldest = R.Device; Order = R.ActiveOrder; }
		if (Oldest.IsValid()) Oldest->FinishActivation();
		if (GetActiveCount(Kind) >= Limit)
		{
			NotifyRejected(Kind == EDeviceKind::Emitter ? EMothCapacityKind::Emitters : EMothCapacityKind::Columns);
			return false;
		}
	}
	Record = Devices.FindByPredicate([Device](const FDeviceRecord& R) { return R.Device == Device; });
	if (!IsGameplayEnabled() || !IsLive(Device) || !Record || Device->GetDeviceState() != EDeviceState::Dormant) return false;
	Record->bActivationReserved = true;
	Record->ActiveOrder = ++NextOrder;
	return true;
}

void UDeviceLifecycleSubsystem::CommitDeviceState(ADeviceBase* Device)
{
	if (FDeviceRecord* Record = Devices.FindByPredicate([Device](const FDeviceRecord& R) { return R.Device == Device; }))
		Record->bActivationReserved = false;
}

bool UDeviceLifecycleSubsystem::CanSpawnProjectile()
{
	if (!IsGameplayEnabled() || GetWorld()->IsPaused()) return false;
	if (GetProjectileCount() < FMath::Max(1, MaxLiveProjectiles)) return true;
	NotifyRejected(EMothCapacityKind::Projectiles);
	return false;
}

bool UDeviceLifecycleSubsystem::RegisterProjectile(ARuleProjectile* Projectile)
{
	if (!IsLive(Projectile) || Projectile->GetWorld() != GetWorld() || !IsGameplayEnabled()) return false;
	if (Projectiles.Contains(Projectile)) return true;
	if (!CanSpawnProjectile()) return false;
	Projectiles.Add(Projectile);
	return true;
}

void UDeviceLifecycleSubsystem::UnregisterProjectile(ARuleProjectile* Projectile)
{
	Projectiles.Remove(Projectile);
}

bool UDeviceLifecycleSubsystem::SetSafetyBounds(FVector Center, FVector HalfExtent)
{
	if (Center.ContainsNaN() || HalfExtent.ContainsNaN() || HalfExtent.GetMin() <= 0.0) return false;
	SafetyBounds = FBox(Center - HalfExtent, Center + HalfExtent);
	return true;
}

bool UDeviceLifecycleSubsystem::IsOutsideSafety(const AActor* Actor) const
{
	const FVector Position = Actor->GetActorLocation();
	if (Position.ContainsNaN()) return true;
	if (SafetyBounds.IsValid) return !SafetyBounds.IsInsideOrOn(Position);
	const AWorldSettings* Settings = GetWorld()->GetWorldSettings();
	return Settings->bEnableWorldBoundsChecks && (Position.Z < Settings->KillZ
		|| Position.GetAbsMax() > HALF_WORLD_MAX);
}

void UDeviceLifecycleSubsystem::Tick(float DeltaTime)
{
	if (!IsGameplayEnabled() || !GetWorld()->HasBegunPlay() || GetWorld()->IsPaused()) return;
	const TArray<FDeviceRecord> DeviceSnapshot = Devices;
	for (const FDeviceRecord& Record : DeviceSnapshot)
	{
		ADeviceBase* Device = Record.Device.Get();
		if (!IsLive(Device)) continue;
		if (IsOutsideSafety(Device)) Device->DestroyDevice();
		else Device->UpdateIdleLifecycle(GetIdleLifetimeSeconds(), GetExpiryWarningSeconds(), GetDormantSpentSeconds());
	}
	const auto ProjectileSnapshot = Projectiles;
	for (const auto& Projectile : ProjectileSnapshot)
		if (IsLive(Projectile.Get()) && IsOutsideSafety(Projectile.Get())) Projectile->Destroy();
	Devices.RemoveAllSwap([](const FDeviceRecord& R) { return !IsLive(R.Device.Get()); });
	Projectiles.RemoveAllSwap([](const auto& P) { return !IsLive(P.Get()); });
}

bool UDeviceLifecycleSubsystem::SetGameplayEnabled(bool bEnabled)
{
	if (bResetting) return false;
	if (bGameplayEnabled && !bEnabled) ++LifecycleGeneration;
	bGameplayEnabled = bEnabled;
	if (!bEnabled) if (auto* Feedback = GetWorld()->GetSubsystem<UMothFeedbackSubsystem>()) Feedback->ClearFeedback();
	return true;
}

void UDeviceLifecycleSubsystem::ResetManagedObjects()
{
	if (bResetting) return;
	TGuardValue<bool> Guard(bResetting, true);
	if (auto* Feedback = GetWorld()->GetSubsystem<UMothFeedbackSubsystem>()) Feedback->ClearFeedback();
	++LifecycleGeneration;
	const auto DeviceSnapshot = Devices;
	const auto ProjectileSnapshot = Projectiles;
	const auto SpawnSnapshot = ReservedSpawns;
	// Clear admission first; callbacks during destruction cannot create replacements.
	Devices.Reset(); Projectiles.Reset(); ReservedSpawns.Reset(); PendingSpawns = 0;
	for (const auto& Record : DeviceSnapshot) if (IsLive(Record.Device.Get())) Record.Device->DestroyDevice();
	for (const auto& Projectile : ProjectileSnapshot) if (IsLive(Projectile.Get())) Projectile->Destroy();
	for (const auto& Device : SpawnSnapshot) if (IsLive(Device.Get())) Device->DestroyDevice();
	NextOrder = 0;
	for (double& Notice : LastCapacityNotice) Notice = -1.0;
}

void UDeviceLifecycleSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
	bResetting = true;
	Devices.Reset(); Projectiles.Reset(); ReservedSpawns.Reset(); PendingSpawns = 0;
	Super::OnWorldEndPlay(InWorld);
}

void UDeviceLifecycleSubsystem::Deinitialize()
{
	bResetting = true;
	Devices.Reset(); Projectiles.Reset(); ReservedSpawns.Reset(); PendingSpawns = 0;
	Super::Deinitialize();
}
