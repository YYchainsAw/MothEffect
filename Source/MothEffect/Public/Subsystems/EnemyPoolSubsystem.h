#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyPoolSubsystem.generated.h"

class AEnemyBase;

/** Reuses exact enemy classes inside one world. Checkout is inert until the director registers it. */
UCLASS(Config=Game, DefaultConfig)
class MOTHEFFECT_API UEnemyPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	AEnemyBase* AcquireEnemy(TSubclassOf<AEnemyBase> EnemyClass, const FTransform& Transform, AActor* Owner);
	void FinishSpawningEnemy(AEnemyBase* Enemy, const FTransform& Transform);
	void RecycleEnemy(AEnemyBase* Enemy);
	int32 GetInactiveCount(TSubclassOf<AEnemyBase> EnemyClass = nullptr) const;
	int32 GetCreatedCount() const { return CreatedCount; }
	int32 GetReuseCount() const { return ReuseCount; }
	virtual void OnWorldEndPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;

private:
	friend class AEnemyBase;
	bool IsBirthplaceClear(const AEnemyBase* Enemy) const;
	/** Global idle retention; active enemy admission remains the wave director's responsibility. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Pool", meta=(ClampMin="0"))
	int32 MaxRetainedEnemies = 32;
	UPROPERTY(Transient)
	TArray<TObjectPtr<AEnemyBase>> ManagedEnemies;
	TArray<TWeakObjectPtr<AEnemyBase>> InactiveEnemies;
	TSet<TWeakObjectPtr<AEnemyBase>> PendingCheckout;
	TSet<TWeakObjectPtr<AEnemyBase>> RecyclingEnemies;
	int32 CreatedCount = 0;
	int32 ReuseCount = 0;
	bool bShuttingDown = false;
};
