#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Interfaces/BallisticReactive.h"
#include "Interfaces/ImpulseReceiver.h"
#include "MothTutorialTarget.generated.h"

class UStaticMeshComponent;
class UHealthComponent;
class UExternalImpulseComponent;

/** World-local practice prop. Uses the same ballistic/health path as combat targets. */
UCLASS(NotPlaceable)
class MOTHEFFECT_API AMothTutorialTarget : public AActor, public IBallisticReactive, public IImpulseReceiver
{
	GENERATED_BODY()
public:
	AMothTutorialTarget();
	virtual bool ReceiveBallisticHit_Implementation(const FHitContext& Context) override;
	virtual void ReceiveImpulse_Implementation(FVector Velocity, bool bOverrideZ, FGuid EventId) override;
	UHealthComponent* GetHealthComponent() const { return Health; }
	UStaticMeshComponent* GetBody() const { return Body; }
	void EnableLiftPhysics();
protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UHealthComponent> Health;
	UPROPERTY() TObjectPtr<UExternalImpulseComponent> Impulse;
	UFUNCTION() void HandleDied(AActor* Victim, AActor* Source);
};

/** Passive character target: D02 intentionally damages characters only, never generic props. */
UCLASS(NotPlaceable)
class MOTHEFFECT_API AMothTutorialBlastTarget : public ACharacter, public IBallisticReactive
{
	GENERATED_BODY()
public:
	AMothTutorialBlastTarget();
	virtual bool ReceiveBallisticHit_Implementation(const FHitContext& Context) override;
	UHealthComponent* GetHealthComponent() const { return Health; }
protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UHealthComponent> Health;
	UFUNCTION() void HandleDied(AActor* Victim, AActor* Source);
};
