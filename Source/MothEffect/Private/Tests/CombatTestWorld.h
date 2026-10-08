#pragma once

#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"

/** No game mode, engine tick or asset dependencies. Tests dispatch BeginPlay explicitly. */
struct FMothCombatTestWorld
{
	UWorld* World = nullptr;

	FMothCombatTestWorld()
	{
		if (GEngine)
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (World)
			{
				GEngine->CreateNewWorldContext(World->WorldType).SetCurrentWorld(World);
				World->SetShouldTick(false);
				World->InitializeActorsForPlay(FURL());
			}
		}
	}

	~FMothCombatTestWorld()
	{
		if (World)
		{
			World->BeginTearingDown();
			for (FActorIterator It(World); It; ++It)
			{
				It->RouteEndPlay(EEndPlayReason::Quit);
			}
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	}

	template<class T>
	T* Spawn(const FVector& Location = FVector::ZeroVector)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		T* Actor = World ? World->SpawnActor<T>(Location, FRotator::ZeroRotator, Params) : nullptr;
		if (Actor)
		{
			Actor->DispatchBeginPlay();
		}
		return Actor;
	}
};
