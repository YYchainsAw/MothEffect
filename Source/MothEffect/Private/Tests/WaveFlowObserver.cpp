#include "Tests/WaveFlowObserver.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Framework/MothWaveDirector.h"
#include "Components/HealthComponent.h"
#include "Devices/DeviceBase.h"
#include "Engine/World.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#endif

void UMothWaveFlowObserver::HealthChanged(float Health, float MaxHealth)
{
#if WITH_DEV_AUTOMATION_TESTS
	if (Health > 0 || !Director.IsValid()) return;
	bNotified = true;
	bTerminal = Director->GetPhase() == EMothFlowPhase::Defeat;
	FHitContext Hit; Hit.HitId = FGuid::NewGuid(); Hit.Damage = 10;
	bDamaged = OtherHealth.IsValid() && OtherHealth->ApplyHit(Hit);
	bSpawned = Director->GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()->SpawnDevice(ADeviceBase::StaticClass(), FTransform(FVector(0,0,200))) != nullptr;
#endif
}
