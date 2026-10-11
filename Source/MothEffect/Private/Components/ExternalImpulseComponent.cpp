#include "Components/ExternalImpulseComponent.h"
#include "Components/HealthComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Devices/DeviceBase.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UExternalImpulseComponent::UExternalImpulseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UExternalImpulseComponent::BeginPlay()
{
	Super::BeginPlay();
	WorldTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this,
		&UExternalImpulseComponent::SettleImpulses);
}

void UExternalImpulseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FWorldDelegates::OnWorldPostActorTick.Remove(WorldTickHandle);
	WorldTickHandle.Reset();
	bGameplayEnabled = false;
	ClearPending();
	AcceptedEvents.Reset();
	Super::EndPlay(EndPlayReason);
}

bool UExternalImpulseComponent::CanReceive() const
{
	const AActor* Owner = GetOwner();
	if (!bGameplayEnabled || !HasBegunPlay() || !IsValid(Owner) || Owner->IsActorBeingDestroyed()
		|| !GetWorld() || GetWorld()->IsPaused()) return false;
	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		const UHealthComponent* Health = Owner->FindComponentByClass<UHealthComponent>();
		return Character->GetCharacterMovement()->MovementMode != MOVE_None && (!Health || Health->IsAlive());
	}
	const ADeviceBase* Device = Cast<ADeviceBase>(Owner);
	const UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	return Body && Body->IsSimulatingPhysics() && (!Device || Device->CanReceiveExternalImpulse());
}

bool UExternalImpulseComponent::QueueImpulse(const FVector& Velocity, bool bOverrideZ, const FGuid& EventId)
{
	if (!CanReceive() || !EventId.IsValid() || Velocity.ContainsNaN() || AcceptedEvents.Contains(EventId))
		return false;
	AcceptedEvents.Add(EventId);
	if (bOverrideZ)
	{
		PendingZ = bHasOverrideZ ? FMath::Max(PendingZ, static_cast<float>(Velocity.Z)) : Velocity.Z;
		bHasOverrideZ = true;
	}
	else PendingVelocity += Velocity;
	return true;
}

void UExternalImpulseComponent::ClearPending()
{
	PendingVelocity = FVector::ZeroVector;
	PendingZ = 0.0f;
	bHasOverrideZ = false;
}

void UExternalImpulseComponent::SetGameplayEnabled(bool bEnabled)
{
	bGameplayEnabled = bEnabled;
	if (!bEnabled)
	{
		ClearPending();
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
			Character->GetCharacterMovement()->ClearAccumulatedForces();
	}
}

void UExternalImpulseComponent::SettleImpulses(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (World != GetWorld() || World->IsPaused()) return;
	if (!CanReceive()) { ClearPending(); return; }
	if (PendingVelocity.IsNearlyZero() && !bHasOverrideZ) return;
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	const FVector Current = Character ? Character->GetCharacterMovement()->Velocity : Body->GetPhysicsLinearVelocity();
	FVector Final = Current + PendingVelocity;
	if (bHasOverrideZ) Final.Z = PendingZ;
	Final = Final.GetClampedToMaxSize(FMath::Max(1.0f, MaxExternalVelocityCmPerSec));
	// Clear before LaunchCharacter's presentation event can re-enter the receiver.
	ClearPending();
	if (Character) Character->LaunchCharacter(Final, true, true);
	else Body->AddImpulse(Final - Current, NAME_None, true);
}
