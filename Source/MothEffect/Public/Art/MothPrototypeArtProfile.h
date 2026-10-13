#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/DeviceTypes.h"
#include "MothPrototypeArtProfile.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;

/** Mesh-space presentation only; never changes the owning actor's physics transform. */
USTRUCT(BlueprintType)
struct MOTHEFFECT_API FMothPrototypeAppearance
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TSoftObjectPtr<UMaterialInterface>> Materials;
	/** Materials are Metal/Copper/Glow roles when true; false copies a legacy visual's slot order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay) bool bNamedMaterialSlots = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform RelativeTransform = FTransform::Identity;
};

/** Replaceable T20 prototype assets; combat values remain in their gameplay classes. */
UCLASS(BlueprintType)
class MOTHEFFECT_API UMothPrototypeArtProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	UMothPrototypeArtProfile();
	static const TCHAR* DefaultPath();
	static void ApplyMaterials(UStaticMeshComponent* Component, const FMothPrototypeAppearance& Style);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Devices") bool bDeviceAppearance = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Devices") TMap<EDeviceKind, FMothPrototypeAppearance> Devices;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Scene") bool bCoverArt = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Scene") FMothPrototypeAppearance Cover;
	/** Excludes large perimeter walls that share the graybox cover material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Scene", meta=(Units="cm")) FVector CoverSelectionMaxExtentCm = FVector(300,300,200);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Scene") bool bRuneArt = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Scene") FMothPrototypeAppearance Rune;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Scene") TArray<FTransform> RuneTransforms;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Scene", meta=(ClampMin="0", ClampMax="2000")) float RuneLightIntensity = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prototype|Scene", meta=(ClampMin="0", ClampMax="800", Units="cm")) float RuneLightRadiusCm = 300.f;
};
