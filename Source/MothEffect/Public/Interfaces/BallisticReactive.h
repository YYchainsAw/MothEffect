#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/HitContext.h"
#include "BallisticReactive.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UBallisticReactive : public UInterface
{
	GENERATED_BODY()
};

class MOTHEFFECT_API IBallisticReactive
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Combat")
	bool ReceiveBallisticHit(const FHitContext& Context);
};
