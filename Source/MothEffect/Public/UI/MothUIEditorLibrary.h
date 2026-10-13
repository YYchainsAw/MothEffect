#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MothUIEditorLibrary.generated.h"

/** Editor Python bridge. Never replaces an existing Designer tree or saves unrelated packages. */
UCLASS()
class MOTHEFFECT_API UMothUIEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Populate an empty T18 scaffold, validate required controls, compile; returns a JSON report. */
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static FString PrepareDesignerLayout(UObject* WidgetAsset);
};
