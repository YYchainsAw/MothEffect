#if WITH_DEV_AUTOMATION_TESTS
#include "Art/MothPrototypeSceneArt.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Devices/DeviceBase.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Framework/MothEffectPlayerController.h"
#include "Framework/MothWaveDirector.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UI/MothHUDWidget.h"
#include "UnrealClient.h"

namespace MothArtRuntime
{
	class FGallery final : public IAutomationLatentCommand
	{
	public:
		explicit FGallery(FAutomationTestBase* Test):T(Test) {}
		bool Update() override
		{
			UWorld* W=nullptr; for (const auto& C:GEngine->GetWorldContexts()) if (C.WorldType==EWorldType::Game) { W=C.World(); break; }
			if (!W || UGameplayStatics::GetCurrentLevelName(W,true)!=TEXT("L_Arena_01")) { T->AddError(TEXT("Run ArtRuntime in actual -game L_Arena_01")); return true; }
			auto* PC=Cast<AMothEffectPlayerController>(UGameplayStatics::GetPlayerController(W,0));
			if (!PC||!PC->GetGameplayUI()) return false;
			if (W->GetTimeSeconds()<WaitUntil) return false;
			if (Stage==0)
			{
				AMothWaveDirector* Director=nullptr; for (TActorIterator<AMothWaveDirector> It(W);It;++It) Director=*It;
				if (!Director||Director->GetPhase()!=EMothFlowPhase::Ready) { PC->ReturnToTitle(); return true; }
				AMothPrototypeSceneArt* Art=nullptr; int32 Layers=0; for (TActorIterator<AMothPrototypeSceneArt> It(W);It;++It) { Art=*It; ++Layers; }
				if (!T->TestNotNull(TEXT("GameMode supplies the main arena art layer"),Art)) return true;
				T->TestEqual(TEXT("Exactly one layer"),Layers,1); T->TestEqual(TEXT("All six low and three high covers dressed"),Art->GetCoverCount(),9);
				T->TestEqual(TEXT("Three configurable rune decorations"),Art->GetRuneCount(),3);
				FHitResult Hit; T->TestTrue(TEXT("Original high cover retains its actual ballistic blocker"),W->LineTraceSingleByChannel(Hit,FVector(-900,600,100),FVector(-300,600,100),ECC_Visibility)&&Hit.GetActor()->ActorHasTag(TEXT("T17Arena")));
				Art->bEnabled=false; Art->RefreshArt(); T->TestEqual(TEXT("Layer can be disabled without deleting gameplay cover"),Art->GetCoverCount(),0);
				Art->bEnabled=true; Art->RefreshArt(); T->TestEqual(TEXT("Re-enable recreates only nine visuals"),Art->GetCoverCount(),9);
				CoverCount=Art->GetCoverCount(); RuneCount=Art->GetRuneCount();
				PC->StartTeaching(); PC->GetGameplayUI()->SetVisibility(ESlateVisibility::Collapsed);
				if (APawn* Pawn=PC->GetPawn()) Pawn->SetActorHiddenInGame(true);
				FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				const TCHAR* Names[]={TEXT("Launcher"),TEXT("Bomb"),TEXT("Emitter")};
				for (int32 I=0; I<3; ++I)
				{
					auto* Class=LoadClass<ADeviceBase>(nullptr,*FString::Printf(TEXT("/Game/MothEffect/Devices/BP_Device%s.BP_Device%s_C"),Names[I],Names[I]));
					auto* Device=W->SpawnActor<ADeviceBase>(Class,FVector(-2320+I*120,-2050,35),FRotator::ZeroRotator,Params);
					if (!T->TestNotNull(TEXT("Saved device with original art"),Device)) return true;
					Devices.Add(Device); T->TestTrue(TEXT("Actual actor uses the shared resolved appearance"),Device->GetDeviceMesh()->GetStaticMesh()==Device->GetVisualAppearance().Mesh.LoadSynchronous());
				}
				Camera=W->SpawnActor<ACameraActor>(); Camera->GetCameraComponent()->FieldOfView=65;
				PC->SetViewTarget(Camera.Get()); View(FVector(-2200,-2320,145),FRotator(-25,90,0));
				WaitUntil=W->GetTimeSeconds()+1; ++Stage; return false;
			}
			if (Stage==1) { Capture(TEXT("01_DeviceShells.png")); WaitUntil=W->GetTimeSeconds()+.3; ++Stage; return false; }
			if (Stage==2) { View(FVector(-600,200,170),FRotator(-11,90,0)); WaitUntil=W->GetTimeSeconds()+.4; ++Stage; return false; }
			if (Stage==3) { Capture(TEXT("02_CoverModule.png")); WaitUntil=W->GetTimeSeconds()+.3; ++Stage; return false; }
			if (Stage==4) { View(FVector(0,2050,190),FRotator(2,90,0)); WaitUntil=W->GetTimeSeconds()+.4; ++Stage; return false; }
			if (Stage==5) { Capture(TEXT("03_RuneBeacon.png")); WaitUntil=W->GetTimeSeconds()+.3; ++Stage; return false; }
			for (const auto& Device:Devices) if (Device.IsValid()) Device->Destroy();
			if (Camera.IsValid()) Camera->Destroy(); PC->GetGameplayUI()->SetVisibility(ESlateVisibility::Visible);
			T->AddInfo(FString::Printf(TEXT("T20 runtime: actual saved device appearances=%d coverVisuals=%d runes=%d layers=1; main arena blocker and reversible layer verified"),Devices.Num(),CoverCount,RuneCount));
			PC->ReturnToTitle(); return true;
		}
	private:
		FAutomationTestBase* T; int32 Stage=0,CoverCount=0,RuneCount=0; double WaitUntil=0;
		TWeakObjectPtr<ACameraActor> Camera; TArray<TWeakObjectPtr<ADeviceBase>> Devices;
		void View(FVector Position,FRotator Rotation) { Camera->SetActorLocationAndRotation(Position,Rotation); }
		void Capture(const TCHAR* Name)
		{ if (FParse::Param(FCommandLine::Get(),TEXT("MothArtScreens"))) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Automation/T20/Screens")/Name,false,false); }
	};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArtRuntime,"MothEffect.ArtRuntime.ArenaLayerAndVisualGallery",EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FMothArtRuntime::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(MothArtRuntime::FGallery(this)); return true; }
#endif
