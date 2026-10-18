#include "Feedback/MothCharacterVoiceComponent.h"
#include "AI/EnemyBase.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/HealthComponent.h"
#include "Engine/World.h"
#include "Feedback/MothAudioSubsystem.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Sound/SoundBase.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include <initializer_list>

namespace
{
	void SetWaves(TArray<TSoftObjectPtr<USoundBase>>& Out, const TCHAR* Directory,
		std::initializer_list<const TCHAR*> Names)
	{
		Out.Reset();
		for (const TCHAR* Name : Names)
			Out.Emplace(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Directory, Name, Name)));
	}
	constexpr const TCHAR* MurdockWaves = TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Sounds/SoundWaves");
	constexpr const TCHAR* DrongoWaves = TEXT("/Game/ParagonDrongo/Audio/Wavs");
}

UMothCharacterVoiceComponent::UMothCharacterVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Verified individual dialogue text: Target engaged! / Shots fired!
	SetWaves(EngageSounds, MurdockWaves, {TEXT("Murdock_Ability_LMB_Engage_020"), TEXT("Murdock_Ability_LMB_Engage_030")});
	// Tango down. / Job well done. / And stay down!
	SetWaves(KillSounds, MurdockWaves, {TEXT("Murdock_Kill_Enemy_010"), TEXT("Murdock_Kill_Enemy_020"), TEXT("Murdock_Kill_Enemy_050")});
	SetWaves(PainSounds, MurdockWaves, {TEXT("Murdock_Effort_Pain_01"), TEXT("Murdock_Effort_Pain_02"),
		TEXT("Murdock_Effort_Pain_03"), TEXT("Murdock_Effort_Pain_04"), TEXT("Murdock_Effort_Pain_05"), TEXT("Murdock_Effort_Pain_06")});
	SetWaves(DeathSounds, MurdockWaves, {TEXT("Murdock_Effort_Death_01"), TEXT("Murdock_Effort_Death_02"),
		TEXT("Murdock_Effort_Death_03"), TEXT("Murdock_Effort_Death_04")});
}

void UMothCharacterVoiceComponent::ConfigureDrongoDefaults()
{
	bEnemyVoice = true;
	// Only this engage variant is suitable: I shoot, you die! Other variants mention gold/pistols.
	SetWaves(EngageSounds, DrongoWaves, {TEXT("Drongo_Ability_LMB_Engage_010")});
	KillSounds.Reset();
	SetWaves(PainSounds, DrongoWaves, {TEXT("Drongo_Effort_Pain_010"), TEXT("Drongo_Effort_Pain_011"),
		TEXT("Drongo_Effort_Pain_012"), TEXT("Drongo_Effort_Pain_013"), TEXT("Drongo_Effort_Pain_014")});
	SetWaves(DeathSounds, DrongoWaves, {TEXT("Drongo_Effort_Death_010"), TEXT("Drongo_Effort_Death_011"),
		TEXT("Drongo_Effort_Death_012"), TEXT("Drongo_Effort_Death_013"), TEXT("Drongo_Effort_Death_014")});
}

void UMothCharacterVoiceComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = GetOwner()->FindComponentByClass<UHealthComponent>();
	if (!Health) return;
	PreviousHealth = Health->GetCurrentHealth();
	Health->OnHealthChanged.AddUniqueDynamic(this, &UMothCharacterVoiceComponent::HandleHealthChanged);
	DeathCommitHandle = Health->OnDeathCommittedNative.AddUObject(this, &UMothCharacterVoiceComponent::HandleDeathCommitted);
	if (bEnemyVoice)
	{
		if (auto* Enemy = Cast<AEnemyBase>(GetOwner()))
			TelegraphHandle = Enemy->OnTelegraphChangedNative.AddUObject(this, &UMothCharacterVoiceComponent::HandleEnemyTelegraph);
	}
	else if (auto* Feedback = GetWorld()->GetSubsystem<UMothFeedbackSubsystem>())
		EnemyHitHandle = Feedback->OnEnemyHitConfirmedNative.AddUObject(this, &UMothCharacterVoiceComponent::HandleEnemyHit);
	// Preload only this small, explicit whitelist before combat; no synchronous first-hit hitch.
	for (const auto* Variants : {&EngageSounds, &KillSounds, &PainSounds, &DeathSounds})
		for (const auto& Variant : *Variants)
			if (!Variant.IsNull())
				if (USoundBase* Sound = Variant.LoadSynchronous()) LoadedSounds.Add(Variant.ToSoftObjectPath(), Sound);
}

void UMothCharacterVoiceComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	if (Health)
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UMothCharacterVoiceComponent::HandleHealthChanged);
		Health->OnDeathCommittedNative.Remove(DeathCommitHandle);
	}
	if (auto* Enemy = Cast<AEnemyBase>(GetOwner())) Enemy->OnTelegraphChangedNative.Remove(TelegraphHandle);
	if (UWorld* World = GetWorld())
	{
		if (auto* Feedback = World->GetSubsystem<UMothFeedbackSubsystem>()) Feedback->OnEnemyHitConfirmedNative.Remove(EnemyHitHandle);
		// World-owned death audio outlives the short enemy corpse lifespan.
		if (auto* Audio = World->GetSubsystem<UMothAudioSubsystem>()) Audio->StopSpeaker(GetOwner());
	}
	LoadedSounds.Reset();
	Super::EndPlay(Reason);
}

bool UMothCharacterVoiceComponent::CanSpeak(bool bDeathVoice) const
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (bEndingPlay || !bEnabled || !FMath::IsFinite(Volume) || Volume <= 0 || !IsValid(Owner)
		|| Owner->IsActorBeingDestroyed() || !World || World->IsPaused() || World->GetNetMode() == NM_DedicatedServer
		|| !IsValid(Health)) return false;
	const auto* Session = World->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!Session) return false;
	if (!bEnemyVoice)
	{
		const APawn* Pawn = Cast<APawn>(Owner);
		if (!Pawn || !Pawn->IsPlayerControlled() || !Pawn->IsLocallyControlled()) return false;
	}
	if (bDeathVoice)
	{
		if (!bDeathCommitted || bDeathRequested || Health->IsAlive()) return false;
		const uint64 CurrentGeneration = Session->GetLifecycleGeneration();
		// Death may finish the run before its health callback. A reset must never revive old audio.
		return CurrentGeneration == DeathCommitGeneration || (bDeathCommitSessionEnabled
			&& !Session->IsGameplayEnabled() && CurrentGeneration == DeathCommitGeneration + 1);
	}
	if (!Health->IsAlive() || !Session->IsGameplayEnabled()) return false;
	if (const auto* Player = Cast<AMothEffectCharacter>(Owner)) return Player->IsGameplayEnabled();
	if (const auto* Enemy = Cast<AEnemyBase>(Owner)) return Enemy->IsCombatEnabled();
	return false;
}

void UMothCharacterVoiceComponent::RequestCue(EMothCharacterVoiceCue Cue)
{
	const bool bDeath = Cue == EMothCharacterVoiceCue::Death;
	if (!CanSpeak(bDeath)) return;
	const double Now = GetWorld()->GetTimeSeconds();
	const bool bBark = Cue == EMothCharacterVoiceCue::Engage || Cue == EMothCharacterVoiceCue::Kill;
	if (Cue == EMothCharacterVoiceCue::Pain && Now - LastPainAt < FMath::Max(0.0f, PainIntervalSeconds)) return;
	if (bBark && Now - LastBarkAt < FMath::Max(0.0f, BarkIntervalSeconds)) return;
	const auto& Variants = Cue == EMothCharacterVoiceCue::Engage ? EngageSounds : Cue == EMothCharacterVoiceCue::Kill ? KillSounds
		: Cue == EMothCharacterVoiceCue::Pain ? PainSounds : DeathSounds;
	if (Variants.IsEmpty()) return;
	int32 Index = FMath::RandRange(0, Variants.Num() - 1);
	if (Variants.Num() > 1 && LastVariant.Contains(Cue) && Index == LastVariant.FindRef(Cue)) Index = (Index + 1) % Variants.Num();
	USoundBase* Sound = LoadedSounds.FindRef(Variants[Index].ToSoftObjectPath());
	if (!Sound) return;
	auto* Audio = GetWorld()->GetSubsystem<UMothAudioSubsystem>();
	if (!Audio) return;
	// Reserve the local cooldown before playback: audio callbacks cannot recursively repeat a cue.
	LastVariant.Add(Cue, Index);
	if (Cue == EMothCharacterVoiceCue::Pain) LastPainAt = Now;
	if (bBark) LastBarkAt = Now;
	if (bDeath) bDeathRequested = true;
	if (Audio->PlayCharacterVoice(GetOwner(), Sound, bEnemyVoice, FMath::Clamp(Volume, 0.0f, 1.0f), bDeath, bEnemyVoice && bBark))
		++CueCounts.FindOrAdd(Cue);
}

void UMothCharacterVoiceComponent::NotifyShotFired()
{
	if (!bEnemyVoice) RequestCue(EMothCharacterVoiceCue::Engage);
}

void UMothCharacterVoiceComponent::ResetForPoolReuse()
{
	if (auto* Audio = GetWorld() ? GetWorld()->GetSubsystem<UMothAudioSubsystem>() : nullptr)
		Audio->StopSpeaker(GetOwner());
	LastPainAt = LastBarkAt = -1.e9;
	PreviousHealth = Health ? Health->GetCurrentHealth() : 0.0f;
	DeathCommitGeneration = 0;
	bDeathCommitted = bDeathCommitSessionEnabled = bDeathRequested = false;
	CueCounts.Reset();
	LastVariant.Reset();
}

void UMothCharacterVoiceComponent::HandleEnemyTelegraph(AEnemyBase* Enemy, bool bVisible)
{
	if (bEnemyVoice && Enemy == GetOwner() && bVisible) RequestCue(EMothCharacterVoiceCue::Engage);
}

void UMothCharacterVoiceComponent::HandleEnemyHit(APawn* Instigator, const FMothEnemyHitFeedback& Feedback)
{
	if (bEndingPlay || !GetWorld()) return;
	if (!bEnemyVoice && Instigator == GetOwner() && Feedback.bKilled)
		if (const auto* Session = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>(); Session && Session->IsGenerationCurrent(Feedback.LifecycleGeneration))
			RequestCue(EMothCharacterVoiceCue::Kill);
}

void UMothCharacterVoiceComponent::HandleDeathCommitted(AActor* Victim, AActor*)
{
	if (bEndingPlay || !GetWorld() || Victim != GetOwner() || bDeathCommitted) return;
	if (const auto* Enemy = Cast<AEnemyBase>(GetOwner()); Enemy && Enemy->IsPoolInactive()) return;
	bDeathCommitted = true;
	if (const auto* Session = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>())
	{
		// Multicast order is unspecified: a prior observer may already have reset the
		// session. Health snapshots the accepted hit's generation before broadcasting.
		DeathCommitGeneration = Health ? Health->GetLastHitLifecycleGeneration() : Session->GetLifecycleGeneration();
		bDeathCommitSessionEnabled = true; // ApplyHit only accepts damage while gameplay is enabled.
	}
}

void UMothCharacterVoiceComponent::HandleHealthChanged(float CurrentHealth, float)
{
	if (!Health || !FMath::IsNearlyEqual(CurrentHealth, Health->GetCurrentHealth())) return;
	const bool bLostHealth = CurrentHealth < PreviousHealth;
	PreviousHealth = CurrentHealth;
	if (!bLostHealth) return;
	if (CurrentHealth > 0)
	{
		const auto* Session = GetWorld() ? GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>() : nullptr;
		if (!Session || !Session->IsGenerationCurrent(Health->GetLastHitLifecycleGeneration())) return;
	}
	RequestCue(CurrentHealth <= 0 ? EMothCharacterVoiceCue::Death : EMothCharacterVoiceCue::Pain);
}
