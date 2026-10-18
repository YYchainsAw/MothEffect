#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Feedback/MothFeedbackProfile.h"
#include "MothFeedbackVisual.generated.h"

class UStaticMeshComponent; class UNiagaraComponent; class UNiagaraSystem;

/** Bounded world feedback owner. Destroying this actor also destroys its Niagara simulation. */
UCLASS(Transient, NotBlueprintable, NotPlaceable)
class MOTHEFFECT_API AMothFeedbackVisual : public AActor
{
	GENERATED_BODY()
public:
	AMothFeedbackVisual();
	EMothFeedbackCue GetCueKind() const { return Cue; }
	void InitializeCue(EMothFeedbackCue Kind, const FMothFeedbackStyle& Style, UMothFeedbackProfile* Profile, FVector Direction, float RadiusCm, UNiagaraSystem* System=nullptr);
	UFUNCTION(BlueprintPure, Category="Moth|Feedback") UNiagaraComponent* GetNiagaraComponent() const { return NiagaraComponent; }
	void SetCuePaused(bool bPaused);
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Pieces;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> NiagaraComponent;
	EMothFeedbackCue Cue=EMothFeedbackCue::Impact;
	float Age=0, Duration=.15f, Size=12;
	bool bDrawLegacyShape=false;
	void UpdateShape();
};
