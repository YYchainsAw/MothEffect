#pragma once

#include "Engine/EngineTypes.h"

namespace MothCollision
{
	// Enemies and grounded, unactivated drops share the same targeting silhouette.
	inline constexpr float EnemyCapsuleRadiusCm = 35.0f;
	inline constexpr float EnemyCapsuleHalfHeightCm = 90.0f;
	// Keep in sync with JamProjectile in Config/DefaultEngine.ini.
	inline constexpr ECollisionChannel Projectile = ECC_GameTraceChannel1;
	// World -Z launcher probe; only WorldStatic objects are admitted by the query.
	inline constexpr ECollisionChannel GroundTrace = ECC_GameTraceChannel2;
}
