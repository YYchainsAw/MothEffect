#pragma once

#include "Engine/EngineTypes.h"

namespace MothCollision
{
	// Keep in sync with JamProjectile in Config/DefaultEngine.ini.
	inline constexpr ECollisionChannel Projectile = ECC_GameTraceChannel1;
	// World -Z launcher probe; only WorldStatic objects are admitted by the query.
	inline constexpr ECollisionChannel GroundTrace = ECC_GameTraceChannel2;
}
