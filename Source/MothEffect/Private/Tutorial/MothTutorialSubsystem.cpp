#include "Tutorial/MothTutorialSubsystem.h"
#include "Tutorial/MothTutorialTarget.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Devices/DeviceBase.h"
#include "Engine/World.h"
#include "Framework/MothWaveDirector.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Weapons/Rifle.h"

bool UMothTutorialSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

bool UMothTutorialSubsystem::StartTutorial(AMothEffectCharacter* InPlayer, AMothWaveDirector* InDirector)
{
	if (!IsValid(InPlayer) || !IsValid(InDirector) || InPlayer->GetWorld() != GetWorld()
		|| InDirector->GetWorld() != GetWorld() || InDirector->GetPhase() != EMothFlowPhase::Ready
		|| !InPlayer->GetHealthComponent()->IsAlive() || GetWorld()->IsPaused()) return false;
	EndTutorial();
	Player = InPlayer;
	Director = InDirector;
	Player->SetGameplayEnabled(true);
	Player->OnDeviceReleasedNative.AddUObject(this, &UMothTutorialSubsystem::HandleRelease);
	BuildStep(EMothTutorialStep::BombAndLift);
	GetWorld()->GetTimerManager().SetTimer(CheckTimer, this, &UMothTutorialSubsystem::CheckProgress, 0.05f, true);
	return true;
}

void UMothTutorialSubsystem::ClearOwnedActors()
{
	// Detach evidence observers before destroying actors, including the player's held prop.
	for (const auto& Actor : OwnedActors)
		if (auto* Device = Cast<ADeviceBase>(Actor.Get())) Device->OnActivatedNative.RemoveAll(this);
	const auto Previous = MoveTemp(OwnedActors);
	OwnedActors.Reset();
	BlastTarget.Reset(); LiftTarget.Reset(); ThrownEmitter.Reset();
	for (const auto& Actor : Previous)
		if (Actor.IsValid()) Actor->Destroy();
}

void UMothTutorialSubsystem::EndTutorial()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(CheckTimer);
	if (Player.IsValid()) Player->OnDeviceReleasedNative.RemoveAll(this);
	ClearOwnedActors();
	Player.Reset(); Director.Reset();
	if (Step != EMothTutorialStep::Inactive)
	{
		Step = EMothTutorialStep::Inactive;
		OnStepChanged.Broadcast(Step);
	}
}

void UMothTutorialSubsystem::OnWorldEndPlay(UWorld& World) { EndTutorial(); Super::OnWorldEndPlay(World); }
void UMothTutorialSubsystem::Deinitialize() { EndTutorial(); OnStepChanged.Clear(); Super::Deinitialize(); }

ADeviceBase* UMothTutorialSubsystem::SpawnDevice(EDeviceKind Kind, FVector Location)
{
	const TCHAR* Path = Kind == EDeviceKind::Launcher ? TEXT("/Game/MothEffect/Devices/BP_DeviceLauncher.BP_DeviceLauncher_C")
		: Kind == EDeviceKind::Bomb ? TEXT("/Game/MothEffect/Devices/BP_DeviceBomb.BP_DeviceBomb_C")
		: TEXT("/Game/MothEffect/Devices/BP_DeviceEmitter.BP_DeviceEmitter_C");
	UClass* Class = LoadClass<ADeviceBase>(nullptr, Path);
	ADeviceBase* Device = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>()->SpawnDevice(Class, FTransform(Location));
	if (Device)
	{
		OwnedActors.Add(Device);
		Device->OnActivatedNative.AddUObject(this, &UMothTutorialSubsystem::HandleActivation);
	}
	return Device;
}

AMothTutorialTarget* UMothTutorialSubsystem::SpawnTarget(FVector Location, bool bPhysics)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Target = GetWorld()->SpawnActor<AMothTutorialTarget>(Location, FRotator::ZeroRotator, Params);
	if (Target)
	{
		OwnedActors.Add(Target);
		if (bPhysics) Target->EnableLiftPhysics();
	}
	return Target;
}

void UMothTutorialSubsystem::BuildStep(EMothTutorialStep Next)
{
	ClearOwnedActors();
	Step = Next;
	bBombHit = bLiftHit = bThrown = bAirborneHit = bIndirectHit = bRetryAir = false;
	if (Next == EMothTutorialStep::BombAndLift)
	{
		SpawnDevice(EDeviceKind::Bomb, FVector(-2200, -850, 40));
		BlastTarget = SpawnBlastTarget(FVector(-2020, -850, 40));
		SpawnDevice(EDeviceKind::Launcher, FVector(-1700, -1200, 40));
		LiftTarget = SpawnTarget(FVector(-1700, -1200, 130), true);
		LiftStartZ = LiftTarget.IsValid() ? LiftTarget->GetActorLocation().Z : 0;
	}
	else if (Next == EMothTutorialStep::AirborneEmitter && Player.IsValid())
	{
		const FVector Location = Player->GetActorLocation() + Player->GetActorForwardVector() * 140 + FVector(0, 0, -50);
		ThrownEmitter = SpawnDevice(EDeviceKind::Emitter, Location);
	}
	else if (Next == EMothTutorialStep::IndirectChain)
	{
		SpawnDevice(EDeviceKind::Emitter, FVector(-2200, -1200, 40));
		SpawnDevice(EDeviceKind::Bomb, FVector(-1600, -1200, 40));
		BlastTarget = SpawnBlastTarget(FVector(-1420, -1200, 40));
		FActorSpawnParameters Params;
		auto* Marker = GetWorld()->SpawnActor<AActor>(FVector(-2520, -1200, 80), FRotator::ZeroRotator, Params);
		if (Marker)
		{
			OwnedActors.Add(Marker);
			auto* Arrow = NewObject<UArrowComponent>(Marker);
			Marker->SetRootComponent(Arrow); Marker->AddInstanceComponent(Arrow);
			Arrow->ArrowColor = FColor(181, 146, 239); Arrow->ArrowLength = 300;
			Arrow->SetHiddenInGame(false); Arrow->RegisterComponent();
			Marker->SetActorLocation(FVector(-2520, -1200, 80));
		}
	}
	else if (Next == EMothTutorialStep::Completed)
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckTimer);
		if (Player.IsValid()) Player->OnDeviceReleasedNative.RemoveAll(this);
	}
	OnStepChanged.Broadcast(Step);
}

AMothTutorialBlastTarget* UMothTutorialSubsystem::SpawnBlastTarget(FVector Location)
{
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Target=GetWorld()->SpawnActor<AMothTutorialBlastTarget>(Location,FRotator::ZeroRotator,Params);
	if (Target) OwnedActors.Add(Target); return Target;
}

void UMothTutorialSubsystem::RetryStep()
{
	if (IsRunning() && Player.IsValid() && !GetWorld()->IsPaused()) BuildStep(Step);
}

bool UMothTutorialSubsystem::IsAirborne(ADeviceBase* Device) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothTutorialAirborne), false, Device);
	const FVector Position = Device->GetActorLocation();
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, Position, Position - FVector(0, 0, 1500),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params)) return false;
	return Position.Z - Hit.ImpactPoint.Z > Device->GetPhysicsBody()->GetScaledSphereRadius() + 20;
}

void UMothTutorialSubsystem::HandleRelease(ADeviceBase* Device, bool bThrow)
{
	if (Step == EMothTutorialStep::AirborneEmitter && Device == ThrownEmitter.Get() && bThrow) bThrown = true;
}

void UMothTutorialSubsystem::HandleActivation(ADeviceBase* Device, const FHitContext& Context)
{
	const auto* Rifle = Cast<ARifle>(Context.SourceActor);
	const bool bPlayerShot = Rifle && Rifle->GetOwner() == Player.Get();
	if (Step == EMothTutorialStep::BombAndLift && bPlayerShot)
	{
		if (Device->GetDeviceKind() == EDeviceKind::Bomb) bBombHit = true;
		if (Device->GetDeviceKind() == EDeviceKind::Launcher) bLiftHit = true;
	}
	if (Step == EMothTutorialStep::AirborneEmitter && Device == ThrownEmitter.Get())
	{
		bAirborneHit = bPlayerShot && bThrown && IsAirborne(Device);
		bRetryAir = !bAirborneHit;
	}
	if (Step == EMothTutorialStep::IndirectChain && Device->GetDeviceKind() == EDeviceKind::Bomb)
	{
		// P01 preserves its firing D03 as SourceActor; a direct W01 carries the rifle.
		bIndirectHit = Context.SourceActor == GetPracticeDevice(EDeviceKind::Emitter);
	}
}

void UMothTutorialSubsystem::CheckProgress()
{
	if (Director.IsValid()) Director->EndPracticeIfPlayerInvalid();
	if (!Player.IsValid() || !Player->GetHealthComponent()->IsAlive() || !Director.IsValid()
		|| Director->GetPhase() != EMothFlowPhase::Ready) { EndTutorial(); return; }
	if (Step == EMothTutorialStep::BombAndLift)
	{
		if (bBombHit && BlastTarget.IsValid() && !BlastTarget->GetHealthComponent()->IsAlive()
			&& bLiftHit && LiftTarget.IsValid() && LiftTarget->GetActorLocation().Z > LiftStartZ + 100)
			BuildStep(EMothTutorialStep::AirborneEmitter);
	}
	else if (Step == EMothTutorialStep::AirborneEmitter)
	{
		if (bAirborneHit) BuildStep(EMothTutorialStep::IndirectChain);
		else if (bRetryAir || !ThrownEmitter.IsValid()
			|| ThrownEmitter->GetDeviceState() == EDeviceState::Spent || ThrownEmitter->GetDeviceState() == EDeviceState::Destroyed)
			BuildStep(EMothTutorialStep::AirborneEmitter);
	}
	else if (Step == EMothTutorialStep::IndirectChain && bIndirectHit && BlastTarget.IsValid()
		&& !BlastTarget->GetHealthComponent()->IsAlive()) BuildStep(EMothTutorialStep::Completed);
}

FText UMothTutorialSubsystem::GetInstruction() const
{
	switch (Step)
	{
	case EMothTutorialStep::BombAndLift: return FText::FromString(TEXT("射击橙色爆裂器，炸毁方块；再射击青色抬升器，把箱子抬起。爆炸也会伤到自己，保持距离。"));
	case EMothTutorialStep::AirborneEmitter: return FText::FromString(TEXT("对准紫色发射器按 E 拾取，左键投出；松开左键，再次按下射击空中的道具。启动瞬间的弹道决定发射方向，此后不会跟随准星。失败会补充道具。"));
	case EMothTutorialStep::IndirectChain: return FText::FromString(TEXT("沿紫色箭头从左向右射击发射器，让它的弹丸启动前方爆裂器，间接炸毁方块。方向锁定后，移动准星不会改变连锁。"));
	default: return FText::GetEmpty();
	}
}

int32 UMothTutorialSubsystem::GetOwnedActorCount() const
{
	int32 Count = 0; for (const auto& Actor : OwnedActors) if (Actor.IsValid()) ++Count; return Count;
}

ADeviceBase* UMothTutorialSubsystem::GetPracticeDevice(EDeviceKind Kind) const
{
	for (const auto& Actor : OwnedActors)
		if (auto* Device = Cast<ADeviceBase>(Actor.Get()); Device && Device->GetDeviceKind() == Kind) return Device;
	return nullptr;
}
