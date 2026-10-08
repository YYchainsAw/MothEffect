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
	const FCollisionResponseParams Responses(Body->GetCollisionResponseToChannels());
	FHitResult Hit;
	return !World->OverlapBlockingTestByChannel(Start, FQuat::Identity, Channel, Shape, Params, Responses)
		&& !World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, Channel, Shape, Params, Responses)
		&& !World->OverlapBlockingTestByChannel(End, FQuat::Identity, Channel, Shape, Params, Responses);
}
