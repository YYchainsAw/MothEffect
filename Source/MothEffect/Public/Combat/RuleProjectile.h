#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/HitContext.h"
#include "RuleProjectile.generated.h"

class UProjectileMovementComponent;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class UProjectilePoolSubsystem;
class ARuleProjectile;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FMothProjectileImpactNative,
	ARuleProjectile*, AActor*, const FHitContext&);

/** P01: straight swept flight, one blocking impact, no allegiance filtering. */
UCLASS(Blueprintable)
class MOTHEFFECT_API ARuleProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARuleProjectile();
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Call between SpawnActorDeferred and FinishSpawning. Direction is world space. */
	UFUNCTION(BlueprintCallable, Category="Projectile")
	bool InitializeProjectile(const FVector& WorldDirection, AActor* SourceActor, APawn* InstigatorPawn);

	/** Also handles a blocking hit on the emitter-to-muzzle segment at birth. */
	bool ProcessBlockingHit(const FHitResult& Hit);

	/** Pooled instances are dormant between shots; direct spawns retain Destroy semantics. */
	UFUNCTION(BlueprintPure, Category="Projectile|Pool")
	bool IsPoolInactive() const { return bPoolInactive; }
	UFUNCTION(BlueprintCallable, Category="Projectile|Pool")
	void ReturnToPool();
	/** Detach a recycled firing actor's old identity without stopping an in-flight shot. */
	void ForgetSourceActor(AActor* Actor);

	UFUNCTION(BlueprintPure, Category="Projectile")
	bool HasProcessedImpact() const { return bImpactProcessed; }

	UFUNCTION(BlueprintPure, Category="Projectile")
	FGuid GetHitId() const { return HitId; }

	UFUNCTION(BlueprintPure, Category="Projectile")
	FVector GetFlightDirection() const { return FlightDirection; }

	UFUNCTION(BlueprintPure, Category="Projectile")
	float GetCollisionRadiusCm() const;

	/** Development evidence follows the firing actor's logging setting. */
	void SetEventLoggingEnabled(bool bEnabled) { bLogProjectileEvents = bEnabled; }

	FMothProjectileImpactNative OnImpactNative;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void LifeSpanExpired() override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;
	virtual void OutsideWorldBounds() override;

	/** Presentation only; collision and the one-hit guard are committed before this event. */
	UFUNCTION(BlueprintImplementableEvent, Category="Projectile")
	void OnProjectileImpact(const FHitContext& Context, AActor* HitActor, bool bAccepted);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0.1", Units="cm"))
	float CollisionRadiusCm = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="1.0", Units="cm/s"))
	float SpeedCmPerSec = 1400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0.0"))
	float Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0.01", Units="s"))
	float LifetimeSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category="Projectile|Debug")
	bool bLogProjectileEvents = false;

private:
	friend class UProjectilePoolSubsystem;
	void PreparePoolActivation(const FTransform& Transform, AActor* SourceActor, APawn* InstigatorPawn, bool bLogEvents);
	void StopFlight();
	void DeactivateForPool();
	bool StartFlight();
	bool IsActivationCurrent() const;

	UFUNCTION()
	void HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void HandleMovementStopped(const FHitResult& Hit);

	FGuid HitId;
	FVector FlightDirection = FVector::ZeroVector;
	TWeakObjectPtr<AActor> FiringSource;
	TWeakObjectPtr<APawn> FiringInstigator;
	TWeakObjectPtr<UProjectilePoolSubsystem> OwningPool;
	uint64 ActivationGeneration = 0;
	bool bFiredByDevice = false;
	bool bInitialized = false;
	bool bImpactProcessed = false;
	bool bEndingPlay = false;
	bool bPoolInactive = false;
	bool bPoolActivationInProgress = false;
	bool bImpactDispatching = false;
	bool bReturnPending = false;
};
