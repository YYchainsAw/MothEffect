#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WaveFlowObserver.generated.h"

class AMothWaveDirector;
class UHealthComponent;

/** Transient observer verifies actual Blueprint-facing health notification order. */
UCLASS(Transient, NotBlueprintable)
class UMothWaveFlowObserver : public UObject
{
	GENERATED_BODY()
public:
	virtual bool IsEditorOnly() const override { return true; }
	TWeakObjectPtr<AMothWaveDirector> Director;
	TWeakObjectPtr<UHealthComponent> OtherHealth;
	bool bNotified = false, bTerminal = false, bDamaged = false, bSpawned = false;
	UFUNCTION() void HealthChanged(float Health, float MaxHealth);
};
