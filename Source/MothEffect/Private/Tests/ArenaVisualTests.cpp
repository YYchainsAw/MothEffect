#if WITH_DEV_AUTOMATION_TESTS

#include "AI/EnemyBase.h"
#include "Art/MothArenaVisualDirector.h"
#include "Combat/RuleProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/HealthComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Subsystems/EnemyPoolSubsystem.h"
#include "Subsystems/ProjectilePoolSubsystem.h"
#include "Tests/CombatTestWorld.h"

namespace MothArenaVisualTest
{
	ADirectionalLight* Directional(FMothCombatTestWorld& Fixture, float Intensity, bool bShadows)
	{
		auto* Light = Fixture.Spawn<ADirectionalLight>();
		if (!Light) return nullptr;
		auto* Component = Light->GetLightComponent();
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetIntensity(Intensity);
		Component->SetCastShadows(bShadows);
		Component->SetLightColor(FLinearColor(.8f, .6f, .4f));
		Light->SetActorRotation(FRotator(-30, 45, 0));
		return Light;
	}

	ASkyLight* Sky(FMothCombatTestWorld& Fixture, float Intensity)
	{
		auto* Light = Fixture.Spawn<ASkyLight>();
		if (!Light) return nullptr;
		Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Light->GetLightComponent()->SetIntensity(Intensity);
		return Light;
	}

	void OptIntoFixture(AMothArenaVisualDirector* Director)
	{
		// Exercise the same map gate without weakening the production SoulCity default.
		Director->bEnabled = true;
		Director->TargetMapName = FName(*UGameplayStatics::GetCurrentLevelName(Director, true));
		Director->ApplyLook();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArenaVisualLights,
	"MothEffect.ArenaVisual.ScopedLightingIsIdempotentAndRestored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothArenaVisualLights::RunTest(const FString&)
{
	using namespace MothArenaVisualTest;
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Isolated lighting world"), Fixture.World)) return false;
	auto* Sun = Directional(Fixture, 11.f, true);
	auto* OtherSun = Directional(Fixture, 3.f, true);
	auto* Fill = Directional(Fixture, .75f, false);
	auto* OtherFill = Directional(Fixture, 1.25f, false);
	auto* SkyLight = Sky(Fixture, 5.f);
	auto* OtherSky = Sky(Fixture, 2.f);
	auto* Point = Fixture.Spawn<APointLight>();
	if (!TestNotNull(TEXT("Main sun"), Sun) || !TestNotNull(TEXT("Secondary sun"), OtherSun)
		|| !TestNotNull(TEXT("Main fill"), Fill) || !TestNotNull(TEXT("Additional fill"), OtherFill)
		|| !TestNotNull(TEXT("Main sky"), SkyLight) || !TestNotNull(TEXT("Secondary sky"), OtherSky)
		|| !TestNotNull(TEXT("Local practical light"), Point)) return false;
	Point->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Point->GetLightComponent()->SetIntensity(77.f);
	const FTransform SunTransform = Sun->GetActorTransform();
	const FLinearColor SunColor = Sun->GetLightComponent()->GetLightColor();
	const FTransform FillTransform = Fill->GetActorTransform();
	const FLinearColor FillColor = Fill->GetLightComponent()->GetLightColor();
	auto* Director = Fixture.Spawn<AMothArenaVisualDirector>();
	if (!TestNotNull(TEXT("Visual director"), Director)) return false;
	TestFalse(TEXT("Ordinary fixture worlds do not receive the SoulCity look"), Director->WasLookApplied());
	TestEqual(TEXT("Map gate leaves lights intact"), Sun->GetLightComponent()->Intensity, 11.f);
	OptIntoFixture(Director);
	TestTrue(TEXT("Explicit fixture map opt-in applies the look"), Director->WasLookApplied());
	TestTrue(TEXT("Main sun, sky and both shadow-free fills receive the look"),
		Sun->GetLightComponent()->Intensity != 11.f && SkyLight->GetLightComponent()->Intensity != 5.f
		&& Fill->GetLightComponent()->Intensity != .75f && OtherFill->GetLightComponent()->Intensity != 1.25f);
	TestTrue(TEXT("Secondary shadow sun, secondary sky and local practical light retain their authored intensity"),
		OtherSun->GetLightComponent()->Intensity == 3.f && OtherSky->GetLightComponent()->Intensity == 2.f
		&& Point->GetLightComponent()->Intensity == 77.f);
	TestTrue(TEXT("Lighting adjustments preserve authored direction, color and shadow choices"),
		Sun->GetActorTransform().Equals(SunTransform) && Fill->GetActorTransform().Equals(FillTransform)
		&& Sun->GetLightComponent()->GetLightColor().Equals(SunColor)
		&& Fill->GetLightComponent()->GetLightColor().Equals(FillColor)
		&& Sun->GetLightComponent()->CastShadows && !Fill->GetLightComponent()->CastShadows);
	const float AppliedSun = Sun->GetLightComponent()->Intensity;
	Director->ApplyLook();
	TestEqual(TEXT("Repeated apply does not compound intensity"), Sun->GetLightComponent()->Intensity, AppliedSun);
	TestTrue(TEXT("Repeated apply keeps the original sun and sky selection even after both become dimmer than secondary lights"),
		OtherSun->GetLightComponent()->Intensity == 3.f && OtherSky->GetLightComponent()->Intensity == 2.f);
	TArray<UPostProcessComponent*> PostProcesses;
	Director->GetComponents(PostProcesses);
	TestEqual(TEXT("Repeated apply retains one post process layer"), PostProcesses.Num(), 1);
	Director->RestoreLook();
	TestFalse(TEXT("Restore removes the applied state"), Director->WasLookApplied());
	TestTrue(TEXT("Restore uses original intensities after repeated apply"),
		Sun->GetLightComponent()->Intensity == 11.f && SkyLight->GetLightComponent()->Intensity == 5.f
		&& Fill->GetLightComponent()->Intensity == .75f && OtherFill->GetLightComponent()->Intensity == 1.25f);
	TestFalse(TEXT("Restored post process is disabled"), Director->GetPostProcessComponent()->bEnabled);
	Director->ApplyLook();
	Director->bEnabled = false;
	Director->ApplyLook();
	TestTrue(TEXT("Disabling the look also restores the authored lighting and removes post processing"),
		!Director->WasLookApplied() && !Director->GetPostProcessComponent()->bEnabled
		&& Sun->GetLightComponent()->Intensity == 11.f && SkyLight->GetLightComponent()->Intensity == 5.f);
	Director->bEnabled = true;
	Director->ApplyLook();
	Director->Destroy();
	TestTrue(TEXT("Destroy restores lights without leaving a runtime look behind"),
		Sun->GetLightComponent()->Intensity == 11.f && SkyLight->GetLightComponent()->Intensity == 5.f
		&& Fill->GetLightComponent()->Intensity == .75f && OtherFill->GetLightComponent()->Intensity == 1.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothArenaVisualIsolation,
	"MothEffect.ArenaVisual.PostProcessPreservesCombatAndCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothArenaVisualIsolation::RunTest(const FString&)
{
	using namespace MothArenaVisualTest;
	FMothCombatTestWorld Fixture;
	if (!TestNotNull(TEXT("Isolated combat world"), Fixture.World)) return false;
	auto* Cover = Fixture.Spawn<AActor>();
	if (!TestNotNull(TEXT("Physical cover"), Cover)) return false;
	auto* Box = NewObject<UBoxComponent>(Cover);
	Cover->AddInstanceComponent(Box);
	Cover->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(50));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->SetCanEverAffectNavigation(true);
	Box->RegisterComponent();
	Box->SetWorldLocation(FVector(0, 500, 100));
	const FTransform CoverTransform = Box->GetComponentTransform();
	const bool bCoverNavigation = Box->CanEverAffectNavigation();
	auto* EnemyPool = Fixture.World->GetSubsystem<UEnemyPoolSubsystem>();
	auto* ProjectilePool = Fixture.World->GetSubsystem<UProjectilePoolSubsystem>();
	auto* Lifecycle = Fixture.World->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (!TestNotNull(TEXT("Enemy pool"), EnemyPool) || !TestNotNull(TEXT("Projectile pool"), ProjectilePool)
		|| !TestNotNull(TEXT("Gameplay lifecycle"), Lifecycle)) return false;
	const FTransform EnemyTransform(FVector(1000, 0, 92));
	auto* Enemy = EnemyPool->AcquireEnemy(AEnemyBase::StaticClass(), EnemyTransform, nullptr);
	if (!TestNotNull(TEXT("Pooled active enemy"), Enemy)) return false;
	EnemyPool->FinishSpawningEnemy(Enemy, EnemyTransform);
	auto* Shot = ProjectilePool->SpawnProjectile(ARuleProjectile::StaticClass(), FTransform(FVector(0, 0, 1000)),
		FVector::ForwardVector, nullptr, nullptr);
	if (!TestNotNull(TEXT("Pooled in-flight projectile"), Shot)
		|| !TestFalse(TEXT("Fixture enemy admitted"), Enemy->IsPoolInactive())) return false;
	const FTransform EnemyPose = Enemy->GetActorTransform();
	const auto EnemyCollision = Enemy->GetCapsuleComponent()->GetCollisionEnabled();
	const bool bEnemyNavigation = Enemy->GetCapsuleComponent()->CanEverAffectNavigation();
	const float Health = Enemy->GetHealthComponent()->GetCurrentHealth();
	const FGuid ShotIdentity = Shot->GetHitId();
	const int32 ProjectileCount = Lifecycle->GetProjectileCount();
	const uint64 LifecycleGeneration = Lifecycle->GetLifecycleGeneration();
	FHitResult BeforeHit;
	TestTrue(TEXT("Authored cover physically blocks before look"), Fixture.World->LineTraceSingleByChannel(
		BeforeHit, FVector(-200, 500, 100), FVector(200, 500, 100), ECC_Visibility) && BeforeHit.GetActor() == Cover);
	auto* Director = Fixture.Spawn<AMothArenaVisualDirector>();
	if (!TestNotNull(TEXT("Visual director"), Director)) return false;
	OptIntoFixture(Director);
	auto* PostProcess = Director->GetPostProcessComponent();
	if (!TestNotNull(TEXT("Runtime post process component"), PostProcess)) return false;
	TestTrue(TEXT("Look covers the arena with an enabled unbound layer"),
		Director->WasLookApplied() && PostProcess->bEnabled && PostProcess->bUnbound && PostProcess->BlendWeight > 0.f);
	const FPostProcessSettings& Settings = PostProcess->Settings;
	TestTrue(TEXT("Combat retains explicit fixed exposure"),
		Settings.bOverride_AutoExposureMethod && Settings.AutoExposureMethod == AEM_Manual
		&& Settings.bOverride_AutoExposureApplyPhysicalCameraExposure && !Settings.AutoExposureApplyPhysicalCameraExposure);
	TestTrue(TEXT("Motion blur stays disabled while tonal grading is explicitly overridden"),
		Settings.bOverride_MotionBlurAmount && Settings.MotionBlurAmount == 0.f
		&& Settings.bOverride_ColorGammaShadows && Settings.bOverride_ColorSaturation);
	Director->ApplyLook();
	Director->RestoreLook();
	FHitResult AfterHit;
	TestTrue(TEXT("Look and restore preserve the physical blocker, transform and navigation participation"),
		Fixture.World->LineTraceSingleByChannel(AfterHit, FVector(-200, 500, 100), FVector(200, 500, 100), ECC_Visibility)
		&& AfterHit.GetActor() == Cover && Box->GetComponentTransform().Equals(CoverTransform)
		&& Box->CanEverAffectNavigation() == bCoverNavigation);
	TestTrue(TEXT("Enemy health, pose, collision and navigation remain unchanged"),
		!Enemy->IsPoolInactive() && Enemy->GetActorTransform().Equals(EnemyPose)
		&& Enemy->GetHealthComponent()->GetCurrentHealth() == Health
		&& Enemy->GetCapsuleComponent()->GetCollisionEnabled() == EnemyCollision
		&& Enemy->GetCapsuleComponent()->CanEverAffectNavigation() == bEnemyNavigation);
	TestTrue(TEXT("Active shot identity and gameplay admission survive look changes"),
		!Shot->IsPoolInactive() && Shot->GetHitId() == ShotIdentity && !Shot->HasProcessedImpact()
		&& Lifecycle->GetProjectileCount() == ProjectileCount && Lifecycle->GetLifecycleGeneration() == LifecycleGeneration
		&& Lifecycle->IsGameplayEnabled());
	TestTrue(TEXT("Look does not allocate or recycle combat actors"),
		EnemyPool->GetCreatedCount() == 1 && EnemyPool->GetInactiveCount() == 0 && EnemyPool->GetReuseCount() == 0
		&& ProjectilePool->GetCreatedCount() == 1 && ProjectilePool->GetInactiveCount() == 0 && ProjectilePool->GetReuseCount() == 0);
	return true;
}

#endif
