#pragma once

#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "TimerManager.h"

/** No game mode, engine tick or asset dependencies. World-level BeginPlay/EndPlay are paired. */
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
				World->GetWorldSettings()->NotifyBeginPlay();
			}
		}
	}

	~FMothCombatTestWorld()
	{
		if (World)
		{
			// UE 5.8 also ends subsystems and clears the world's BegunPlay flag here.
			// Ending actors individually leaves CleanupWorld reporting a missing EndPlay.
			World->EndPlay(EEndPlayReason::Quit);
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
			if (!Actor->HasActorBegunPlay())
			{
				Actor->DispatchBeginPlay();
			}
		}
		return Actor;
	}
};

/** Keep an isolated fixture alive while real World ticks run its timers and physics. */
class FMothTickWorldUntil final : public IAutomationLatentCommand
{
public:
	FMothTickWorldUntil(TSharedPtr<FMothCombatTestWorld> InFixture, float Duration, TFunction<void()> InCheck)
		: Fixture(MoveTemp(InFixture)), Check(MoveTemp(InCheck))
	{
		Until = Fixture->World->GetTimeSeconds() + Duration;
	}
	bool Update() override
	{
		UWorld* World = Fixture->World;
		if (World->GetTimerManager().HasBeenTickedThisFrame()) return false;
		World->Tick(LEVELTICK_All, 0.01f);
		if (World->GetTimeSeconds() < Until) return false;
		Check();
		return true;
	}
private:
	TSharedPtr<FMothCombatTestWorld> Fixture;
	TFunction<void()> Check;
	double Until = 0;
};
