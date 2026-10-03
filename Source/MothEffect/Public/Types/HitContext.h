#pragma once

#include "CoreMinimal.h"
#include "HitContext.generated.h"

class AActor;
class APawn;

USTRUCT(BlueprintType, meta=(DisplayName="ST_HitContext"))
struct MOTHEFFECT_API FHitContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	FGuid HitId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	TObjectPtr<APawn> InstigatorPawn = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	FVector ImpactPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	FVector ShotDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit")
	float Damage = 0.0f;
};
