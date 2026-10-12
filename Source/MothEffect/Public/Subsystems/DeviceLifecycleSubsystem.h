#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Types/DeviceTypes.h"
#include "DeviceLifecycleSubsystem.generated.h"

class ADeviceBase;
class ARuleProjectile;

UENUM(BlueprintType)
enum class EMothCapacityKind : uint8 { Devices, Emitters, Columns, Projectiles };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMothCapacityRejected, EMothCapacityKind, Kind);

/** World-local admission, idle expiry and teardown; no counters survive a map reload. */
UCLASS(Config=Game, DefaultConfig)
class MOTHEFFECT_API UDeviceLifecycleSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void OnWorldEndPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Drop/spawn callers use this entry before allocating an actor. Placed actors also register. */
	UFUNCTION(BlueprintCallable, Category="Moth|Lifecycle")
	ADeviceBase* SpawnDevice(TSubclassOf<ADeviceBase> DeviceClass, FTransform Transform);
	/** Revalidate birthplace after capacity callbacks; never admit a drop from a reset epoch. */
	ADeviceBase* SpawnDeviceForGeneration(TSubclassOf<ADeviceBase> DeviceClass, FTransform Transform,
		uint64 Generation, TFunctionRef<bool()> ValidateBirthplace);
	uint64 GetLifecycleGeneration() const { return LifecycleGeneration; }
	bool IsGenerationCurrent(uint64 Generation) const { return !bResetting && LifecycleGeneration == Generation; }
	UFUNCTION(BlueprintCallable, Category="Moth|Lifecycle")
	void ResetManagedObjects();
	/** Optional arena bounds. Without these, use the world's enabled KillZ/world bounds checks. */
	UFUNCTION(BlueprintCallable, Category="Moth|Lifecycle")
	bool SetSafetyBounds(FVector Center, FVector HalfExtent);
	UFUNCTION(BlueprintCallable, Category="Moth|Lifecycle")
	void ClearSafetyBounds() { SafetyBounds = FBox(ForceInit); }
	UFUNCTION(BlueprintPure, Category="Moth|Lifecycle")
	int32 GetDeviceCount() const;
	UFUNCTION(BlueprintPure, Category="Moth|Lifecycle")
	int32 GetActiveCount(EDeviceKind Kind) const;
	UFUNCTION(BlueprintPure, Category="Moth|Lifecycle")
	int32 GetProjectileCount() const;
	UPROPERTY(BlueprintAssignable, Category="Moth|Lifecycle")
	FMothCapacityRejected OnCapacityRejected;

	bool RegisterDevice(ADeviceBase* Device);
	void UnregisterDevice(ADeviceBase* Device);
	bool ReserveActivation(ADeviceBase* Device);
	void CommitDeviceState(ADeviceBase* Device);
	bool CanSpawnProjectile();
	bool RegisterProjectile(ARuleProjectile* Projectile);
	void UnregisterProjectile(ARuleProjectile* Projectile);
	float GetIdleLifetimeSeconds() const { return FMath::Max(0.01f, DormantIdleLifetimeSeconds); }
	float GetExpiryWarningSeconds() const { return FMath::Clamp(DormantExpiryWarningSeconds, 0.0f, GetIdleLifetimeSeconds()); }
	float GetDormantSpentSeconds() const { return FMath::Max(0.01f, DormantSpentVisualSeconds); }
	int32 GetDeviceLimit() const { return FMath::Max(1, MaxDevicesTotal); }
	int32 GetProjectileLimit() const { return FMath::Max(1, MaxLiveProjectiles); }
	int32 GetActiveLimit(EDeviceKind Kind) const
	{
		return Kind == EDeviceKind::Emitter ? FMath::Max(1, MaxActiveEmitters)
			: Kind == EDeviceKind::Launcher ? FMath::Max(1, MaxActiveColumns) : 0;
	}

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
	UPROPERTY(Config, EditDefaultsOnly, Category="Limits", meta=(ClampMin="1"))
	int32 MaxDevicesTotal = 24;
	UPROPERTY(Config, EditDefaultsOnly, Category="Limits", meta=(ClampMin="1"))
	int32 MaxActiveEmitters = 6;
	UPROPERTY(Config, EditDefaultsOnly, Category="Limits", meta=(ClampMin="1"))
	int32 MaxActiveColumns = 6;
	UPROPERTY(Config, EditDefaultsOnly, Category="Limits", meta=(ClampMin="1"))
	int32 MaxLiveProjectiles = 64;
	UPROPERTY(Config, EditDefaultsOnly, Category="Limits", meta=(ClampMin="0.01"))
	float DormantIdleLifetimeSeconds = 30.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Limits", meta=(ClampMin="0.0"))
	float DormantExpiryWarningSeconds = 5.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Limits", meta=(ClampMin="0.01"))
	float DormantSpentVisualSeconds = 0.25f;

private:
	struct FDeviceRecord
	{
		TWeakObjectPtr<ADeviceBase> Device;
		uint64 CreatedOrder = 0;
		uint64 ActiveOrder = 0;
		bool bActivationReserved = false;
	};
	TArray<FDeviceRecord> Devices;
	TArray<TWeakObjectPtr<ARuleProjectile>> Projectiles;
	TSet<TWeakObjectPtr<ADeviceBase>> ReservedSpawns;
	int32 PendingSpawns = 0;
	uint64 NextOrder = 0;
	uint64 LifecycleGeneration = 0;
	bool bResetting = false;
	bool bChangingCapacity = false;
	FBox SafetyBounds = FBox(ForceInit);
	double LastCapacityNotice[4] = { -1.0, -1.0, -1.0, -1.0 };
	bool MakeRoomForDevice();
	ADeviceBase* SpawnDeviceInternal(TSubclassOf<ADeviceBase> DeviceClass, FTransform Transform,
		uint64 Generation, TFunctionRef<bool()> ValidateBirthplace);
	bool IsOutsideSafety(const AActor* Actor) const;
	void NotifyRejected(EMothCapacityKind Kind);
};
