#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/EnemyHitFeedback.h"
#include "MothCharacterVoiceComponent.generated.h"

class AEnemyBase;
class APawn;
class UHealthComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EMothCharacterVoiceCue : uint8 { Engage, Kill, Pain, Death };

/** Committed combat events select a short line; world audio owns playback and teardown. */
UCLASS(Config=Game, DefaultConfig, ClassGroup=(MothEffect), meta=(BlueprintSpawnableComponent))
class MOTHEFFECT_API UMothCharacterVoiceComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMothCharacterVoiceComponent();
	/** Constructor-only archetype defaults. Blueprint component overrides remain editable. */
	void ConfigureDrongoDefaults();
	/** Called by the player's actual rifle-shot callback, never by fire input. */
	void NotifyShotFired();
	/** Keep loaded assets and BeginPlay bindings; clear state belonging to the old life. */
	void ResetForPoolReuse();
	uint64 GetCueCount(EMothCharacterVoiceCue Cue) const { return CueCounts.FindRef(Cue); }
	UFUNCTION(BlueprintPure, Category="Voice")
	int32 GetCueRequestCount(EMothCharacterVoiceCue Cue) const { return static_cast<int32>(GetCueCount(Cue)); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Voice") bool bEnabled = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voice") bool bEnemyVoice = false;
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category="Voice", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Volume = 0.8f;
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category="Voice", meta=(ClampMin="0.0", Units="s"))
	float PainIntervalSeconds = 0.8f;
	/** Engage and kill lines share a cooldown so automatic fire cannot become narration. */
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category="Voice", meta=(ClampMin="0.0", Units="s"))
	float BarkIntervalSeconds = 10.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voice|Sounds")
	TArray<TSoftObjectPtr<USoundBase>> EngageSounds;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voice|Sounds")
	TArray<TSoftObjectPtr<USoundBase>> KillSounds;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voice|Sounds")
	TArray<TSoftObjectPtr<USoundBase>> PainSounds;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voice|Sounds")
	TArray<TSoftObjectPtr<USoundBase>> DeathSounds;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UPROPERTY(Transient) TObjectPtr<UHealthComponent> Health;
	/** Only the selected archetype's small whitelist is loaded, never a dialogue randomizer. */
	UPROPERTY(Transient) TMap<FSoftObjectPath,TObjectPtr<USoundBase>> LoadedSounds;
	TMap<EMothCharacterVoiceCue,uint64> CueCounts;
	TMap<EMothCharacterVoiceCue,int32> LastVariant;
	FDelegateHandle EnemyHitHandle, TelegraphHandle, DeathCommitHandle;
	double LastPainAt = -1.e9, LastBarkAt = -1.e9;
	float PreviousHealth = 0;
	uint64 DeathCommitGeneration = 0;
	bool bDeathCommitted = false, bDeathCommitSessionEnabled = false;
	bool bDeathRequested = false, bEndingPlay = false;
	bool CanSpeak(bool bDeathVoice = false) const;
	void RequestCue(EMothCharacterVoiceCue Cue);
	void HandleDeathCommitted(AActor* Victim, AActor* Source);
	void HandleEnemyTelegraph(AEnemyBase* Enemy, bool bVisible);
	void HandleEnemyHit(APawn* Instigator, const FMothEnemyHitFeedback& Feedback);
	UFUNCTION() void HandleHealthChanged(float CurrentHealth, float MaxHealth);
};
