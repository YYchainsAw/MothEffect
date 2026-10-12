// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/MothEffectGameMode.h"
#include "Framework/MothWaveDirector.h"
#include "EngineUtils.h"
#include "Tutorial/MothTutorialSubsystem.h"

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
	if (!Director || Director->GetPhase() != EMothFlowPhase::Ready) return false;
	GetWorld()->GetSubsystem<UMothTutorialSubsystem>()->EndTutorial();
	return Director->StartEncounter();
}

bool AMothEffectGameMode::RestartEncounter()
{
	AMothWaveDirector* Director = GetWaveDirector();
	if (!Director) return false;
	GetWorld()->GetSubsystem<UMothTutorialSubsystem>()->EndTutorial();
	return Director->RestartEncounter();
}
