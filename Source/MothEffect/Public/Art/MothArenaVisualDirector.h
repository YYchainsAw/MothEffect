#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MothArenaVisualDirector.generated.h"

class ULightComponentBase;
class UPostProcessComponent;

/** Reversible lighting and grading for Soul City's arena. Does not own gameplay geometry. */
UCLASS(Config=Game, Blueprintable)
class MOTHEFFECT_API AMothArenaVisualDirector : public AActor
{
	GENERATED_BODY()

public:
	AMothArenaVisualDirector();

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look")
	bool bEnabled = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look")
	FName TargetMapName = TEXT("L_Arena_SoulCity");

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Lighting", meta=(ClampMin="0.0"))
	float SunIntensity = 2.9f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Lighting", meta=(ClampMin="0.0"))
	float SkyIntensity = 1.2f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Lighting", meta=(ClampMin="0.0"))
	float FillIntensity = 0.65f;

	/** Compensates the manual exposure path to preserve the previous fixed-exposure brightness. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Exposure", meta=(ClampMin="-15.0", ClampMax="15.0"))
	float ExposureCompensation = 1.263f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Grading", meta=(ClampMin="0.0", ClampMax="2.0"))
	float Saturation = 0.96f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Grading", meta=(ClampMin="0.0", ClampMax="2.0"))
	float Contrast = 0.98f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Grading", meta=(ClampMin="0.01", ClampMax="2.0"))
	float ShadowGamma = 1.06f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Grading", meta=(ClampMin="0.0"))
	float BloomIntensity = 0.2f;

	/** Map and camera effects can override this low priority baseline. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Arena Look|Grading")
	float PostProcessPriority = -10.0f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Arena Look")
	void ApplyLook();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="Arena Look")
	void RestoreLook();

	UFUNCTION(BlueprintPure, Category="Arena Look")
	UPostProcessComponent* GetPostProcessComponent() const { return PostProcessComponent; }

	UFUNCTION(BlueprintPure, Category="Arena Look")
	bool WasLookApplied() const { return bLookApplied; }

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool IsTargetMap() const;
	void RememberLight(ULightComponentBase* Light);

	UPROPERTY(VisibleAnywhere, Category="Arena Look")
	TObjectPtr<UPostProcessComponent> PostProcessComponent;

	// Weak references let teardown restore only surviving components without creating anything.
	TMap<TWeakObjectPtr<ULightComponentBase>, float> OriginalIntensities;
	bool bLookApplied = false;
};
