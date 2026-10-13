#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Art/MothPrototypeArtProfile.h"
#include "MothPrototypeSceneArt.generated.h"

class UStaticMeshComponent;

/** Reversible render layer for the saved T17 arena; its original blockers still own collision. */
UCLASS(Blueprintable)
class MOTHEFFECT_API AMothPrototypeSceneArt : public AActor
{
	GENERATED_BODY()
public:
	AMothPrototypeSceneArt();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prototype") bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prototype") TSoftObjectPtr<UMothPrototypeArtProfile> Profile;
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Prototype") void RefreshArt();
	UFUNCTION(BlueprintPure, Category="Prototype") int32 GetCoverCount() const { return Covers.Num(); }
	UFUNCTION(BlueprintPure, Category="Prototype") int32 GetRuneCount() const { return RuneCount; }
	virtual void OnConstruction(const FTransform& Transform) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void ClearArt();
	UStaticMeshComponent* AddVisual(const FMothPrototypeAppearance& Style, const FTransform& WorldTransform);
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> Pieces;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Covers;
	TArray<bool> CoverVisibility;
	int32 RuneCount = 0;
};
