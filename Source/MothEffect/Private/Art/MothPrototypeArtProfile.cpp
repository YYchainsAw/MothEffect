#include "Art/MothPrototypeArtProfile.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

const TCHAR* UMothPrototypeArtProfile::DefaultPath()
{ return TEXT("/Game/MothEffect/PrototypeArt/DA_PrototypeArt.DA_PrototypeArt"); }

void UMothPrototypeArtProfile::ApplyMaterials(UStaticMeshComponent* Component, const FMothPrototypeAppearance& Style)
{
	const FName Roles[] { TEXT("Metal"), TEXT("Copper"), TEXT("Glow") };
	Component->EmptyOverrideMaterials();
	for (int32 I=0; I<Style.Materials.Num(); ++I)
	{
		int32 Slot=Style.bNamedMaterialSlots && I<3 ? Component->GetMaterialIndex(Roles[I]) : I;
		if (Slot==INDEX_NONE) Slot=I;
		if (auto* Material=Style.Materials[I].LoadSynchronous()) Component->SetMaterial(Slot,Material);
	}
}

UMothPrototypeArtProfile::UMothPrototypeArtProfile()
{
	auto Appearance=[](const TCHAR* MeshName, const TCHAR* Metal, const TCHAR* Glow)
	{
		FMothPrototypeAppearance Out;
		Out.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("/Game/MothEffect/PrototypeArt/Meshes/%s.%s"), MeshName, MeshName)));
		for (const TCHAR* Name : { Metal, TEXT("MI_Copper"), Glow })
			Out.Materials.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("/Game/MothEffect/PrototypeArt/Materials/%s.%s"), Name, Name))));
		return Out;
	};
	Devices.Add(EDeviceKind::Launcher, Appearance(TEXT("SM_D01Launcher"), TEXT("MI_D01Metal"), TEXT("MI_D01Glow")));
	Devices.Add(EDeviceKind::Bomb, Appearance(TEXT("SM_D02Bomb"), TEXT("MI_D02Metal"), TEXT("MI_D02Glow")));
	Devices.Add(EDeviceKind::Emitter, Appearance(TEXT("SM_D03Emitter"), TEXT("MI_D03Metal"), TEXT("MI_D03Glow")));
	Cover = Appearance(TEXT("SM_CoverModule"), TEXT("MI_Metal"), TEXT("MI_RuneGlow"));
	Rune = Appearance(TEXT("SM_RuneBeacon"), TEXT("MI_Metal"), TEXT("MI_RuneGlow"));
	for (float X : {-2000.f, 0.f, 2000.f}) RuneTransforms.Add(FTransform(FRotator(0,-90,0), FVector(X,2420,160), FVector(2)));
}
