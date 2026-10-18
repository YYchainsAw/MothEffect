// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/MothEffectGameMode.h"
#include "Framework/MothWaveDirector.h"
#include "EngineUtils.h"
#include "Tutorial/MothTutorialSubsystem.h"
#include "Art/MothPrototypeSceneArt.h"
#include "Art/MothArenaVisualDirector.h"
#include "Kismet/GameplayStatics.h"

AMothEffectGameMode::AMothEffectGameMode()
{
	PrototypeSceneArtClass=AMothPrototypeSceneArt::StaticClass();
	ArenaVisualDirectorClass=AMothArenaVisualDirector::StaticClass();
}

void AMothEffectGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (ArenaVisualDirectorClass && UGameplayStatics::GetCurrentLevelName(this,true)==TEXT("L_Arena_SoulCity"))
	{
		bool bHasVisualDirector = false;
		for (TActorIterator<AMothArenaVisualDirector> It(GetWorld()); It; ++It)
		{
			if (!It->IsActorBeingDestroyed()) { bHasVisualDirector = true; break; }
		}
		if (!bHasVisualDirector)
		{
			FActorSpawnParameters Params; Params.ObjectFlags|=RF_Transient;
			GetWorld()->SpawnActor<AMothArenaVisualDirector>(ArenaVisualDirectorClass,FTransform::Identity,Params);
		}
	}
	if (!PrototypeSceneArtClass || UGameplayStatics::GetCurrentLevelName(this,true)!=TEXT("L_Arena_01")) return;
	for (TActorIterator<AMothPrototypeSceneArt> It(GetWorld()); It; ++It) if (!It->IsActorBeingDestroyed()) return;
	FActorSpawnParameters Params; Params.ObjectFlags|=RF_Transient;
	GetWorld()->SpawnActor<AMothPrototypeSceneArt>(PrototypeSceneArtClass,FTransform::Identity,Params);
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
