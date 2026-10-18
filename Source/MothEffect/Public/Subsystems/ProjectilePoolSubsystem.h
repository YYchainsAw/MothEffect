#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectilePoolSubsystem.generated.h"

class ARuleProjectile;

/** World-local, exact-class projectile reuse. Inactive actors hold no gameplay quota. */
UCLASS(Config=Game, DefaultConfig)
class MOTHEFFECT_API UProjectilePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	ARuleProjectile* SpawnProjectile(TSubclassOf<ARuleProjectile> Class, const FTransform& Transform,
		const FVector& Direction, AActor* Source, APawn* Instigator, bool bLogEvents = false);
	void RecycleProjectile(ARuleProjectile* Projectile);
	int32 GetInactiveCount() const;
	int32 GetCreatedCount() const { return CreatedCount; }
	int32 GetReuseCount() const { return ReuseCount; }
	virtual void OnWorldEndPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
	UPROPERTY(Config, EditDefaultsOnly, Category="Pool", meta=(ClampMin="0"))
	int32 MaxRetainedProjectiles = 128;

private:
	void PruneInactive();
	TMap<TWeakObjectPtr<UClass>, TArray<TWeakObjectPtr<ARuleProjectile>>> InactiveByClass;
	int32 CreatedCount = 0;
	int32 ReuseCount = 0;
	bool bClosed = false;
};
