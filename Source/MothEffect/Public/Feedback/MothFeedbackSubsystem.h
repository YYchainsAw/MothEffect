#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Feedback/MothFeedbackProfile.h"
#include "Types/EnemyHitFeedback.h"
#include "MothFeedbackSubsystem.generated.h"

class AMothFeedbackVisual; class UAudioComponent; class UNiagaraSystem;
class AActor;
class APawn;
struct FHitContext;

DECLARE_MULTICAST_DELEGATE_TwoParams(FMothEnemyHitConfirmedNative, APawn*, const FMothEnemyHitFeedback&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothEnemyHitConfirmed, APawn*, Instigator,
	const FMothEnemyHitFeedback&, Feedback);

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
	/** Accepted damage only; independent of world visual/audio styles and their budgets. */
	void ReportConfirmedEnemyHit(AActor* Victim, const FHitContext& Context,
		float AppliedDamage, float RemainingHealth, bool bKilled, uint64 LifecycleGeneration);
	/** Read-only consumers receive one snapshot for each accepted enemy damage result. */
	FMothEnemyHitConfirmedNative OnEnemyHitConfirmedNative;
	UPROPERTY(BlueprintAssignable, Category="Moth|Feedback")
	FMothEnemyHitConfirmed OnEnemyHitConfirmed;
	UFUNCTION(BlueprintCallable, Category="Moth|Feedback") void ClearFeedback();
	UFUNCTION(BlueprintPure, Category="Moth|Feedback") UMothFeedbackProfile* GetProfile() const { return Profile; }
	UFUNCTION(BlueprintPure, Category="Moth|Feedback") float GetHitMarkerAlpha() const;
	UFUNCTION(BlueprintPure, Category="Moth|Feedback") float GetDamageEdgeAlpha() const;
	int32 GetLiveVisualCount() const;
	UFUNCTION(BlueprintPure, Category="Moth|Feedback") int32 GetLiveNiagaraCount() const;
	int32 GetLiveAudioCount() const;
	uint64 GetCueCount(EMothFeedbackCue Kind) const { return Counts.FindRef(Kind); }
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
	UPROPERTY(Config, EditDefaultsOnly, Category="Feedback") TSoftObjectPtr<UMothFeedbackProfile> DefaultProfile;
private:
	UPROPERTY(Transient) TObjectPtr<UMothFeedbackProfile> Profile;
	/** Strong references avoid a load and GC churn on every committed shot. */
	UPROPERTY(Transient) TMap<FSoftObjectPath,TObjectPtr<UNiagaraSystem>> NiagaraSystems;
	TSet<FSoftObjectPath> FailedNiagaraSystems;
	TArray<TWeakObjectPtr<AMothFeedbackVisual>> Visuals;
	TArray<TWeakObjectPtr<UAudioComponent>> Audio;
	TMap<EMothFeedbackCue,double> LastSound;
	TMap<EMothFeedbackCue,uint64> Counts;
	double LastHit=-100, LastDamage=-100;
	uint64 PresentationGeneration=0;
	bool bClosed=false, bAudioPaused=false, bClearing=false;
	UNiagaraSystem* ResolveNiagara(const FMothFeedbackStyle& Style);
};
