#if WITH_DEV_AUTOMATION_TESTS
#include "Devices/DeviceBase.h"
#include "Misc/AutomationTest.h"
#include "Tests/CombatTestWorld.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothActivationObserverShutdown,"MothEffect.Tutorial.ActivationObserverCannotReopenClosedGameplay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMothActivationObserverShutdown::RunTest(const FString&)
{
	FMothCombatTestWorld Fixture;
	auto* Device=Fixture.Spawn<ADeviceBase>();if (!TestNotNull(TEXT("Native device fixture"),Device)) return false;
	Fixture.AgeDeviceForActivation(Device);
	Device->OnActivatedNative.AddLambda([&](ADeviceBase* D,const FHitContext& H)
	{
		TestEqual(TEXT("Observer sees already committed Active state"),D->GetDeviceState(),EDeviceState::Active);
		TestFalse(TEXT("Observer cannot activate the same device twice"),D->TryActivate(H));
		Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>()->SetGameplayEnabled(false);
	});
	FHitContext Hit;Hit.HitId=FGuid::NewGuid();Hit.ShotDirection=FVector::ForwardVector;
	TestTrue(TEXT("First hit commits once even when observer closes the session"),Device->TryActivate(Hit));
	TestEqual(TEXT("No emitter shots escape an observer's closed session"),Device->GetEmitterShotAttempts(),0);
	TestEqual(TEXT("No projectile allocation escapes closed admission"),Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>()->GetProjectileCount(),0);
	return true;
}
#endif
