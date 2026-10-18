#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/BallisticReactive.h"
#include "Interfaces/ImpulseReceiver.h"
#include "Types/DeviceTypes.h"
#include "EnemyBase.generated.h"

class ARuleProjectile;
class UHealthComponent;
class UExternalImpulseComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UArrowComponent;
class AEnemyBase;
class ADeviceBase;
class UWidgetComponent;
class UMothEnemyHealthWidget;
class UMothCharacterVoiceComponent;
class UEnemyPoolSubsystem;
class UAnimInstance;
class AAIController;

UENUM(BlueprintType)
enum class EEnemyCombatPhase : uint8 { Idle, Pursuing, Telegraphing, Cooldown, Disabled, Dead };

UENUM(BlueprintType)
enum class EEnemyDropResult : uint8 { Unconfigured, Ready, Spawned, InvalidClass, NoSafeLocation, Rejected, Cancelled };

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
	virtual void OnConstruction(const FTransform& Transform) override;
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
	UFUNCTION(BlueprintPure, Category="Enemy|Pool")
	bool IsPoolInactive() const { return bPoolInactive; }
	UFUNCTION(BlueprintCallable, Category="Enemy|Pool")
	void ReturnToPool();
	UFUNCTION(BlueprintPure, Category="Enemy|Drop")
	EDeviceKind GetDropKind() const { return DropKind; }
	UFUNCTION(BlueprintPure, Category="Enemy|Drop")
	EEnemyDropResult GetDropResult() const { return DropResult; }
	UFUNCTION(BlueprintPure, Category="Enemy|Drop")
	ADeviceBase* GetDroppedDevice() const { return DroppedDevice.Get(); }
	int32 GetDropAttempts() const { return bDropAttempted ? 1 : 0; }
	FMothEnemyDiedNative OnEnemyDiedNative;
	FMothEnemyTelegraphNative OnTelegraphChangedNative;
	UPROPERTY(BlueprintAssignable, Category="Enemy")
	FMothEnemyDied OnEnemyDied;
	UPROPERTY(BlueprintAssignable, Category="Enemy")
	FMothEnemyTelegraph OnTelegraphChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void LifeSpanExpired() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UMothCharacterVoiceComponent> CharacterVoice;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UExternalImpulseComponent> ExternalImpulseComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> TelegraphText;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UArrowComponent> AimArrow;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BackpackMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> DropHintText;
	/** Screen-space content is authored in WBP_EnemyHealth, never built natively. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UWidgetComponent> EnemyHealthWidget;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|UI")
	TSubclassOf<UMothEnemyHealthWidget> HealthWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|UI", meta=(ClampMin="0.0", Units="cm"))
	float HealthWidgetMaxDistanceCm = 2500.0f;
	/** Null keeps the T14 base shooter drop-free. Three child BPs provide fixed classes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Drop")
	TSubclassOf<ADeviceBase> DropDeviceClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Drop")
	EDeviceKind DropKind = EDeviceKind::Emitter;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Drop", meta=(ClampMin="0.0", Units="cm"))
	float DropSearchRadiusCm = 150.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Drop", meta=(ClampMin="1.0", Units="cm"))
	float DropGroundProbeDistanceCm = 1500.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Drop", meta=(ClampMin="0.01", Units="cm"))
	float DropClearanceCm = 2.0f;
	UPROPERTY(Config, EditDefaultsOnly, Category="Enemy|Drop", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DropGroundUpNormalMin = 0.65f;
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
	friend class UEnemyPoolSubsystem;
	void PrepareForPoolAcquisition();
	void ActivateFromPool();
	void DeactivateForPool();
	void SuspendPoolTicks();
	void RestorePoolTicks();
	UFUNCTION()
	void HandleEnemyDied(AActor* Victim, AActor* Source);
	bool IsUsableTarget(const APawn* Target) const;
	void SetTelegraph(bool bVisible);
	void StopNavigation();
	void Pursue(double Now);
	void FireProjectile();
	FVector AimPoint() const;
	void LogEvent(const TCHAR* Event) const;
	void RefreshDropAppearance();
	void InitializeEnemyHealthWidget();
	void UpdateEnemyHealthWidgetVisibility();
	void ReleaseEnemyHealthWidget();
	void TryDropDevice(TSubclassOf<ADeviceBase> Class, EDeviceKind Kind, const FVector& DeathLocation, uint64 Generation, uint64 LifeGeneration);
	bool FindDropTransform(const ADeviceBase* Defaults, const FVector& DeathLocation, FTransform& OutTransform) const;
	TWeakObjectPtr<ADeviceBase> DroppedDevice;
	EEnemyDropResult DropResult = EEnemyDropResult::Unconfigured;
	bool bDropAttempted = false;
	TWeakObjectPtr<APawn> CombatTarget;
	EEnemyCombatPhase CombatPhase = EEnemyCombatPhase::Idle;
	FVector LastSeenLocation = FVector::ZeroVector;
	FVector ProgressLocation = FVector::ZeroVector;
	FVector RecoveryGoal = FVector::ZeroVector;
	double LastSeenAt = -1.0e9;
	double NextFireAt = 0.0;
	double TelegraphEndsAt = 0.0;
	double NextRepathAt = 0.0;
	double LastProgressAt = 0.0;
	double RecoveryExpiresAt = 0.0;
	uint64 CombatGeneration = 0;
	uint64 PoolLifeGeneration = 0;
	uint64 PoolCheckoutLifecycleGeneration = 0;
	int32 PoolCallbackDepth = 0;
	UPROPERTY(Transient)
	TObjectPtr<UClass> PoolAnimationClass;
	FTransform PoolMeshRelativeTransform;
	TMap<TWeakObjectPtr<UActorComponent>, bool> PoolComponentTicks;
	TWeakObjectPtr<AAIController> PoolController;
	bool bPoolActorTickEnabled = true;
	bool bPoolControllerTickEnabled = true;
	bool bPoolTicksSuspended = false;
	bool bPoolManaged = false;
	bool bPoolInactive = false;
	bool bPoolMeshHidden = false;
	bool bPoolAnimationPaused = false;
	bool bPoolAnimationInitialized = false;
	uint8 PoolAnimationMode = 0;
	int32 ShotAttempts = 0;
	int32 ProjectilesSpawned = 0;
	int32 NavigationRequests = 0;
	int32 StallRecoveries = 0;
	int32 RecoveryIndex = 0;
	bool bTelegraphVisible = false;
	bool bRecovering = false;
	bool bGameplayEnabled = true;
	bool bDeathProcessed = false;
	bool bEndingPlay = false;
	double NextHealthWidgetVisibilityAt = 0.0;
};
