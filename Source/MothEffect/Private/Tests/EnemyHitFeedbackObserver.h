#pragma once

#include "CoreMinimal.h"
#include "Types/EnemyHitFeedback.h"
#include "UObject/Object.h"
#include "EnemyHitFeedbackObserver.generated.h"

class APawn;
class AActor;

/** Transient receiver exercises the actual Blueprint-facing multicast delegate. */
UCLASS(Transient, NotBlueprintable)
class UMothEnemyHitFeedbackTestObserver : public UObject
{
	GENERATED_BODY()
public:
	virtual bool IsEditorOnly() const override { return true; }
	TArray<FMothEnemyHitFeedback> Results;
	TArray<TWeakObjectPtr<APawn>> Instigators;
	int32 HealthNotifications = 0;
	int32 DeathNotifications = 0;
	float NotifiedHealth = 0.0f;
	UFUNCTION()
	void HandleEnemyHitConfirmed(APawn* Instigator, const FMothEnemyHitFeedback& Result);
	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);
	UFUNCTION()
	void HandleDied(AActor* Victim, AActor* Source);
};
