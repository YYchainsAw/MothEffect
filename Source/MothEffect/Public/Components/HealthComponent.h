#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/HitContext.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothHealthChanged, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothDied, AActor*, Victim, AActor*, SourceActor);

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

	UFUNCTION(BlueprintCallable, Category="Health")
	bool ApplyHit(const FHitContext& Context);

	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsAlive() const { return !bHasDied && CurrentHealth > 0.0f; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float CurrentHealth = 100.0f;

private:
	TSet<FGuid> AcceptedHits;
	bool bHasDied = false;
	bool bEndingPlay = false;
};
