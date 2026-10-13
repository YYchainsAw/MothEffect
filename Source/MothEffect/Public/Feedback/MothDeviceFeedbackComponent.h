#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MothDeviceFeedbackComponent.generated.h"

class UStaticMeshComponent; class UMaterialInstanceDynamic;

/** Type glyph, state pulse and a world-locked D03 arrow; never changes the device's body. */
UCLASS(ClassGroup=(Moth), meta=(BlueprintSpawnableComponent))
class MOTHEFFECT_API UMothDeviceFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMothDeviceFeedbackComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback") bool bEnabled = true;
	UFUNCTION(BlueprintCallable, Category="Feedback") void RefreshPresentation();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Pieces;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
};
