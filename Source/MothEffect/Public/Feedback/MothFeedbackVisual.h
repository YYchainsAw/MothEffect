#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Feedback/MothFeedbackProfile.h"
#include "MothFeedbackVisual.generated.h"

class UStaticMeshComponent;

/** Shipping-visible geometric cues. Every primitive has collision and navigation disabled. */
UCLASS(Transient, NotBlueprintable, NotPlaceable)
class MOTHEFFECT_API AMothFeedbackVisual : public AActor
{
	GENERATED_BODY()
public:
	AMothFeedbackVisual();
	void InitializeCue(EMothFeedbackCue Kind, const FMothFeedbackStyle& Style, UMothFeedbackProfile* Profile, FVector Direction, float RadiusCm);
	virtual void Tick(float DeltaSeconds) override;
private:
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Pieces;
	EMothFeedbackCue Cue=EMothFeedbackCue::Impact;
	float Age=0, Duration=.15f, Size=12;
	void UpdateShape();
};
