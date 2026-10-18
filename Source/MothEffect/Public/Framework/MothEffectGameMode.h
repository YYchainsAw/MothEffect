// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MothEffectGameMode.generated.h"

class AMothWaveDirector;
class AMothPrototypeSceneArt;
class AMothArenaVisualDirector;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AMothEffectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AMothEffectGameMode();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moth|Prototype") TSubclassOf<AMothPrototypeSceneArt> PrototypeSceneArtClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moth|Art") TSubclassOf<AMothArenaVisualDirector> ArenaVisualDirectorClass;
	UFUNCTION(BlueprintPure, Category="Moth|Flow")
	AMothWaveDirector* GetWaveDirector() const;
	UFUNCTION(BlueprintCallable, Category="Moth|Flow")
	bool StartEncounter();
	UFUNCTION(BlueprintCallable, Category="Moth|Flow")
	bool RestartEncounter();
protected:
	virtual void BeginPlay() override;
};



