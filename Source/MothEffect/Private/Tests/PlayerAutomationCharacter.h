#pragma once

#include "Characters/MothEffectCharacter.h"
#include "PlayerAutomationCharacter.generated.h"

/** Transient native fixture for input/lifecycle tests; never placed in gameplay maps. */
UCLASS(Transient, NotBlueprintable, NotPlaceable)
class AMothPlayerAutomationCharacter : public AMothEffectCharacter
{
	GENERATED_BODY()

public:
	AMothPlayerAutomationCharacter();
	virtual bool IsEditorOnly() const override { return true; }
	void SetPrimaryDown(bool bDown) { bAutomationPrimaryDown = bDown; }
	void SetInputFocused(bool bFocused) { bAutomationFocused = bFocused; }

protected:
	virtual void BeginPlay() override;
	virtual bool IsPrimaryButtonPhysicallyDown() const override { return bAutomationPrimaryDown; }
	virtual bool IsCombatInputFocused() const override { return bAutomationFocused; }

private:
	bool bAutomationPrimaryDown = false;
	bool bAutomationFocused = true;
};
