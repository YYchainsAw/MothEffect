#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "Types/HitContext.h"
#include "Types/DeviceTypes.h"
#include "MothTutorialSubsystem.generated.h"

class ADeviceBase;
class AMothEffectCharacter;
class AMothTutorialTarget;
class AMothTutorialBlastTarget;
class AMothWaveDirector;

UENUM(BlueprintType)
enum class EMothTutorialStep : uint8 { Inactive, BombAndLift, AirborneEmitter, IndirectChain, Completed };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMothTutorialChanged, EMothTutorialStep, Step);

/** Action-driven, world-local tutorial. Observers record evidence; transitions occur outside hit callbacks. */
UCLASS()
class MOTHEFFECT_API UMothTutorialSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	bool StartTutorial(AMothEffectCharacter* Player, AMothWaveDirector* Director);
	void EndTutorial();
	void RetryStep();
	EMothTutorialStep GetStep() const { return Step; }
	FText GetInstruction() const;
	bool IsRunning() const { return Step != EMothTutorialStep::Inactive && Step != EMothTutorialStep::Completed; }
	int32 GetOwnedActorCount() const;
	ADeviceBase* GetPracticeDevice(EDeviceKind Kind) const;
	UPROPERTY(BlueprintAssignable) FMothTutorialChanged OnStepChanged;
	virtual void OnWorldEndPlay(UWorld& World) override;
	virtual void Deinitialize() override;
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
private:
	TWeakObjectPtr<AMothEffectCharacter> Player;
	TWeakObjectPtr<AMothWaveDirector> Director;
	TArray<TWeakObjectPtr<AActor>> OwnedActors;
	TWeakObjectPtr<AMothTutorialBlastTarget> BlastTarget;
	TWeakObjectPtr<AMothTutorialTarget> LiftTarget;
	TWeakObjectPtr<ADeviceBase> ThrownEmitter;
	FTimerHandle CheckTimer;
	EMothTutorialStep Step = EMothTutorialStep::Inactive;
	float LiftStartZ = 0;
	bool bBombHit = false, bLiftHit = false, bThrown = false, bAirborneHit = false, bIndirectHit = false, bRetryAir = false;
	void BuildStep(EMothTutorialStep Next);
	void ClearOwnedActors();
	void CheckProgress();
	ADeviceBase* SpawnDevice(EDeviceKind Kind, FVector Location);
	AMothTutorialTarget* SpawnTarget(FVector Location, bool bPhysics);
	AMothTutorialBlastTarget* SpawnBlastTarget(FVector Location);
	void HandleActivation(ADeviceBase* Device, const FHitContext& Context);
	void HandleRelease(ADeviceBase* Device, bool bThrow);
	bool IsAirborne(ADeviceBase* Device) const;
};
