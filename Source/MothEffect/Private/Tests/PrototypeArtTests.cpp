#if WITH_DEV_AUTOMATION_TESTS
#include "Art/MothPrototypeArtProfile.h"
#include "Art/MothPrototypeSceneArt.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Devices/DeviceBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"
#include "Tests/CombatTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArtAssets,"MothEffect.Art.SavedMeshesMaterialsAndUnits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothArtAssets::RunTest(const FString&)
{
	const auto* Profile=LoadObject<UMothPrototypeArtProfile>(nullptr,UMothPrototypeArtProfile::DefaultPath());
	if (!TestNotNull(TEXT("Saved editable art profile"),Profile)) return false;
	TestEqual(TEXT("Three type appearances"),Profile->Devices.Num(),3);
	TArray<FMothPrototypeAppearance> Styles; Profile->Devices.GenerateValueArray(Styles); Styles.Add(Profile->Cover); Styles.Add(Profile->Rune);
	for (int32 I=0; I<Styles.Num(); ++I)
	{
		auto* Mesh=Styles[I].Mesh.LoadSynchronous(); if (!TestNotNull(TEXT("Original mesh reloads"),Mesh)) return false;
		TestEqual(TEXT("Three surface roles"),Mesh->GetStaticMaterials().Num(),3);
		TSet<FName> Roles; for (const auto& Material : Mesh->GetStaticMaterials()) Roles.Add(Material.MaterialSlotName);
		TestTrue(TEXT("Imported role names survive FBX slot reordering"),Roles.Contains(TEXT("Metal"))&&Roles.Contains(TEXT("Copper"))&&Roles.Contains(TEXT("Glow")));
		for (const auto& Material:Styles[I].Materials) TestNotNull(TEXT("Shared editable material instance loads"),Material.LoadSynchronous());
		if (auto* Body=Mesh->GetBodySetup()) TestEqual(TEXT("Art asset adds no simple collision"),Body->AggGeom.GetElementCount(),0);
		if (I<3)
		{
			const auto* Data=Mesh->GetRenderData(); if (!TestNotNull(TEXT("Saved mesh geometry available"),Data)||Data->LODResources.IsEmpty()) return false;
			const auto& Positions=Data->LODResources[0].VertexBuffers.PositionVertexBuffer;
			for (uint32 V=0; V<Positions.GetNumVertices(); ++V) if (Positions.VertexPosition(V).SizeSquared()>400.1f)
			{ AddError(TEXT("Imported device vertex escaped the 20cm native sphere")); break; }
		}
	}
	const FBox Cover=Profile->Cover.Mesh.LoadSynchronous()->GetBoundingBox();
	TestTrue(TEXT("Cover retains the exact old cube envelope"),Cover.Min.Equals(FVector(-50),.01)&&Cover.Max.Equals(FVector(50),.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArtDevice,"MothEffect.Art.AppearanceSwitchPreservesPhysicsAndUserEdits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothArtDevice::RunTest(const FString&)
{
	FMothCombatTestWorld F; auto* Device=F.Spawn<ADeviceBase>(FVector(0,0,150));
	const FTransform ActorTransform=Device->GetActorTransform(); const float Body=Device->GetPhysicsBody()->GetUnscaledSphereRadius();
	auto* Shot=FindObject<USphereComponent>(Device,TEXT("ShotCollider")); const float ShotRadius=Shot->GetUnscaledSphereRadius();
	Device->bUsePrototypeAppearance=false; Device->RefreshPrototypeAppearance();
	auto* Original=Device->GetDeviceMesh()->GetStaticMesh().Get(); const auto OriginalTransform=Device->GetDeviceMesh()->GetRelativeTransform();
	Device->bUsePrototypeAppearance=true; Device->RefreshPrototypeAppearance();
	TestTrue(TEXT("Prototype selects its saved visual"),Device->GetDeviceMesh()->GetStaticMesh()==Device->GetVisualAppearance().Mesh.LoadSynchronous());
	TestTrue(TEXT("Appearance cannot alter actor transform or physical/query sphere sizes"),Device->GetActorTransform().Equals(ActorTransform)&&Body==Device->GetPhysicsBody()->GetUnscaledSphereRadius()&&ShotRadius==Shot->GetUnscaledSphereRadius());
	TestTrue(TEXT("Appearance cannot participate in shots/nav/overlaps"),Device->GetDeviceMesh()->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!Device->GetDeviceMesh()->CanEverAffectNavigation()&&!Device->GetDeviceMesh()->GetGenerateOverlapEvents());
	Device->bUsePrototypeAppearance=false; Device->RefreshPrototypeAppearance();
	TestTrue(TEXT("Disabling prototype restores the previous visual"),Device->GetDeviceMesh()->GetStaticMesh()==Original&&Device->GetDeviceMesh()->GetRelativeTransform().Equals(OriginalTransform));
	Device->bUsePrototypeAppearance=true; Device->RefreshPrototypeAppearance();
	auto* UserMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")); Device->GetDeviceMesh()->SetStaticMesh(UserMesh);
	Device->bUsePrototypeAppearance=false; Device->RefreshPrototypeAppearance();
	TestTrue(TEXT("Direct user replacement wins over restoring a cached mesh"),Device->GetDeviceMesh()->GetStaticMesh()==UserMesh);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArtCover,"MothEffect.Art.ReversibleCoverLayerPreservesBlocking",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothArtCover::RunTest(const FString&)
{
	FMothCombatTestWorld F; auto* Cover=F.Spawn<AStaticMeshActor>(); Cover->Tags.Add(TEXT("T17Arena"));
	auto* Collider=Cover->GetStaticMeshComponent(); Collider->SetMobility(EComponentMobility::Movable);
	if (!TestTrue(TEXT("Fixture permits setting its mesh after BeginPlay"),Collider->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"))))) return false;
	Collider->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MothEffect/Maps/Materials/MI_ArenaLowCover.MI_ArenaLowCover")));
	Collider->SetCollisionProfileName(TEXT("BlockAll")); const FTransform Original=Collider->GetComponentTransform();
	auto* Layer=F.Spawn<AMothPrototypeSceneArt>(); TestEqual(TEXT("Recognizes saved material plus runtime tag"),Layer->GetCoverCount(),1);
	TestFalse(TEXT("Only original render geometry is hidden"),Collider->IsVisible());
	FHitResult Hit; TestTrue(TEXT("Hidden original cover still blocks actual shots"),F.World->LineTraceSingleByChannel(Hit,FVector(-100,0,0),FVector(100,0,0),ECC_Visibility)&&Hit.GetActor()==Cover);
	TArray<UStaticMeshComponent*> Meshes; Layer->GetComponents(Meshes);
	for (auto* Piece:Meshes) TestTrue(TEXT("Layer adds no collision/nav/overlaps"),Piece->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!Piece->CanEverAffectNavigation()&&!Piece->GetGenerateOverlapEvents());
	Layer->RefreshArt(); TestEqual(TEXT("Refresh does not duplicate covers"),Layer->GetCoverCount(),1); TestEqual(TEXT("Three configurable runes"),Layer->GetRuneCount(),3);
	Layer->bEnabled=false; Layer->RefreshArt(); TestTrue(TEXT("Disable restores visibility with identical collision transform"),Collider->IsVisible()&&Collider->GetComponentTransform().Equals(Original));
	Layer->bEnabled=true; Layer->RefreshArt(); Layer->Destroy(); TestTrue(TEXT("Destroy restores original rendering"),Collider->IsVisible());
	return true;
}
#endif
