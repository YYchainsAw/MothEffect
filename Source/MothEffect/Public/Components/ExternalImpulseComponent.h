#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExternalImpulseComponent.generated.h"

/** Shared D02/D01 velocity settlement for CharacterMovement and a simulating physics root. */
UCLASS(ClassGroup=(MothEffect), meta=(BlueprintSpawnableComponent))
class MOTHEFFECT_API UExternalImpulseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UExternalImpulseComponent();
	bool QueueImpulse(const FVector& Velocity, bool bOverrideZ, const FGuid& EventId);
	void SetGameplayEnabled(bool bEnabled);
	void ClearPending();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Impulse", meta=(ClampMin="1.0", Units="cm/s"))
	float MaxExternalVelocityCmPerSec = 1600.0f;

private:
	FVector PendingVelocity = FVector::ZeroVector;
	float PendingZ = 0.0f;
	bool bHasOverrideZ = false;
	bool bGameplayEnabled = true;
	TSet<FGuid> AcceptedEvents;
	FDelegateHandle WorldTickHandle;

	bool CanReceive() const;
	void SettleImpulses(UWorld* World, ELevelTick TickType, float DeltaSeconds);
};
