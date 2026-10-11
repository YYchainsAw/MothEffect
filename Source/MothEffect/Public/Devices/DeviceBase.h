#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/BallisticReactive.h"
#include "Interfaces/ImpulseReceiver.h"
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
class UExternalImpulseComponent;
class ALaunchColumn;
class UTextRenderComponent;
class UDeviceLifecycleSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMothDeviceStateChanged,
	ADeviceBase*, Device, EDeviceState, OldState, EDeviceState, NewState);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FMothDeviceStateChangedNative,
	ADeviceBase*, EDeviceState, EDeviceState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothDormantExpiryWarning, ADeviceBase*, Device, float, RemainingSeconds);

/** Shared device state and ballistic entry point for D01/D02/D03. */
UCLASS(Blueprintable)
class MOTHEFFECT_API ADeviceBase : public AActor, public IBallisticReactive, public IImpulseReceiver
{
	GENERATED_BODY()

public:
	ADeviceBase();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual bool ReceiveBallisticHit_Implementation(const FHitContext& Context) override;
	virtual void ReceiveImpulse_Implementation(FVector Velocity, bool bOverrideZ, FGuid EventId) override;
	bool CanReceiveExternalImpulse() const;

	UFUNCTION(BlueprintPure, Category="Device|Bomb")
	FVector GetExplosionCenter() const { return ExplosionCenter; }

	/** Presentation only; native damage and impulse settlement already completed. */
	UFUNCTION(BlueprintImplementableEvent, Category="Device|Bomb")
	void OnBombExploded(FVector Center, float RadiusCm);

	UFUNCTION(BlueprintPure, Category="Device|Launcher")
	ALaunchColumn* GetLaunchColumn() const { return LaunchColumn.Get(); }
	UFUNCTION(BlueprintPure, Category="Device|Launcher")
	ELauncherFailureReason GetLauncherFailureReason() const { return LauncherFailureReason; }
	UFUNCTION(BlueprintPure, Category="Device|Launcher")
	FVector GetLauncherOrigin() const { return LauncherOrigin; }
	/** Presentation only; the invalid seed is already consumed. */
	UFUNCTION(BlueprintImplementableEvent, Category="Device|Launcher")
	void OnLauncherFailed(ELauncherFailureReason Reason);

	UPROPERTY(BlueprintAssignable, Category="Device")
	FMothDeviceStateChanged OnDeviceStateChanged;

	/** Native gameplay observers use the same committed transitions as Blueprint presentation. */
	FMothDeviceStateChangedNative OnDeviceStateChangedNative;
	UPROPERTY(BlueprintAssignable, Category="Device|Lifecycle")
	FMothDormantExpiryWarning OnDormantExpiryWarning;
	UFUNCTION(BlueprintPure, Category="Device|Lifecycle")
	float GetDormantIdleSeconds() const;
	UFUNCTION(BlueprintPure, Category="Device|Lifecycle")
	float GetRemainingIdleSeconds() const;
	UFUNCTION(BlueprintPure, Category="Device|Lifecycle")
	bool IsExpiryWarningActive() const { return bExpiryWarningActive; }

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UExternalImpulseComponent> ExternalImpulseComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> LauncherFeedback;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> LifecycleFeedback;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Launcher")
	TSubclassOf<ALaunchColumn> LauncherColumnClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Launcher", meta=(ClampMin="1.0", Units="cm"))
	float LauncherGroundProbeDistanceCm = 1500.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Launcher", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LauncherGroundUpNormalMin = 0.65f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Launcher")
	FName LauncherSurfaceTag = TEXT("MothColumnSurface");
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Launcher")
	bool bLauncherRequiresSurfaceTag = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Launcher", meta=(ClampMin="0.01", Units="s"))
	float LauncherSpentVisualSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device")
	EDeviceKind DeviceKind = EDeviceKind::Emitter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Bomb", meta=(ClampMin="1.0", Units="cm"))
	float BombRadiusCm = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Bomb", meta=(ClampMin="0.0"))
	float BombDamage = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Bomb", meta=(ClampMin="0.0", Units="cm/s"))
	float BombCharacterHorizontalVelocityAdd = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Bomb", meta=(ClampMin="0.0", Units="cm/s"))
	float BombCharacterUpVelocityAdd = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Bomb", meta=(ClampMin="0.0", Units="cm/s"))
	float BombDeviceRadialVelocityAdd = 1100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device|Bomb", meta=(ClampMin="0.01", Units="s"))
	float BombSpentVisualSeconds = 0.25f;

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

	UPROPERTY(EditAnywhere, Category="Device|Debug")
	bool bLogDeviceEvents = false;

private:
	// T07 commits ownership only after pickup/release safety checks succeed.
	friend class AMothEffectCharacter;
	friend class ALaunchColumn;
	friend class UDeviceLifecycleSubsystem;
	double DormantIdleElapsed = 0.0;
	double IdleSegmentStartedAt = 0.0;
	bool bExpiryWarningActive = false;
	void AccumulateIdleTime();
	void UpdateIdleLifecycle(float Lifetime, float WarningSeconds, float SpentSeconds);

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
	FVector ExplosionCenter = FVector::ZeroVector;
	FVector LauncherOrigin = FVector::ZeroVector;
	ELauncherFailureReason LauncherFailureReason = ELauncherFailureReason::None;
	UPROPERTY(Transient)
	TObjectPtr<ALaunchColumn> LaunchColumn;
	void StartLauncher();
	void FailLauncher(ELauncherFailureReason Reason);
	void OnLaunchColumnEnded(ALaunchColumn* Column);

	void ExplodeBomb(const FHitContext& Context);

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
