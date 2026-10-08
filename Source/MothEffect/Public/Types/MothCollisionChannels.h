#pragma once

#include "Engine/EngineTypes.h"

namespace MothCollision
{
	// Keep in sync with JamProjectile in Config/DefaultEngine.ini.
	inline constexpr ECollisionChannel Projectile = ECC_GameTraceChannel1;
}
