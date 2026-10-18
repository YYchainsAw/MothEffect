#include "Art/MothArenaVisualDirector.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AMothArenaVisualDirector::AMothArenaVisualDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("ArenaPostProcess"));
	SetRootComponent(PostProcessComponent);
	PostProcessComponent->bUnbound = true;
	PostProcessComponent->bEnabled = false;
	PostProcessComponent->Priority = PostProcessPriority;
}

bool AMothArenaVisualDirector::IsTargetMap() const
{
	return GetWorld() && UGameplayStatics::GetCurrentLevelName(this, true) == TargetMapName.ToString();
}

void AMothArenaVisualDirector::RememberLight(ULightComponentBase* Light)
{
	if (Light && !OriginalIntensities.Contains(Light))
	{
		OriginalIntensities.Add(Light, Light->Intensity);
	}
}

void AMothArenaVisualDirector::ApplyLook()
{
	if (!bEnabled || !IsTargetMap())
	{
		RestoreLook();
		return;
	}

	UDirectionalLightComponent* Sun = nullptr;
	TArray<UDirectionalLightComponent*> FillLights;
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		if (It->IsActorBeingDestroyed()) continue;
		UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
		if (!Light || !Light->IsVisible()) continue;
		if (!Light->CastShadows)
		{
			FillLights.Add(Light);
		}
		else
		{
			// Compare the original intensity so applying the look repeatedly selects the same sun.
			const float* Original = OriginalIntensities.Find(Light);
			const float CandidateIntensity = Original ? *Original : Light->Intensity;
			const float* OriginalSun = Sun ? OriginalIntensities.Find(Sun) : nullptr;
			const float CurrentSunIntensity = Sun ? (OriginalSun ? *OriginalSun : Sun->Intensity) : -1.0f;
			if (CandidateIntensity > CurrentSunIntensity) Sun = Light;
		}
	}
	if (Sun)
	{
		RememberLight(Sun);
		Sun->SetIntensity(FMath::Max(0.0f, SunIntensity));
	}
	for (UDirectionalLightComponent* Fill : FillLights)
	{
		RememberLight(Fill);
		Fill->SetIntensity(FMath::Max(0.0f, FillIntensity));
	}

	USkyLightComponent* Sky = nullptr;
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (It->IsActorBeingDestroyed()) continue;
		USkyLightComponent* Light = It->GetLightComponent();
		if (!Light || !Light->IsVisible()) continue;
		const float* Original = OriginalIntensities.Find(Light);
		const float CandidateIntensity = Original ? *Original : Light->Intensity;
		const float* OriginalSky = Sky ? OriginalIntensities.Find(Sky) : nullptr;
		const float CurrentSkyIntensity = Sky ? (OriginalSky ? *OriginalSky : Sky->Intensity) : -1.0f;
		if (CandidateIntensity > CurrentSkyIntensity) Sky = Light;
	}
	if (Sky)
	{
		RememberLight(Sky);
		Sky->SetIntensity(FMath::Max(0.0f, SkyIntensity));
	}

	FPostProcessSettings& Settings = PostProcessComponent->Settings;
	Settings = FPostProcessSettings();
	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = AEM_Manual;
	Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Settings.AutoExposureApplyPhysicalCameraExposure = false;
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = FMath::Clamp(ExposureCompensation, -15.0f, 15.0f);
	Settings.bOverride_ColorSaturation = true;
	const float SafeSaturation = FMath::Clamp(Saturation, 0.0f, 2.0f);
	Settings.ColorSaturation = FVector4(SafeSaturation, SafeSaturation, SafeSaturation, 1.0f);
	Settings.bOverride_ColorContrast = true;
	const float SafeContrast = FMath::Clamp(Contrast, 0.0f, 2.0f);
	Settings.ColorContrast = FVector4(SafeContrast, SafeContrast, SafeContrast, 1.0f);
	Settings.bOverride_ColorGammaShadows = true;
	const float SafeShadowGamma = FMath::Clamp(ShadowGamma, 0.01f, 2.0f);
	Settings.ColorGammaShadows = FVector4(SafeShadowGamma, SafeShadowGamma, SafeShadowGamma, 1.0f);
	Settings.bOverride_BloomIntensity = true;
	Settings.BloomIntensity = FMath::Max(0.0f, BloomIntensity);
	Settings.bOverride_MotionBlurAmount = true;
	Settings.MotionBlurAmount = 0.0f;
	Settings.bOverride_SceneFringeIntensity = true;
	Settings.SceneFringeIntensity = 0.0f;
	const bool bPriorityChanged = PostProcessComponent->Priority != PostProcessPriority;
	PostProcessComponent->Priority = PostProcessPriority;
	PostProcessComponent->BlendWeight = 1.0f;
	PostProcessComponent->bUnbound = true;
	PostProcessComponent->bEnabled = true;
	if (bPriorityChanged && PostProcessComponent->IsRegistered())
	{
		// The world sorts its post process list on registration, not when Priority is assigned.
		GetWorld()->RemovePostProcessVolume(PostProcessComponent);
		GetWorld()->AddPostProcessVolume(PostProcessComponent);
	}
	bLookApplied = true;
}

void AMothArenaVisualDirector::RestoreLook()
{
	if (PostProcessComponent) PostProcessComponent->bEnabled = false;
	for (const TPair<TWeakObjectPtr<ULightComponentBase>, float>& Entry : OriginalIntensities)
	{
		if (ULightComponentBase* Light = Entry.Key.Get())
		{
			if (USkyLightComponent* Sky = Cast<USkyLightComponent>(Light)) Sky->SetIntensity(Entry.Value);
			else if (UDirectionalLightComponent* Directional = Cast<UDirectionalLightComponent>(Light)) Directional->SetIntensity(Entry.Value);
		}
	}
	OriginalIntensities.Empty();
	bLookApplied = false;
}

void AMothArenaVisualDirector::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!bEnabled) RestoreLook();
}

void AMothArenaVisualDirector::BeginPlay()
{
	Super::BeginPlay();
	ApplyLook();
}

void AMothArenaVisualDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreLook();
	Super::EndPlay(EndPlayReason);
}

void AMothArenaVisualDirector::Destroyed()
{
	RestoreLook();
	Super::Destroyed();
}
