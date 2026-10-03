#pragma once

#include "CoreMinimal.h"
#include "PlayerActionState.generated.h"

UENUM(BlueprintType)
enum class EPlayerActionState : uint8
{
	Ready,
	Carrying,
	ThrowRecovery,
	Reloading,
	Dead
};

UENUM(BlueprintType)
enum class EPrimaryPressMode : uint8
{
	None,
	Fire,
	Throw,
	Blocked
};
