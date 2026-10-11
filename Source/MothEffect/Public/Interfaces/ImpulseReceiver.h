#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ImpulseReceiver.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UImpulseReceiver : public UInterface
{
	GENERATED_BODY()
};

/** Velocity events are accumulated until the end of this world's actor tick. */
class MOTHEFFECT_API IImpulseReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Combat|Impulse")
	void ReceiveImpulse(FVector Velocity, bool bOverrideZ, FGuid EventId);
};
