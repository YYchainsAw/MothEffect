// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/MothEffectGameMode.h"
#include "Framework/MothWaveDirector.h"
#include "EngineUtils.h"

AMothEffectGameMode::AMothEffectGameMode()
{
}

AMothWaveDirector* AMothEffectGameMode::GetWaveDirector() const
{
	for (TActorIterator<AMothWaveDirector> It(GetWorld()); It; ++It)
		if (!It->IsActorBeingDestroyed()) return *It;
	return nullptr;
}

bool AMothEffectGameMode::StartEncounter()
{
	AMothWaveDirector* Director = GetWaveDirector();
	return Director && Director->StartEncounter();
}

bool AMothEffectGameMode::RestartEncounter()
{
	AMothWaveDirector* Director = GetWaveDirector();
	return Director && Director->RestartEncounter();
}
