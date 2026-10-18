#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/HitContext.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothHealthChanged, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothDied, AActor*, Victim, AActor*, SourceActor);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMothDeathCommittedNative, AActor*, AActor*);

UCLASS(ClassGroup=(MothEffect), meta=(BlueprintSpawnableComponent))
class MOTHEFFECT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UPROPERTY(BlueprintAssignable, Category="Health")
	FMothHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FMothDied OnDied;
	/** Native session owners commit results before health/death presentation callbacks. */
	FMothDeathCommittedNative OnDeathCommittedNative;

	UFUNCTION(BlueprintCallable, Category="Health")
	bool ApplyHit(const FHitContext& Context);

	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	/** Accepted-hit snapshot, captured before native result observers can close/reset the session. */
	uint64 GetLastHitLifecycleGeneration() const { return LastHitLifecycleGeneration; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsAlive() const { return !bHasDied && CurrentHealth > 0.0f; }

	/** Configure an owning archetype before component BeginPlay; never heals a live actor. */
	bool ConfigureMaxHealth(float Value);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float CurrentHealth = 100.0f;

private:
	friend class AEnemyBase;
	friend class UEnemyPoolSubsystem;
	/** Only the enemy pool lifecycle can heal/rearm an existing health owner. */
	void ResetForPoolReuse(float Value);
	void SetPoolInactive(bool bInactive);
	TSet<FGuid> AcceptedHits;
	uint64 LastHitLifecycleGeneration = 0;
	uint64 PoolLifeGeneration = 0;
	int32 HitDispatchDepth = 0;
	bool bPoolInactive = false;
	bool bHasDied = false;
	bool bEndingPlay = false;
};
