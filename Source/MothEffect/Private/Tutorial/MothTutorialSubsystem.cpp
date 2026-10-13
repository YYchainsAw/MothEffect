#include "Tutorial/MothTutorialSubsystem.h"
#include "Tutorial/MothTutorialTarget.h"
#include "Tutorial/MothTutorialConfig.h"
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

const UMothTutorialConfig* UMothTutorialSubsystem::GetConfig() const
{
	return Config ? Config.Get() : GetDefault<UMothTutorialConfig>();
}

bool UMothTutorialSubsystem::StartTutorial(AMothEffectCharacter* InPlayer, AMothWaveDirector* InDirector, const UMothTutorialConfig* InConfig)
{
	if (!IsValid(InPlayer) || !IsValid(InDirector) || InPlayer->GetWorld() != GetWorld()
		|| InDirector->GetWorld() != GetWorld() || InDirector->GetPhase() != EMothFlowPhase::Ready
		|| !InPlayer->GetHealthComponent()->IsAlive() || GetWorld()->IsPaused()) return false;
	EndTutorial();
	Config = DuplicateObject<UMothTutorialConfig>(InConfig ? InConfig : GetDefault<UMothTutorialConfig>(), this);
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
	{
		if (auto* Device = Cast<ADeviceBase>(Actor.Get())) Device->OnActivatedNative.RemoveAll(this);
		if (auto* Target = Cast<AMothTutorialBlastTarget>(Actor.Get())) Target->GetHealthComponent()->OnDeathCommittedNative.RemoveAll(this);
	}
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
	Config = nullptr;
	Feedback = FText::GetEmpty(); FeedbackEndsAt = 0;
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
	bBombHit = bLiftHit = bLiftObserved = bBlastGoal = bThrown = bAirborneHit = bIndirectHit = bRetryAir = false;
	ThrowCommittedAt = 0;
	Feedback = FText::GetEmpty(); FeedbackEndsAt = 0;
	const auto* Settings = GetConfig();
	if (Next == EMothTutorialStep::BombAndLift)
	{
		SpawnDevice(EDeviceKind::Bomb, Settings->BombLocation);
		BlastTarget = SpawnBlastTarget(Settings->BlastTargetLocation);
		SpawnDevice(EDeviceKind::Launcher, Settings->LauncherLocation);
		LiftTarget = SpawnTarget(Settings->LiftTargetLocation, true);
		LiftStartZ = LiftTarget.IsValid() ? LiftTarget->GetActorLocation().Z : 0;
	}
	else if (Next == EMothTutorialStep::AirborneEmitter && Player.IsValid())
	{
		const FVector Offset = Settings->EmitterPlayerOffset;
		const FVector Location = Player->GetActorLocation() + Player->GetActorForwardVector() * Offset.X
			+ Player->GetActorRightVector() * Offset.Y + FVector::UpVector * Offset.Z;
		ThrownEmitter = SpawnDevice(EDeviceKind::Emitter, Location);
	}
	else if (Next == EMothTutorialStep::IndirectChain)
	{
		SpawnDevice(EDeviceKind::Emitter, Settings->ChainEmitterLocation);
		SpawnDevice(EDeviceKind::Bomb, Settings->ChainBombLocation);
		BlastTarget = SpawnBlastTarget(Settings->ChainTargetLocation);
		FActorSpawnParameters Params;
		auto* Marker = GetWorld()->SpawnActor<AActor>(Settings->ArrowLocation, Settings->ArrowRotation, Params);
		if (Marker)
		{
			OwnedActors.Add(Marker);
			auto* Arrow = NewObject<UArrowComponent>(Marker);
			Marker->SetRootComponent(Arrow); Marker->AddInstanceComponent(Arrow);
			Arrow->ArrowColor = FColor(181, 146, 239); Arrow->ArrowLength = 300;
			Arrow->SetHiddenInGame(false); Arrow->RegisterComponent();
			Marker->SetActorTransform(FTransform(Settings->ArrowRotation, Settings->ArrowLocation));
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
	if (Target)
	{
		OwnedActors.Add(Target);
		Target->GetHealthComponent()->OnDeathCommittedNative.AddUObject(this, &UMothTutorialSubsystem::HandleTargetDied);
	}
	return Target;
}

void UMothTutorialSubsystem::RetryStep()
{
	if (IsRunning() && Player.IsValid() && !GetWorld()->IsPaused())
	{
		BuildStep(Step); Feedback=GetConfig()->ResetFeedback; FeedbackEndsAt=GetWorld()->GetTimeSeconds()+3;
	}
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
	if (Step == EMothTutorialStep::AirborneEmitter && Device == ThrownEmitter.Get() && bThrow)
	{
		bThrown = true; ThrowCommittedAt = GetWorld()->GetTimeSeconds();
	}
}

void UMothTutorialSubsystem::HandleTargetDied(AActor* Victim, AActor* Source)
{
	if (Victim == BlastTarget.Get()) bBlastGoal = Source == GetPracticeDevice(EDeviceKind::Bomb);
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
		if (bLiftHit && LiftTarget.IsValid() && LiftTarget->GetActorLocation().Z > LiftStartZ + 100) bLiftObserved = true;
		if (bBombHit && bBlastGoal && bLiftObserved)
			BuildStep(EMothTutorialStep::AirborneEmitter);
	}
	else if (Step == EMothTutorialStep::AirborneEmitter)
	{
		if (bAirborneHit) BuildStep(EMothTutorialStep::IndirectChain);
		else if (bRetryAir || !ThrownEmitter.IsValid()
			|| (bThrown && GetWorld()->GetTimeSeconds() - ThrowCommittedAt > .6 && !IsAirborne(ThrownEmitter.Get()))
			|| ThrownEmitter->GetDeviceState() == EDeviceState::Spent || ThrownEmitter->GetDeviceState() == EDeviceState::Destroyed)
		{
			const bool bInvalidHit = bRetryAir;
			BuildStep(EMothTutorialStep::AirborneEmitter);
			Feedback = bInvalidHit ? GetConfig()->InvalidAirHitFeedback : GetConfig()->RefillFeedback;
			FeedbackEndsAt = GetWorld()->GetTimeSeconds() + 3;
		}
	}
	else if (Step == EMothTutorialStep::IndirectChain && bIndirectHit && bBlastGoal) BuildStep(EMothTutorialStep::Completed);
}

FText UMothTutorialSubsystem::GetFeedback() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < FeedbackEndsAt ? Feedback : FText::GetEmpty();
}

FText UMothTutorialSubsystem::GetInstruction() const
{
	switch (Step)
	{
	case EMothTutorialStep::BombAndLift: return GetConfig()->BombAndLiftInstruction;
	case EMothTutorialStep::AirborneEmitter: return GetConfig()->AirborneInstruction;
	case EMothTutorialStep::IndirectChain: return GetConfig()->IndirectInstruction;
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
