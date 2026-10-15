#pragma once

#include "CoreMinimal.h"
#include "EnemyHitFeedback.generated.h"

/** Committed enemy damage snapshot. No combat actor references survive in the payload. */
USTRUCT(BlueprintType)
struct MOTHEFFECT_API FMothEnemyHitFeedback
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Moth|Feedback")
	FGuid HitId;

	UPROPERTY(BlueprintReadOnly, Category="Moth|Feedback")
	int32 VictimId = 0;

	UPROPERTY(BlueprintReadOnly, Category="Moth|Feedback")
	float AppliedDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Moth|Feedback")
	float RemainingHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Moth|Feedback")
	bool bKilled = false;

	UPROPERTY(BlueprintReadOnly, Category="Moth|Feedback")
	FVector ImpactPoint = FVector::ZeroVector;

	/** Native consumers reject results from a run reset by an earlier multicast observer. */
	uint64 LifecycleGeneration = 0;
};
