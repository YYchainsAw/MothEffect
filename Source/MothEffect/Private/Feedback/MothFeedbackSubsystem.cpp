#include "Feedback/MothFeedbackSubsystem.h"
#include "Feedback/MothFeedbackVisual.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Devices/DeviceBase.h"
#include "Feedback/MothDeviceFeedbackComponent.h"
#include "EngineUtils.h"

bool UMothFeedbackSubsystem::DoesSupportWorldType(EWorldType::Type Type) const { return Type==EWorldType::Game||Type==EWorldType::PIE; }
void UMothFeedbackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UDeviceLifecycleSubsystem>();
	if (DefaultProfile.IsNull()) DefaultProfile=TSoftObjectPtr<UMothFeedbackProfile>(FSoftObjectPath(TEXT("/Game/MothEffect/Feedback/DA_DefaultFeedback.DA_DefaultFeedback")));
	Profile=DefaultProfile.LoadSynchronous();
	if (!Profile) Profile=NewObject<UMothFeedbackProfile>(this);
}
void UMothFeedbackSubsystem::Deinitialize() { bClosed=true; ClearFeedback(); Super::Deinitialize(); }
void UMothFeedbackSubsystem::OnWorldEndPlay(UWorld& World) { bClosed=true; ClearFeedback(); Super::OnWorldEndPlay(World); }
TStatId UMothFeedbackSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UMothFeedbackSubsystem,STATGROUP_Tickables); }

void UMothFeedbackSubsystem::Tick(float)
{
	Visuals.RemoveAllSwap([](const auto& Item){ return !Item.IsValid()||Item->IsActorBeingDestroyed(); });
	const bool bPaused=GetWorld()->IsPaused();
	if (bPaused!=bAudioPaused)
	{
		bAudioPaused=bPaused;
		for (const auto& Item : Audio) if (Item.IsValid()) Item->SetPaused(bPaused);
	}
	Audio.RemoveAllSwap([this](const auto& Item){ return !Item.IsValid()||(!bAudioPaused&&!Item->IsPlaying()); });
}

void UMothFeedbackSubsystem::Emit(EMothFeedbackCue Kind, FVector Position, FVector Direction, bool bLocalConfirmedHit, bool bLocalDamage, float RadiusCm)
{
	UWorld* World=GetWorld();
	if (bClosed||!Profile||!Profile->bEnabled||!World||World->IsPaused()||Position.ContainsNaN()||Direction.ContainsNaN()
		||!World->GetSubsystem<UDeviceLifecycleSubsystem>()->IsGameplayEnabled()) return;
	const auto* Style=Profile->Styles.Find(Kind); if (!Style) return;
	++Counts.FindOrAdd(Kind);
	const double Now=World->GetTimeSeconds();
	if (bLocalConfirmedHit) LastHit=Now;
	if (bLocalDamage) LastDamage=Now;
	Tick(0);
	if (Profile->bWorldVisuals)
	{
		while (Visuals.Num()>=FMath::Clamp(Profile->MaxVisualCues,1,64))
		{
			if (Visuals[0].IsValid()) Visuals[0]->Destroy(); Visuals.RemoveAt(0);
		}
		FActorSpawnParameters Params; Params.ObjectFlags|=RF_Transient;
		Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (auto* Visual=World->SpawnActor<AMothFeedbackVisual>(Position,FRotator::ZeroRotator,Params))
		{
			Visual->InitializeCue(Kind,*Style,Profile,Direction,RadiusCm); Visuals.Add(Visual);
		}
	}
	const double* Previous=LastSound.Find(Kind);
	if (Profile->bAudio&&Profile->MasterVolume>0&&Style->Volume>0&&(!Previous||Now-*Previous>=FMath::Max(0.f,Style->AudioInterval)))
	{
		if (auto* Sound=Style->Sound.LoadSynchronous())
		{
			LastSound.Add(Kind,Now);
			while (Audio.Num()>=FMath::Clamp(Profile->MaxAudioVoices,1,32))
			{
				if (Audio[0].IsValid()) Audio[0]->Stop(); Audio.RemoveAt(0);
			}
			if (auto* Voice=UGameplayStatics::SpawnSoundAtLocation(World,Sound,Position,FRotator::ZeroRotator,
				FMath::Clamp(Profile->MasterVolume*Style->Volume,0.f,1.f),1.f,0.f,Profile->Attenuation.LoadSynchronous())) Audio.Add(Voice);
		}
	}
}

void UMothFeedbackSubsystem::ClearFeedback()
{
	for (const auto& Item : Visuals) if (Item.IsValid()) Item->Destroy();
	for (const auto& Item : Audio) if (Item.IsValid()) Item->Stop();
	Visuals.Reset(); Audio.Reset(); LastSound.Reset(); LastHit=LastDamage=-100;
	bAudioPaused=false;
	for (TActorIterator<ADeviceBase> It(GetWorld()); It; ++It)
		if (It->DefaultFeedback) It->DefaultFeedback->RefreshPresentation();
}
int32 UMothFeedbackSubsystem::GetLiveVisualCount() const { int32 Count=0; for (const auto& Item : Visuals) if (Item.IsValid()&&!Item->IsActorBeingDestroyed()) ++Count; return Count; }
int32 UMothFeedbackSubsystem::GetLiveAudioCount() const { int32 Count=0; for (const auto& Item : Audio) if (Item.IsValid()&&(Item->IsPlaying()||bAudioPaused)) ++Count; return Count; }
float UMothFeedbackSubsystem::GetHitMarkerAlpha() const { return Profile&&Profile->bEnabled?FMath::Clamp(1.f-float(GetWorld()->GetTimeSeconds()-LastHit)/FMath::Max(.02f,Profile->HitMarkerSeconds),0.f,1.f):0; }
float UMothFeedbackSubsystem::GetDamageEdgeAlpha() const { return Profile&&Profile->bEnabled?FMath::Clamp(1.f-float(GetWorld()->GetTimeSeconds()-LastDamage)/FMath::Max(.02f,Profile->DamageEdgeSeconds),0.f,1.f):0; }
