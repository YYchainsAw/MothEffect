#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaunchColumn.generated.h"

class ADeviceBase;
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class ELaunchColumnPhase : uint8 { Rising, Holding, Retracting, Finished };

/** Fixed world-up column. Queries launch targets; a separate collider provides stationary support. */
UCLASS(Blueprintable)
class MOTHEFFECT_API ALaunchColumn : public AActor
{

	GENERATED_BODY()

public:
	ALaunchColumn();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	/** Called on the deferred actor before FinishSpawning; placed columns are preview-only. */
	bool InitializeColumn(ADeviceBase* Source);
	UFUNCTION(BlueprintCallable, Category="Column")
	void BeginRetraction();
	UFUNCTION(BlueprintPure, Category="Column")
	ELaunchColumnPhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintPure, Category="Column")
	bool IsSolidSupportEnabled() const;
	UFUNCTION(BlueprintPure, Category="Column")
	int32 GetAffectedActorCount() const { return AffectedActors.Num(); }
	UFUNCTION(BlueprintPure, Category="Column")
	float GetColumnHeightCm() const { return FMath::Max(1.0f, ColumnHeightCm); }
	UFUNCTION(BlueprintPure, Category="Column")
	float GetColumnRadiusCm() const { return FMath::Max(1.0f, ColumnRadiusCm); }
	float GetGroundContactToleranceCm() const { return FMath::Clamp(GroundContactToleranceCm, 0.01f, GetColumnHeightCm() * 0.5f); }
	float GetRetractSeconds() const { return FMath::Max(0.01f, RetractSeconds); }
	/** Uses the entire final box except the small bottom contact band; never ignores the support actor. */
	bool HasStaticClearance(UWorld* World, const FVector& Base, const AActor* IgnoredSource) const;
	UFUNCTION(BlueprintImplementableEvent, Category="Column")
	void OnColumnPresentationChanged(ELaunchColumnPhase NewPhase, bool bSolidSupport);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> ColumnRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> ColumnVisual;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> RiseQuery;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> SolidCollider;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> StageLabel;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Column", meta=(ClampMin="1.0", Units="cm"))
	float ColumnHeightCm = 350.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Column", meta=(ClampMin="1.0", Units="cm"))
	float ColumnRadiusCm = 70.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Column", meta=(ClampMin="0.01", Units="s"))
	float RiseSeconds = 0.2f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Column", meta=(ClampMin="0.01", Units="s"))
	float HoldSeconds = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Column", meta=(ClampMin="0.01", Units="s"))
	float RetractSeconds = 0.3f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Column", meta=(ClampMin="0.01", Units="cm"))
	float GroundContactToleranceCm = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Column", meta=(ClampMin="0.0", Units="cm/s"))
	float CharacterUpVelocityCmPerSec = 1000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Column", meta=(ClampMin="0.0", Units="cm/s"))
	float DeviceUpVelocityCmPerSec = 1200.0f;

private:
	TWeakObjectPtr<ADeviceBase> SourceDevice;
	TSet<TWeakObjectPtr<AActor>> AffectedActors;
	FGuid ImpulseEventId;
	ELaunchColumnPhase Phase = ELaunchColumnPhase::Rising;
	double Elapsed = 0.0;
	double RetractionElapsed = 0.0;
	float VisibleHeight = 0.0f;
	float RetractionStartHeight = 0.0f;
	bool bInitialized = false;
	bool bEndingPlay = false;
	void QueryLaunchTargets(float Height);
	bool HasDynamicClearance() const;
	void UpdateVisual(float Height);
	void UpdatePresentation();
	void DisableSolidSupport();
	void FinishColumn();
};
