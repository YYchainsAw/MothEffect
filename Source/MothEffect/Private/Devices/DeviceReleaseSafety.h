#pragma once

#include "CoreMinimal.h"

class UWorld;
class USphereComponent;
struct FCollisionQueryParams;

namespace MothDeviceRelease
{
	/** Uses the physical root's scaled sphere and collision responses, including Pawns/props. */
	bool IsPathClear(UWorld* World, const USphereComponent* Body, const FVector& Start,
		const FVector& End, const FCollisionQueryParams& Params);
}
