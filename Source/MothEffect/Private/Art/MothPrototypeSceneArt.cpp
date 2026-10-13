#include "Art/MothPrototypeSceneArt.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

AMothPrototypeSceneArt::AMothPrototypeSceneArt()
{
	PrimaryActorTick.bCanEverTick=false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ArtRoot")));
	Profile=TSoftObjectPtr<UMothPrototypeArtProfile>(FSoftObjectPath(UMothPrototypeArtProfile::DefaultPath()));
}

void AMothPrototypeSceneArt::OnConstruction(const FTransform& Transform)
{ Super::OnConstruction(Transform); RefreshArt(); }
void AMothPrototypeSceneArt::BeginPlay()
{ Super::BeginPlay(); RefreshArt(); }
void AMothPrototypeSceneArt::EndPlay(const EEndPlayReason::Type Reason)
{ ClearArt(); Super::EndPlay(Reason); }

void AMothPrototypeSceneArt::ClearArt()
{
	for (int32 I=0; I<Covers.Num(); ++I) if (Covers[I].IsValid()) Covers[I]->SetVisibility(CoverVisibility[I]);
	Covers.Reset(); CoverVisibility.Reset(); RuneCount=0;
	for (const auto& Piece : Pieces) if (IsValid(Piece)) Piece->DestroyComponent();
	Pieces.Reset();
}

UStaticMeshComponent* AMothPrototypeSceneArt::AddVisual(const FMothPrototypeAppearance& Style, const FTransform& WorldTransform)
{
	UStaticMesh* Mesh=Style.Mesh.LoadSynchronous();
	if (!Mesh || !WorldTransform.IsValid() || !Style.RelativeTransform.IsValid()) return nullptr;
	auto* Piece=NewObject<UStaticMeshComponent>(this,NAME_None,RF_Transient);
	Piece->SetStaticMesh(Mesh); Piece->SetupAttachment(GetRootComponent());
	Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision); Piece->SetGenerateOverlapEvents(false);
	Piece->SetCanEverAffectNavigation(false);
	UMothPrototypeArtProfile::ApplyMaterials(Piece,Style);
	Piece->RegisterComponent(); Piece->SetWorldTransform(Style.RelativeTransform * WorldTransform); Pieces.Add(Piece);
	return Piece;
}

void AMothPrototypeSceneArt::RefreshArt()
{
	ClearArt();
	const auto* Settings=bEnabled ? Profile.LoadSynchronous() : nullptr;
	if (!Settings || !GetWorld()) return;
	if (Settings->bCoverArt)
		for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(TEXT("T17Arena")) || It->IsActorBeingDestroyed()) continue;
			auto* Original=It->GetStaticMeshComponent(); const auto* Material=Original->GetMaterial(0);
			if (!Material || (Material->GetFName()!=TEXT("MI_ArenaLowCover") && Material->GetFName()!=TEXT("MI_ArenaHighCover"))) continue;
			const auto* ExistingMesh=Original->GetStaticMesh().Get(); if (!ExistingMesh) continue;
			const FVector Extent=ExistingMesh->GetBoundingBox().GetExtent()*Original->GetComponentScale().GetAbs();
			const FVector Limit=Settings->CoverSelectionMaxExtentCm;
			if (Extent.X>Limit.X || Extent.Y>Limit.Y || Extent.Z>Limit.Z) continue;
			if (AddVisual(Settings->Cover,Original->GetComponentTransform()))
			{ Covers.Add(Original); CoverVisibility.Add(Original->IsVisible()); Original->SetVisibility(false); }
		}
	if (Settings->bRuneArt)
		for (const FTransform& Transform : Settings->RuneTransforms)
			if (auto* Rune=AddVisual(Settings->Rune,Transform))
			{
				++RuneCount;
				if (Settings->RuneLightIntensity<=0 || Settings->RuneLightRadiusCm<=0) continue;
				auto* Light=NewObject<UPointLightComponent>(this,NAME_None,RF_Transient);
				Light->SetupAttachment(Rune); Light->SetRelativeLocation(FVector(25,0,0));
				Light->SetIntensity(FMath::Clamp(Settings->RuneLightIntensity,0.f,2000.f));
				Light->SetAttenuationRadius(FMath::Clamp(Settings->RuneLightRadiusCm,0.f,800.f));
				Light->SetLightColor(FLinearColor(.15f,.75f,1.f)); Light->SetCastShadows(false);
				Light->RegisterComponent(); Pieces.Add(Light);
			}
}
