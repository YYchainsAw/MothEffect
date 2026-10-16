#include "Feedback/MothFeedbackSubsystem.h"
#include "Feedback/MothFeedbackVisual.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Devices/DeviceBase.h"
#include "Feedback/MothDeviceFeedbackComponent.h"
#include "EngineUtils.h"
#include "AI/EnemyBase.h"
#include "GameFramework/Pawn.h"
#include "Types/HitContext.h"

bool UMothFeedbackSubsystem::DoesSupportWorldType(EWorldType::Type Type) const { return Type==EWorldType::Game||Type==EWorldType::PIE; }
void UMothFeedbackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UDeviceLifecycleSubsystem>();
	if (DefaultProfile.IsNull()) DefaultProfile=TSoftObjectPtr<UMothFeedbackProfile>(FSoftObjectPath(TEXT("/Game/MothEffect/Feedback/DA_DefaultFeedback.DA_DefaultFeedback")));
	Profile=DefaultProfile.LoadSynchronous();
	if (!Profile) Profile=NewObject<UMothFeedbackProfile>(this);
	for (const auto& Entry : Profile->Styles) ResolveNiagara(Entry.Value);
}

UNiagaraSystem* UMothFeedbackSubsystem::ResolveNiagara(const FMothFeedbackStyle& Style)
{
	if (Style.NiagaraSystem.IsNull()) return nullptr;
	const FSoftObjectPath Path=Style.NiagaraSystem.ToSoftObjectPath();
	if (const auto* Cached=NiagaraSystems.Find(Path)) return Cached->Get();
	if (FailedNiagaraSystems.Contains(Path)) return nullptr;
	if (UNiagaraSystem* System=Style.NiagaraSystem.LoadSynchronous())
	{
		NiagaraSystems.Add(Path,System);
		return System;
	}
	FailedNiagaraSystems.Add(Path);
	return nullptr;
}
void UMothFeedbackSubsystem::Deinitialize()
{
	bClosed = true;
	OnEnemyHitConfirmedNative.Clear();
	OnEnemyHitConfirmed.Clear();
	ClearFeedback();
	NiagaraSystems.Reset(); FailedNiagaraSystems.Reset();
	Super::Deinitialize();
}

void UMothFeedbackSubsystem::OnWorldEndPlay(UWorld& World)
{
	bClosed = true;
	OnEnemyHitConfirmedNative.Clear();
	OnEnemyHitConfirmed.Clear();
	ClearFeedback();
	Super::OnWorldEndPlay(World);
}
TStatId UMothFeedbackSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UMothFeedbackSubsystem,STATGROUP_Tickables); }

void UMothFeedbackSubsystem::Tick(float)
{
	Visuals.RemoveAll([](const auto& Item){ return !Item.IsValid()||Item->IsActorBeingDestroyed(); });
	const bool bPaused=GetWorld()->IsPaused();
	if (bPaused!=bAudioPaused)
	{
		bAudioPaused=bPaused;
		for (const auto& Item : Audio) if (Item.IsValid()) Item->SetPaused(bPaused);
		for (const auto& Item : Visuals) if (Item.IsValid()) Item->SetCuePaused(bPaused);
	}
	Audio.RemoveAll([this](const auto& Item){ return !Item.IsValid()||(!bAudioPaused&&!Item->IsPlaying()); });
}

void UMothFeedbackSubsystem::Emit(EMothFeedbackCue Kind, FVector Position, FVector Direction, bool bLocalConfirmedHit, bool bLocalDamage, float RadiusCm)
{
	UWorld* World=GetWorld();
	if (bClosed||bClearing||!Profile||!Profile->bEnabled||!World||World->IsPaused()||Position.ContainsNaN()||Direction.ContainsNaN()
		||!World->GetSubsystem<UDeviceLifecycleSubsystem>()->IsGameplayEnabled()) return;
	const auto* Session=World->GetSubsystem<UDeviceLifecycleSubsystem>();
	const uint64 SessionGeneration=Session->GetLifecycleGeneration(), FeedbackGeneration=PresentationGeneration;
	auto StillCurrent=[&] { return !bClosed&&!bClearing&&PresentationGeneration==FeedbackGeneration
		&&Session->IsGenerationCurrent(SessionGeneration)&&!World->IsPaused()&&Profile->bEnabled; };
	const auto* StoredStyle=Profile->Styles.Find(Kind); if (!StoredStyle) return;
	// Lazy loading a newly edited asset may invoke observers; keep a stable value snapshot.
	const FMothFeedbackStyle StyleSnapshot=*StoredStyle;
	const auto* Style=&StyleSnapshot;
	++Counts.FindOrAdd(Kind);
	const double Now=World->GetTimeSeconds();
	if (bLocalConfirmedHit) LastHit=Now;
	if (bLocalDamage) LastDamage=Now;
	Tick(0);
	UNiagaraSystem* System=Profile->bWorldVisuals?ResolveNiagara(*Style):nullptr;
	if (!StillCurrent()) return;
	if (Profile->bWorldVisuals && (System||Style->bLegacyVisual))
	{
		while (Visuals.Num()>=FMath::Clamp(Profile->MaxVisualCues,1,64))
		{
			const auto Oldest=Visuals[0]; Visuals.RemoveAt(0);
			if (Oldest.IsValid()) Oldest->Destroy();
			if (!StillCurrent()) return;
		}
		FActorSpawnParameters Params; Params.ObjectFlags|=RF_Transient;
		Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (auto* Visual=World->SpawnActor<AMothFeedbackVisual>(Position,FRotator::ZeroRotator,Params))
		{
			// World spawn observers may end/reset the session before this actor is tracked.
			if (!StillCurrent()||!IsValid(Visual)||Visual->IsActorBeingDestroyed())
			{ if (IsValid(Visual)&&!Visual->IsActorBeingDestroyed()) Visual->Destroy(); return; }
			Visual->InitializeCue(Kind,*Style,Profile,Direction,RadiusCm,System);
			if (!StillCurrent()) { if (IsValid(Visual)&&!Visual->IsActorBeingDestroyed()) Visual->Destroy(); return; }
			if (IsValid(Visual)&&!Visual->IsActorBeingDestroyed()) Visuals.Add(Visual);
		}
	}
	if (!StillCurrent()) return;
	const double* Previous=LastSound.Find(Kind);
	if (Profile->bAudio&&Profile->MasterVolume>0&&Style->Volume>0&&(!Previous||Now-*Previous>=FMath::Max(0.f,Style->AudioInterval)))
	{
		if (auto* Sound=Style->Sound.LoadSynchronous())
		{
			LastSound.Add(Kind,Now);
			while (Audio.Num()>=FMath::Clamp(Profile->MaxAudioVoices,1,32))
			{
				const auto Oldest=Audio[0]; Audio.RemoveAt(0);
				if (Oldest.IsValid()) Oldest->Stop();
				if (!StillCurrent()) return;
			}
			if (auto* Voice=UGameplayStatics::SpawnSoundAtLocation(World,Sound,Position,FRotator::ZeroRotator,
				FMath::Clamp(Profile->MasterVolume*Style->Volume,0.f,1.f),1.f,0.f,Profile->Attenuation.LoadSynchronous()))
			{
				if (!StillCurrent()) { Voice->Stop(); return; }
				Audio.Add(Voice);
			}
		}
	}
}

void UMothFeedbackSubsystem::ReportConfirmedEnemyHit(AActor* Victim, const FHitContext& Context,
	float AppliedDamage, float RemainingHealth, bool bKilled, uint64 LifecycleGeneration)
{
	UWorld* World = GetWorld();
	APawn* Instigator = Context.InstigatorPawn.Get();
	if (bClosed || !World || World->IsPaused() || !IsValid(Victim)
		|| !Victim->IsA<AEnemyBase>() || Victim->IsActorBeingDestroyed() || Victim->GetWorld() != World
		|| !IsValid(Instigator) || Instigator == Victim
		|| Instigator->IsActorBeingDestroyed() || Instigator->GetWorld() != World
		|| !Instigator->IsPlayerControlled() || !Instigator->IsLocallyControlled()
		|| !Context.HitId.IsValid() || Context.ImpactPoint.ContainsNaN()
		|| !FMath::IsFinite(AppliedDamage) || AppliedDamage <= 0.0f
		|| !FMath::IsFinite(RemainingHealth) || RemainingHealth < 0.0f)
	{
		return;
	}

	const auto* Session = World->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!Session || !Session->IsGenerationCurrent(LifecycleGeneration)) return;

	FMothEnemyHitFeedback Feedback;
	Feedback.HitId = Context.HitId;
	Feedback.VictimId = static_cast<int32>(Victim->GetUniqueID());
	Feedback.AppliedDamage = AppliedDamage;
	Feedback.RemainingHealth = RemainingHealth;
	Feedback.bKilled = bKilled;
	Feedback.bDeviceSource = Context.bDeviceSource || Cast<ADeviceBase>(Context.SourceActor.Get()) != nullptr;
	Feedback.ImpactPoint = Context.ImpactPoint;
	Feedback.LifecycleGeneration = LifecycleGeneration;
	OnEnemyHitConfirmedNative.Broadcast(Instigator, Feedback);

	// A native observer can finish/reset the run or destroy either actor while reading the result.
	// Do not deliver the old result again to Blueprint after that transition.
	if (bClosed || GetWorld() != World || !IsValid(World) || World->IsPaused()
		|| !IsValid(Session) || !Session->IsGenerationCurrent(LifecycleGeneration)
		|| !IsValid(Victim) || Victim->IsActorBeingDestroyed() || Victim->GetWorld() != World
		|| !IsValid(Instigator) || Instigator == Victim
		|| Instigator->IsActorBeingDestroyed() || Instigator->GetWorld() != World
		|| !Instigator->IsPlayerControlled() || !Instigator->IsLocallyControlled())
	{
		return;
	}
	OnEnemyHitConfirmed.Broadcast(Instigator, Feedback);
}

void UMothFeedbackSubsystem::ClearFeedback()
{
	if (bClearing) return;
	TGuardValue<bool> Guard(bClearing,true); ++PresentationGeneration;
	const auto VisualSnapshot=Visuals; const auto AudioSnapshot=Audio;
	Visuals.Reset(); Audio.Reset(); LastSound.Reset(); LastHit=LastDamage=-100;
	bAudioPaused=false;
	for (const auto& Item : VisualSnapshot) if (Item.IsValid()) Item->Destroy();
	for (const auto& Item : AudioSnapshot) if (Item.IsValid()) Item->Stop();
	for (TActorIterator<ADeviceBase> It(GetWorld()); It; ++It)
		if (It->DefaultFeedback) It->DefaultFeedback->RefreshPresentation();
}
int32 UMothFeedbackSubsystem::GetLiveVisualCount() const { int32 Count=0; for (const auto& Item : Visuals) if (Item.IsValid()&&!Item->IsActorBeingDestroyed()) ++Count; return Count; }
int32 UMothFeedbackSubsystem::GetLiveNiagaraCount() const
{
	int32 Count=0;
	for (const auto& Item : Visuals) if (Item.IsValid()&&!Item->IsActorBeingDestroyed())
		if (const auto* Component=Item->GetNiagaraComponent(); IsValid(Component)&&Component->IsRegistered()) ++Count;
	return Count;
}
int32 UMothFeedbackSubsystem::GetLiveAudioCount() const { int32 Count=0; for (const auto& Item : Audio) if (Item.IsValid()&&(Item->IsPlaying()||bAudioPaused)) ++Count; return Count; }
float UMothFeedbackSubsystem::GetHitMarkerAlpha() const { return Profile&&Profile->bEnabled?FMath::Clamp(1.f-float(GetWorld()->GetTimeSeconds()-LastHit)/FMath::Max(.02f,Profile->HitMarkerSeconds),0.f,1.f):0; }
float UMothFeedbackSubsystem::GetDamageEdgeAlpha() const { return Profile&&Profile->bEnabled?FMath::Clamp(1.f-float(GetWorld()->GetTimeSeconds()-LastDamage)/FMath::Max(.02f,Profile->DamageEdgeSeconds),0.f,1.f):0; }
