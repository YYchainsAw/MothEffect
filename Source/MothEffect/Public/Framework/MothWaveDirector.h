#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/DeviceTypes.h"
#include "MothWaveDirector.generated.h"

class AEnemyBase;
class APawn;
class AMothEffectCharacter;
class UHealthComponent;
class UMaterialInterface;
class UStaticMeshComponent;
class UTextRenderComponent;
struct FMothEnemyHitFeedback;

UENUM(BlueprintType)
enum class EMothFlowPhase : uint8 { Ready, Preparing, Fighting, Victory, Defeat, Restarting };

USTRUCT(BlueprintType)
struct FMothWaveSpawn
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Point = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EDeviceKind Kind = EDeviceKind::Launcher;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Batch = 0;
	FMothWaveSpawn() = default;
	FMothWaveSpawn(int32 InPoint, EDeviceKind InKind, int32 InBatch = 0) : Point(InPoint), Kind(InKind), Batch(InBatch) {}
};

USTRUCT(BlueprintType)
struct FMothWaveDefinition
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FMothWaveSpawn> Spawns;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMothFlowChanged, EMothFlowPhase, Phase, int32, Wave, int32, Alive, int32, Pending);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMothWaveSpawnedNative, AEnemyBase*, const FMothWaveSpawn&);

/** Opt-in map actor. Counts queued batches as pending until an actual admitted birth. */
UCLASS(Blueprintable, Config=Game, DefaultConfig)
class MOTHEFFECT_API AMothWaveDirector : public AActor
{
	GENERATED_BODY()
public:
	AMothWaveDirector();
	virtual void Tick(float DeltaSeconds) override;
	UFUNCTION(BlueprintCallable, Category="Moth|Flow") bool SetPlayer(AMothEffectCharacter* Player);
	/** Starts a clean combat run from the main menu. */
	UFUNCTION(BlueprintCallable, Category="Moth|Flow") bool StartEncounter();
	/** Menu controllers disable serialized map auto-start before the first director tick. */
	UFUNCTION(BlueprintCallable, Category="Moth|Flow") void SetAutoStartEnabled(bool bEnabled) { bAutoStart = bEnabled; }
	/** P0 restart reloads the current map, with admission and tasks closed before travel. */
	UFUNCTION(BlueprintCallable, Category="Moth|Flow") bool RestartEncounter();
	/** Tutorial uses Ready but still needs the same death/bounds cleanup before its result page. */
	bool EndPracticeIfPlayerInvalid();
	UFUNCTION(BlueprintPure, Category="Moth|Flow") EMothFlowPhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintPure, Category="Moth|Flow") int32 GetWaveNumber() const { return WaveIndex + 1; }
	UFUNCTION(BlueprintPure, Category="Moth|Flow") int32 GetAliveCount() const;
	UFUNCTION(BlueprintPure, Category="Moth|Flow") int32 GetPendingCount() const;
	UFUNCTION(BlueprintPure, Category="Moth|Flow") float GetPreparationRemaining() const;
	/** World game time, including preparation; freezes on pause and after the result commits. */
	UFUNCTION(BlueprintPure, Category="Moth|Flow") float GetRunElapsedSeconds() const;
	UFUNCTION(BlueprintPure, Category="Moth|Flow") int32 GetKillCount() const { return KillCount; }
	UFUNCTION(BlueprintPure, Category="Moth|Flow") int32 GetDeviceKillCount() const { return DeviceKillCount; }
	/** Consecutive player-attributed device kills with no more than two game seconds between kills. */
	UFUNCTION(BlueprintPure, Category="Moth|Flow") int32 GetBestChain() const { return BestChain; }
	UFUNCTION(BlueprintPure, Category="Moth|Flow") FText GetWaveObjective() const;
	UFUNCTION(BlueprintPure, Category="Moth|Flow") FText GetLastRewardText() const { return LastRewardText; }
	UFUNCTION(BlueprintPure, Category="Moth|Flow") float GetRewardRemainingSeconds() const;
	UFUNCTION(BlueprintPure, Category="Moth|Flow") AMothEffectCharacter* GetPlayer() const { return CombatPlayer.Get(); }
	UFUNCTION(BlueprintPure, Category="Moth|Flow") bool IsEncounterRunning() const { return Phase == EMothFlowPhase::Preparing || Phase == EMothFlowPhase::Fighting; }
	UPROPERTY(BlueprintAssignable, Category="Moth|Flow") FMothFlowChanged OnFlowChanged;
	FMothWaveSpawnedNative OnEnemySpawnedNative;
	const TArray<FMothWaveDefinition>& GetWaves() const { return Waves; }
	const TArray<FVector>& GetSpawnPoints() const { return SpawnPoints; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	/** Fixed wave compositions from the level specification; edit positions/classes, not counts. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moth|Flow") TArray<FMothWaveDefinition> Waves;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moth|Flow") TArray<FVector> SpawnPoints;
	/** D01 / D02 / D03. Saved BP provides the T15 variants, keeping references cookable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moth|Flow") TArray<TSubclassOf<AEnemyBase>> EnemyClasses;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moth|Flow") bool bAutoStart = false;
	UPROPERTY(Config, EditDefaultsOnly, Category="Moth|Flow") int32 MaxAliveEnemies = 8;
	UPROPERTY(Config, EditDefaultsOnly, Category="Moth|Flow") float PreparationSeconds = 5.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Moth|Flow") float BatchSpawnDelaySeconds = 3.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Moth|Flow") float SpawnMinDistanceToPlayerCm = 600.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Moth|Flow") float SpawnWarningSeconds = 0.8f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Moth|Flow") float SpawnRetrySeconds = 0.5f;
	UPROPERTY(EditAnywhere, Category="Moth|Bounds") FVector SafetyCenter = FVector(0, 0, 500);
	UPROPERTY(EditAnywhere, Category="Moth|Bounds") FVector SafetyHalfExtent = FVector(3000, 2500, 1500);
	UPROPERTY(EditDefaultsOnly, Category="Moth|Presentation") TObjectPtr<UMaterialInterface> SpawnWarningMaterial;
	UPROPERTY(VisibleAnywhere, Category="Moth|Presentation") TArray<TObjectPtr<UStaticMeshComponent>> WarningCircles;
	UPROPERTY(VisibleAnywhere, Category="Moth|Presentation") TArray<TObjectPtr<UTextRenderComponent>> WarningLabels;

private:
	friend struct FMothEncounterPresentationFixture;
	struct FPendingBirth
	{
		FMothWaveSpawn Spec;
		double Due = 0;
		double RetryAt = 0;
		double WarningAt = -1;
		FVector WarnedPosition = FVector::ZeroVector;
		bool bComplete = false;
	};
	TArray<FPendingBirth> PendingBirths;
	TSet<TWeakObjectPtr<AEnemyBase>> AliveEnemies;
	TWeakObjectPtr<AMothEffectCharacter> CombatPlayer;
	TWeakObjectPtr<UHealthComponent> PlayerHealth;
	EMothFlowPhase Phase = EMothFlowPhase::Ready;
	int32 WaveIndex = -1;
	double PreparationEndsAt = 0;
	uint64 RunGeneration = 0;
	bool bChangingFlow = false;
	bool bEndingPlay = false;
	double RunStartedAt = -1;
	float FinishedElapsedSeconds = 0;
	int32 KillCount = 0;
	int32 DeviceKillCount = 0;
	int32 CurrentChain = 0;
	int32 BestChain = 0;
	double LastDeviceKillAt = -1;
	double RewardEndsAt = 0;
	FText LastRewardText;
	TSet<int32> CountedVictimIds;
	void PrepareWave(int32 Index);
	void ProcessBirths(double Now);
	bool FindBirthplace(const FMothWaveSpawn& Spec, FVector& Position) const;
	void RegisterEnemy(AEnemyBase* Enemy);
	void UnregisterEnemy(AEnemyBase* Enemy);
	void HandleConfirmedEnemyHit(APawn* HitInstigator, const FMothEnemyHitFeedback& Feedback);
	void HandleEnemyDied(AEnemyBase* Enemy, AActor* Source);
	UFUNCTION() void HandleEnemyEndPlay(AActor* Enemy, EEndPlayReason::Type Reason);
	UFUNCTION() void HandlePlayerDied(AActor* Player, AActor* Source);
	UFUNCTION() void HandlePlayerEndPlay(AActor* Player, EEndPlayReason::Type Reason);
	void FinishEncounter(EMothFlowPhase Result);
	void CloseAndClean(bool bResetWave);
	void UnbindPlayer();
	void PublishState();
	void ShowWarning(int32 Point, const FVector& Position, bool bBlocked, EDeviceKind Kind);
	void HideWarnings();
	bool IsCurrent(uint64 Generation) const { return !bEndingPlay && RunGeneration == Generation && IsEncounterRunning(); }
	bool IsOutside(const FVector& Position) const;
};
