#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Rifle.generated.h"

class AMothEffectCharacter;
class USceneComponent;
class UStaticMeshComponent;
class ARifle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMothRifleEvent, ARifle*, Rifle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMothReloadFinished, bool, bCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothAmmoChanged, int32, AmmoInMagazine, int32, MagazineSize);

UCLASS(Blueprintable)
class MOTHEFFECT_API ARifle : public AActor
{
	GENERATED_BODY()

public:
	ARifle();

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FMothRifleEvent OnShotFired;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FMothRifleEvent OnReloadStarted;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FMothReloadFinished OnReloadFinished;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FMothAmmoChanged OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FMothRifleEvent OnEmptyMagazine;

	UPROPERTY(BlueprintAssignable, Category="Weapon")
	FMothRifleEvent OnMuzzleBlocked;

	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool TryStartFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool TryBeginReload();

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void CancelReload();

	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	int32 GetMagazineSize() const { return MagazineSize; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool IsReloading() const { return bReloading; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	float GetReloadSeconds() const { return ReloadSeconds; }

	UFUNCTION(BlueprintPure, Category="Weapon")
	float GetReloadProgress() const;

	/** Shared camera target for the rifle and device throw; each uses its own origin. */
	bool GetAimTarget(FVector& AimPoint, FVector& ViewLocation, FVector& ViewDirection) const;
	ECollisionChannel GetWeaponTraceChannel() const { return WeaponTraceChannel; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Muzzle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.0"))
	float Damage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.01", Units="s"))
	float FireIntervalSeconds = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="1"))
	int32 MagazineSize = 24;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.01", Units="s"))
	float ReloadSeconds = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="1.0", Units="cm"))
	float RangeCm = 8000.0f;

	/** Visibility is the initial test-room channel; later assign JamWeaponTrace in BP. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace")
	TEnumAsByte<ECollisionChannel> WeaponTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace", meta=(ClampMin="0.1", Units="cm"))
	float MuzzleClearanceRadiusCm = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Debug")
	bool bDrawDebugShots = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapon")
	int32 AmmoInMagazine = 0;

private:
	AMothEffectCharacter* GetPlayerOwner() const;
	void HandleFireTimer();
	void ScheduleNextShot();
	void TryFireOneShot();
	void FinishReload(uint32 TaskId);

	FTimerHandle FireTimer;
	FTimerHandle ReloadTimer;
	double NextShotTime = 0.0;
	float ActiveReloadSeconds = 0.0f;
	uint32 ReloadTaskId = 0;
	bool bFireRequested = false;
	bool bReloading = false;
	bool bEndingPlay = false;
};
