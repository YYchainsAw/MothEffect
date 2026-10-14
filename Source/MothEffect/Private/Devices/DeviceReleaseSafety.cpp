#include "Devices/DeviceReleaseSafety.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"

bool MothDeviceRelease::IsPathClear(UWorld* World, const USphereComponent* Body,
	const FVector& Start, const FVector& End, const FCollisionQueryParams& Params)
{
	if (!World || !IsValid(Body) || Start.ContainsNaN() || End.ContainsNaN())
	{
		return false;
	}
	const float Radius = Body->GetScaledSphereRadius();
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
	return !World->OverlapBlockingTestByChannel(Start, FQuat::Identity, Channel, Shape, Params, Responses)
		&& !World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, Channel, Shape, Params, Responses)
		&& !World->OverlapBlockingTestByChannel(End, FQuat::Identity, Channel, Shape, Params, Responses);
}
