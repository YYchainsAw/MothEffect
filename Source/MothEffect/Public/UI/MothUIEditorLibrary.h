#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MothUIEditorLibrary.generated.h"

class UWidget;
class UWidgetTree;
class UUserWidget;

/** Editor-only asset authoring bridge. Never constructs a runtime HUD or saves packages. */
UCLASS()
class MOTHEFFECT_API UMothUIEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Populate an empty T18 scaffold, validate required controls, compile; returns a JSON report. */
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static FString PrepareDesignerLayout(UObject* WidgetAsset);
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static UWidgetTree* GetDesignerTree(UObject* WidgetAsset);
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static UWidget* NewDesignerWidget(UObject* WidgetAsset, TSubclassOf<UWidget> WidgetClass, FName Name);
	/** Explicit migration only; preserve named user widgets and all Blueprint event graphs. */
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static FString ResetDesignerLayout(UObject* WidgetAsset, const TArray<FName>& PreserveWidgets);
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static bool SetDesignerRoot(UObject* WidgetAsset, UWidget* Root);
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static TArray<UWidget*> ListDesignerWidgets(UObject* WidgetAsset);
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static UWidget* GetNamedWidget(UUserWidget* Widget, FName Name);
	/** Capture only the game's viewport, including UMG, for editor verification. */
	UFUNCTION(BlueprintCallable, Category="Moth|Editor", meta=(DevelopmentOnly))
	static bool CaptureViewport(UObject* WorldContextObject, const FString& Filename);
};
