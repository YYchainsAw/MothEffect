#pragma once

#include "CoreMinimal.h"
#include "DeviceTypes.generated.h"

UENUM(BlueprintType)
enum class EDeviceState : uint8
{
	Dormant,
	Held,
	Active,
	Spent,
	Destroyed
};

UENUM(BlueprintType)
enum class EDeviceKind : uint8
{
	Launcher,
	Bomb,
	Emitter
};
