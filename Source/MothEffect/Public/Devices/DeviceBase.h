#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/BallisticReactive.h"
#include "TimerManager.h"
#include "Types/DeviceTypes.h"
#include "DeviceBase.generated.h"

class AMothEffectCharacter;
class USphereComponent;
class UStaticMeshComponent;
class ADeviceBase;
class USceneComponent;
class UArrowComponent;
class ARuleProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMothDeviceStateChanged,
	ADeviceBase*, Device, EDeviceState, OldState, EDeviceState, NewState);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FMothDeviceStateChangedNative,
	ADeviceBase*, EDeviceState, EDeviceState);

/** Shared device state and ballistic entry point; D03 is native, D01/D02 follow later. */
UCLASS(Blueprintable)
class MOTHEFFECT_API ADeviceBase : public AActor, public IBallisticReactive
{
	GENERATED_BODY()

public:
	ADeviceBase();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual bool ReceiveBallisticHit_Implementation(const FHitContext& Context) override;

	UPROPERTY(BlueprintAssignable, Category="Device")
	FMothDeviceStateChanged OnDeviceStateChanged;

	/** Native gameplay observers use the same committed transitions as Blueprint presentation. */
	FMothDeviceStateChangedNative OnDeviceStateChangedNative;

	UFUNCTION(BlueprintCallable, Category="Device")
	bool TryActivate(const FHitContext& Context);

	/** Finish a committed effect once; Spent never returns to Dormant. */
	UFUNCTION(BlueprintCallable, Category="Device")
	bool FinishActivation();

	UFUNCTION(BlueprintCallable, Category="Device")
	void DestroyDevice();

	UFUNCTION(BlueprintCallable, Category="Device")
	void SetGameplayEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Device")
	EDeviceState GetDeviceState() const { return DeviceState; }

	UFUNCTION(BlueprintPure, Category="Device")
	EDeviceKind GetDeviceKind() const { return DeviceKind; }

	/** Device-side eligibility only; T07 also checks distance, line of sight and player action. */
	UFUNCTION(BlueprintPure, Category="Device")
	bool CanBePickedUp() const;

	/** Uses the character's complete distance/visibility/action validation. */
	UFUNCTION(BlueprintCallable, Category="Device")
	bool TryPickup(AMothEffectCharacter* NewHolder);

	UFUNCTION(BlueprintPure, Category="Device")
	AMothEffectCharacter* GetHolder() const { return Holder.Get(); }

	UFUNCTION(BlueprintPure, Category="Device")
	FGuid GetActivationHitId() const { return ActivationHitId; }

	UFUNCTION(BlueprintPure, Category="Device")
	FVector GetActivationDirection() const { return ActivationDirection; }

	UFUNCTION(BlueprintPure, Category="Device")
	FVector GetActivationImpactPoint() const { return ActivationImpactPoint; }

	UFUNCTION(BlueprintPure, Category="Device")
	USphereComponent* GetPhysicsBody() const { return PhysicsBody.Get(); }

	UFUNCTION(BlueprintPure, Category="Device|Emitter")
	UArrowComponent* GetEmitterDirectionMarker() const { return EmitterDirectionMarker.Get(); }

	UFUNCTION(BlueprintPure, Category="Device|Emitter")
	int32 GetEmitterShotAttempts() const { return EmitterShotAttempts; }

	UFUNCTION(BlueprintPure, Category="Device|Emitter")
	int32 GetEmitterProjectilesSpawned() const { return EmitterProjectilesSpawned; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Native hooks keep gameplay effects out of Blueprint presentation callbacks. */
	virtual void ActivateEffect(const FHitContext& Context);
	virtual void StopEffect();
	bool TransitionTo(EDeviceState NewState, bool bNotify = true);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> PhysicsBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> ShotCollider;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> DeviceMesh;

	/** Absolute world rotation; follows body translation while ignoring its spin. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UArrowComponent> EmitterDirectionMarker;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device")
	EDeviceKind DeviceKind = EDeviceKind::Emitter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Emitter")
	TSubclassOf<ARuleProjectile> EmitterProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Emitter", meta=(ClampMin="0.01", Units="s"))
	float EmitterDurationSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Emitter", meta=(ClampMin="0.0", Units="s"))
	float EmitterFirstShotDelaySeconds = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Emitter", meta=(ClampMin="0.01", Units="s"))
	float EmitterShotIntervalSeconds = 0.25f;

	/** Muzzle distance = actual body radius + projectile radius + this clearance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Emitter", meta=(ClampMin="0.0", Units="cm"))
	float EmitterMuzzleClearanceCm = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Emitter", meta=(ClampMin="0.01", Units="s"))
	float EmitterSpentVisualSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Physics", meta=(ClampMin="1.0", Units="cm"))
	float BodyRadiusCm = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Physics", meta=(ClampMin="1.0", Units="cm"))
	float ShotRadiusCm = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Physics", meta=(ClampMin="0.01", Units="kg"))
	float PhysicsMassKg = 2.0f;

	/** Matches the current rifle. Configure JamWeaponTrace together in a later collision pass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Collision")
	TEnumAsByte<ECollisionChannel> WeaponTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category="Device|Debug")
	bool bLogDeviceEvents = false;

private:
	// T07 commits ownership only after pickup/release safety checks succeed.
	friend class AMothEffectCharacter;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Device", meta=(AllowPrivateAccess="true"))
	EDeviceState DeviceState = EDeviceState::Dormant;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Device", meta=(AllowPrivateAccess="true"))
	TObjectPtr<AMothEffectCharacter> Holder;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Device", meta=(AllowPrivateAccess="true"))
	FGuid ActivationHitId;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Device", meta=(AllowPrivateAccess="true"))
	FVector ActivationDirection = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Device", meta=(AllowPrivateAccess="true"))
	FVector ActivationImpactPoint = FVector::ZeroVector;

	bool bGameplayEnabled = true;
	bool bEndingPlay = false;
	bool bEffectRunning = false;
	FTimerHandle EmitterShotTimer;
	FTimerHandle EmitterExpiryTimer;
	TWeakObjectPtr<APawn> EmitterInstigator;
	double EmitterStartedAt = 0.0;
	double EmitterExpiresAt = 0.0;
	int32 EmitterNextShotIndex = 0;
	int32 EmitterShotAttempts = 0;
	int32 EmitterProjectilesSpawned = 0;

	void ScheduleEmitterShot();
	void FireEmitterShot();
	void ExpireEmitter();
	void SpawnEmitterProjectile();

	void ApplyStateCollision();
	/** Called by the character only after T07's pickup/release safety checks pass. */
	bool CommitHeld(AMothEffectCharacter* NewHolder, USceneComponent* HoldPoint);
	bool CommitReleased(AMothEffectCharacter* ReleasingHolder, const FVector& Location, const FVector& Velocity);
	void NotifyStateChanged(EDeviceState OldState, EDeviceState NewState);
	void StopActiveEffect();
	void LogHit(const FHitContext& Context, bool bAccepted) const;
};
