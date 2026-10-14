#include "Devices/DeviceReleaseSafety.h"
#include "Devices/DeviceBase.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool MothDeviceRelease::IsPathClear(UWorld* World, const USphereComponent* Body,
	const FVector& Start, const FVector& End, const FCollisionQueryParams& Params, float RadiusOverrideCm)
{
	if (!World || !IsValid(Body) || Start.ContainsNaN() || End.ContainsNaN())
	{
		return false;
	}
	const float Radius = RadiusOverrideCm > 0.0f ? RadiusOverrideCm : Body->GetScaledSphereRadius();
	if (!FMath::IsFinite(Radius) || Radius <= 0.0f)
	{
		return false;
	}
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	const ECollisionChannel Channel = Body->GetCollisionObjectType();
	FCollisionResponseParams Responses(Body->GetCollisionResponseToChannels());
	// Airborne bodies ignore Pawn contact, but releases still need a clear birth volume.
	Responses.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
	FHitResult Hit;
	if (World->OverlapBlockingTestByChannel(Start, FQuat::Identity, Channel, Shape, Params, Responses)
		|| World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, Channel, Shape, Params, Responses)
		|| World->OverlapBlockingTestByChannel(End, FQuat::Identity, Channel, Shape, Params, Responses)) return false;
	// Fixed devices have no entity collider. Keep births/releases out of their actual body
	// volume without giving them physical blocking or using the larger ballistic query sphere.
	for (TActorIterator<ADeviceBase> It(World); It; ++It)
	{
		const ADeviceBase* Device = *It;
		if (!IsValid(Device) || Device->IsActorBeingDestroyed() || !Device->IsGrounded()
			|| Device == Body->GetOwner() || Params.GetIgnoredActors().Contains(Device->GetUniqueID())
			|| Device->GetDeviceState() == EDeviceState::Spent || Device->GetDeviceState() == EDeviceState::Destroyed) continue;
		const USphereComponent* OtherBody = Device->GetPhysicsBody();
		const float CombinedRadius = Radius + OtherBody->GetScaledSphereRadius();
		if (FMath::PointDistToSegmentSquared(OtherBody->GetComponentLocation(), Start, End) < FMath::Square(CombinedRadius)) return false;
	}
	return true;
}
