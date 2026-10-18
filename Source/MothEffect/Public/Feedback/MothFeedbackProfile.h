#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MothFeedbackProfile.generated.h"

class USoundBase; class USoundAttenuation; class UMaterialInterface; class UNiagaraSystem;

UENUM(BlueprintType)
enum class EMothFeedbackCue : uint8
{
	Shot, Impact, Hit, Damage, Launcher, Bomb, Emitter, EmitterShot, EnemyShot, Expiry, Spent, Failure, Pickup
};

USTRUCT(BlueprintType)
struct FMothFeedbackStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Color = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<USoundBase> Sound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1")) float Volume = .5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.02", ClampMax="2")) float Duration = .15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="1000")) float SizeCm = 12;
	/** A configured Niagara system replaces the geometric cue; gameplay timing is unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara") TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara") FVector NiagaraScale = FVector::OneVector;
	/** Exact exposed LinearColor name; None probes User.MothColor then User.Color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara") FName NiagaraColorParameter = NAME_None;
	/** Optional exposed float controlling authored particle size, e.g. User.Global Scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara") FName NiagaraScaleParameter = NAME_None;
	/** Optional exposed vector for impact normal or emission direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara") FName NiagaraDirectionParameter = NAME_None;
	/** Applied in the committed direction's local space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara") FRotator NiagaraRotation = FRotator::ZeroRotator;
	/** World-time safety limit, including for accidentally looping systems. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara", meta=(ClampMin="0.05", ClampMax="10")) float NiagaraLifetimeSeconds = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara") bool bScaleNiagaraToRadius = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara", meta=(ClampMin="1")) float NiagaraReferenceRadiusCm = 100.f;
	/** Retain geometry only when no usable Niagara system is configured. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara") bool bLegacyVisual = true;
	/** Limits repeated audio per category, without suppressing committed visual feedback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float AudioInterval = .06f;
};

/** Presentation values only: damage, collision, timing and activation rules live elsewhere. */
UCLASS(BlueprintType)
class MOTHEFFECT_API UMothFeedbackProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	UMothFeedbackProfile();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback") bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback") bool bWorldVisuals = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback") bool bAudio = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback") TMap<EMothFeedbackCue, FMothFeedbackStyle> Styles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback") TSoftObjectPtr<UMaterialInterface> Material;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback") TSoftObjectPtr<USoundAttenuation> Attenuation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback", meta=(ClampMin="1", ClampMax="64")) int32 MaxVisualCues = 24;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback", meta=(ClampMin="1", ClampMax="32")) int32 MaxAudioVoices = 16;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback", meta=(ClampMin="0", ClampMax="1")) float MasterVolume = .65f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback", meta=(ClampMin="0.02", ClampMax="1")) float HitMarkerSeconds = .14f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback", meta=(ClampMin="0.02", ClampMax="1")) float DamageEdgeSeconds = .25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feedback", meta=(ClampMin="0.1", ClampMax="8")) float EmissiveStrength = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Device Markers", meta=(ClampMin="1", ClampMax="200")) float MarkerHeightCm = 60;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Device Markers", meta=(ClampMin="1", ClampMax="200")) float MarkerSizeCm = 22;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Device Markers", meta=(ClampMin="10", ClampMax="250")) float DirectionLengthCm = 110;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Device Markers") FLinearColor SpentColor = FLinearColor(.25f,.28f,.32f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Device Markers") FLinearColor WarningColor = FLinearColor(1,.65f,.06f);
};
