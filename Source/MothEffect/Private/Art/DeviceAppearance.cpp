#include "Devices/DeviceBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

FMothPrototypeAppearance ADeviceBase::GetVisualAppearance() const
{
	if (bUsePrototypeAppearance)
		if (const auto* Profile = PrototypeArtProfile.LoadSynchronous(); Profile && Profile->bDeviceAppearance)
			if (const auto* Style = Profile->Devices.Find(DeviceKind); Style && Style->Mesh.LoadSynchronous()
				&& Style->RelativeTransform.IsValid() && Style->RelativeTransform.GetScale3D().GetMin() > 0)
				return *Style;
	FMothPrototypeAppearance Fallback;
	Fallback.bNamedMaterialSlots=false;
	Fallback.Mesh = bPrototypeApplied ? OriginalVisualMesh.Get() : DeviceMesh->GetStaticMesh().Get();
	Fallback.RelativeTransform = bPrototypeApplied ? OriginalVisualTransform : DeviceMesh->GetRelativeTransform();
	if (bPrototypeApplied) for (const auto& Material : OriginalVisualMaterials) Fallback.Materials.Add(Material.Get());
	else for (int32 I=0; I<DeviceMesh->GetNumMaterials(); ++I) Fallback.Materials.Add(DeviceMesh->GetMaterial(I));
	return Fallback;
}

void ADeviceBase::RefreshPrototypeAppearance()
{
	const auto* Profile = bUsePrototypeAppearance ? PrototypeArtProfile.LoadSynchronous() : nullptr;
	const auto* Style = Profile && Profile->bDeviceAppearance ? Profile->Devices.Find(DeviceKind) : nullptr;
	UStaticMesh* Mesh = Style ? Style->Mesh.LoadSynchronous() : nullptr;
	if (!Mesh || !Style->RelativeTransform.IsValid() || Style->RelativeTransform.GetScale3D().GetMin() <= 0)
	{
		if (bPrototypeApplied)
		{
			// A direct Designer edit takes precedence over restoring our cached graybox.
			bool bStillOurs = DeviceMesh->GetStaticMesh()==AppliedVisualMesh && DeviceMesh->GetRelativeTransform().Equals(AppliedVisualTransform);
			for (int32 I=0; I<AppliedVisualMaterials.Num(); ++I) bStillOurs &= DeviceMesh->GetMaterial(I)==AppliedVisualMaterials[I];
			if (bStillOurs)
			{
				DeviceMesh->SetStaticMesh(OriginalVisualMesh);
				DeviceMesh->EmptyOverrideMaterials();
				for (int32 I=0; I<OriginalVisualMaterials.Num(); ++I) DeviceMesh->SetMaterial(I,OriginalVisualMaterials[I]);
				DeviceMesh->SetRelativeTransform(OriginalVisualTransform);
			}
			bPrototypeApplied=false;
		}
		return;
	}
	if (!bPrototypeApplied)
	{
		OriginalVisualMesh=DeviceMesh->GetStaticMesh(); OriginalVisualTransform=DeviceMesh->GetRelativeTransform();
		OriginalVisualMaterials.Reset();
		for (int32 I=0; I<DeviceMesh->GetNumMaterials(); ++I) OriginalVisualMaterials.Add(DeviceMesh->GetMaterial(I));
	}
	DeviceMesh->SetStaticMesh(Mesh);
	UMothPrototypeArtProfile::ApplyMaterials(DeviceMesh,*Style);
	DeviceMesh->SetRelativeTransform(Style->RelativeTransform);
	AppliedVisualMesh=Mesh; AppliedVisualTransform=DeviceMesh->GetRelativeTransform(); AppliedVisualMaterials.Reset();
	for (int32 I=0; I<DeviceMesh->GetNumMaterials(); ++I) AppliedVisualMaterials.Add(DeviceMesh->GetMaterial(I));
	bPrototypeApplied=true;
}
