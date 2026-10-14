#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Feedback/MothFeedbackProfile.h"
#include "MothFeedbackSubsystem.generated.h"

class AMothFeedbackVisual; class UAudioComponent;

/** World-owned, bounded presentation. No retained combat actors and no gameplay writes. */
UCLASS(Config=Game)
class MOTHEFFECT_API UMothFeedbackSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldEndPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	/** Native callers emit only after the corresponding gameplay action commits. */
	void Emit(EMothFeedbackCue Kind, FVector Position, FVector Direction=FVector::ForwardVector,
		bool bLocalConfirmedHit=false, bool bLocalDamage=false, float RadiusCm=0);
	UFUNCTION(BlueprintCallable, Category="Moth|Feedback") void ClearFeedback();
	UFUNCTION(BlueprintPure, Category="Moth|Feedback") UMothFeedbackProfile* GetProfile() const { return Profile; }
	UFUNCTION(BlueprintPure, Category="Moth|Feedback") float GetHitMarkerAlpha() const;
	UFUNCTION(BlueprintPure, Category="Moth|Feedback") float GetDamageEdgeAlpha() const;
	int32 GetLiveVisualCount() const;
	int32 GetLiveAudioCount() const;
	uint64 GetCueCount(EMothFeedbackCue Kind) const { return Counts.FindRef(Kind); }
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
	UPROPERTY(Config, EditDefaultsOnly, Category="Feedback") TSoftObjectPtr<UMothFeedbackProfile> DefaultProfile;
private:
	UPROPERTY(Transient) TObjectPtr<UMothFeedbackProfile> Profile;
	TArray<TWeakObjectPtr<AMothFeedbackVisual>> Visuals;
	TArray<TWeakObjectPtr<UAudioComponent>> Audio;
	TMap<EMothFeedbackCue,double> LastSound;
	TMap<EMothFeedbackCue,uint64> Counts;
	double LastHit=-100, LastDamage=-100;
	uint64 PresentationGeneration=0;
	bool bClosed=false, bAudioPaused=false, bClearing=false;
};
