#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/BallisticReactive.h"
#include "Interfaces/ImpulseReceiver.h"
#include "EnemyBase.generated.h"

class ARuleProjectile;
class UHealthComponent;
class UExternalImpulseComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UArrowComponent;
class AEnemyBase;

UENUM(BlueprintType)
enum class EEnemyCombatPhase : uint8 { Idle, Pursuing, Telegraphing, Cooldown, Disabled, Dead };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothEnemyDied, AEnemyBase*, Enemy, AActor*, Source);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMothEnemyTelegraph, AEnemyBase*, Enemy, bool, bVisible);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMothEnemyDiedNative, AEnemyBase*, AActor*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMothEnemyTelegraphNative, AEnemyBase*, bool);

/** One shooting archetype. T15 binds deterministic drops; T16 owns spawning/wave counts. */
UCLASS(Blueprintable, Config=Game, DefaultConfig)
class MOTHEFFECT_API AEnemyBase : public ACharacter, public IBallisticReactive, public IImpulseReceiver
{
	GENERATED_BODY()
public:
	AEnemyBase();
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostInitializeComponents() override;
	virtual bool ReceiveBallisticHit_Implementation(const FHitContext& Context) override;
	virtual void ReceiveImpulse_Implementation(FVector Velocity, bool bOverrideZ, FGuid EventId) override;
	UFUNCTION(BlueprintCallable, Category="Enemy")
	bool SetCombatTarget(APawn* Target);
	UFUNCTION(BlueprintCallable, Category="Enemy")
	void SetGameplayEnabled(bool bEnabled);
	UFUNCTION(BlueprintPure, Category="Enemy")
	bool IsCombatEnabled() const;
	UFUNCTION(BlueprintPure, Category="Enemy")
	EEnemyCombatPhase GetCombatPhase() const { return CombatPhase; }
	UFUNCTION(BlueprintPure, Category="Enemy")
	APawn* GetCombatTarget() const { return CombatTarget.Get(); }
	UFUNCTION(BlueprintPure, Category="Enemy")
	bool HasTargetSight() const;
	UFUNCTION(BlueprintPure, Category="Enemy")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	int32 GetShotAttempts() const { return ShotAttempts; }
	int32 GetProjectilesSpawned() const { return ProjectilesSpawned; }
	int32 GetNavigationRequests() const { return NavigationRequests; }
	int32 GetStallRecoveries() const { return StallRecoveries; }
	FMothEnemyDiedNative OnEnemyDiedNative;
	FMothEnemyTelegraphNative OnTelegraphChangedNative;
	UPROPERTY(BlueprintAssignable, Category="Enemy")
	FMothEnemyDied OnEnemyDied;
	UPROPERTY(BlueprintAssignable, Category="Enemy")
	FMothEnemyTelegraph OnTelegraphChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UExternalImpulseComponent> ExternalImpulseComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> TelegraphText;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UArrowComponent> AimArrow;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack")
	TSubclassOf<ARuleProjectile> ProjectileClass;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy", meta=(ClampMin="1.0"))
	float MaxHealth = 60.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Attack", meta=(ClampMin="0.01"))
	float ShotIntervalSeconds = 1.2f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Attack", meta=(ClampMin="0.01"))
	float TelegraphSeconds = 0.5f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Movement", meta=(ClampMin="1.0"))
	float WalkSpeedCmPerSec = 300.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Sight", meta=(ClampMin="1.0"))
	float SightRangeCm = 2500.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Attack", meta=(ClampMin="1.0"))
	float AttackRangeCm = 1800.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Sight", meta=(ClampMin="0.0"))
	float SightMemorySeconds = 3.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Movement", meta=(ClampMin="0.1"))
	float RepathIntervalSeconds = 0.5f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Movement", meta=(ClampMin="0.1"))
	float StallTimeoutSeconds = 1.5f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Movement", meta=(ClampMin="1.0"))
	float StallProgressCm = 10.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Movement", meta=(ClampMin="1.0"))
	float RecoveryOffsetCm = 500.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Attack", meta=(ClampMin="0.0"))
	float MuzzleClearanceCm = 2.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy", meta=(ClampMin="0.01"))
	float DeathVisualSeconds = 0.5f;
	UPROPERTY(EditAnywhere, Category="Enemy|Debug")
	bool bLogEnemyEvents = false;
	UPROPERTY(EditAnywhere, Category="Enemy")
	bool bAutoAcquirePlayer = true;

private:
	UFUNCTION()
	void HandleEnemyDied(AActor* Victim, AActor* Source);
	bool IsUsableTarget(const APawn* Target) const;
	void SetTelegraph(bool bVisible);
	void StopNavigation();
	void Pursue(double Now);
	void FireProjectile();
	FVector AimPoint() const;
	void LogEvent(const TCHAR* Event) const;
	TWeakObjectPtr<APawn> CombatTarget;
	EEnemyCombatPhase CombatPhase = EEnemyCombatPhase::Idle;
	FVector LastSeenLocation = FVector::ZeroVector;
	FVector ProgressLocation = FVector::ZeroVector;
	double LastSeenAt = -1.0e9;
	double NextFireAt = 0.0;
	double TelegraphEndsAt = 0.0;
	double NextRepathAt = 0.0;
	double LastProgressAt = 0.0;
	uint64 CombatGeneration = 0;
	int32 ShotAttempts = 0;
	int32 ProjectilesSpawned = 0;
	int32 NavigationRequests = 0;
	int32 StallRecoveries = 0;
	int32 RecoveryIndex = 0;
	bool bTelegraphVisible = false;
	bool bGameplayEnabled = true;
	bool bDeathProcessed = false;
	bool bEndingPlay = false;
};
