#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/BallisticReactive.h"
#include "Types/DeviceTypes.h"
#include "DeviceBase.generated.h"

class AMothEffectCharacter;
class USphereComponent;
class UStaticMeshComponent;
class ADeviceBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMothDeviceStateChanged,
	ADeviceBase*, Device, EDeviceState, OldState, EDeviceState, NewState);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FMothDeviceStateChangedNative,
	ADeviceBase*, EDeviceState, EDeviceState);

/** Shared device state and ballistic entry point. Effects are added in T08/T10/T11. */
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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Native hooks keep gameplay effects out of Blueprint presentation callbacks. */
	virtual void ActivateEffect(const FHitContext& Context);
	virtual void StopEffect();
	bool TransitionTo(EDeviceState NewState);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> PhysicsBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> ShotCollider;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> DeviceMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Device")
	EDeviceKind DeviceKind = EDeviceKind::Emitter;

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

	void ApplyStateCollision();
	/** Called by the character only after T07's pickup/release safety checks pass. */
	bool CommitHeld(AMothEffectCharacter* NewHolder);
	bool CommitReleased(AMothEffectCharacter* ReleasingHolder);
	void StopActiveEffect();
	void LogHit(const FHitContext& Context, bool bAccepted) const;
};
