#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MothGameInstance.generated.h"

/** Only a one-shot travel intention survives a restart; encounter data belongs to the world. */
UCLASS()
class MOTHEFFECT_API UMothGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	static FName GetMainMenuMapName() { return FName(TEXT("/Game/MothEffect/Maps/L_MainMenu")); }
	static FName GetBattleMapName() { return FName(TEXT("/Game/MothEffect/Maps/L_Arena_SoulCity")); }
	void QueueBattleAfterTravel() { bBattleAfterTravel = true; }
	void CancelBattleAfterTravel() { bBattleAfterTravel = false; }
	bool HasBattleAfterTravel() const { return bBattleAfterTravel; }
	bool ConsumeBattleAfterTravel() { const bool bResult = bBattleAfterTravel; bBattleAfterTravel = false; return bResult; }
private:
	bool bBattleAfterTravel = false;
};
