#include "UI/MothUIEditorLibrary.h"

#if WITH_EDITOR
#include "UI/MothHUDWidget.h"
#include "MothHUDLayout.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "UObject/UnrealType.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#endif

FString UMothUIEditorLibrary::PrepareDesignerLayout(UObject* WidgetAsset)
{
#if WITH_EDITOR
	auto* Blueprint = Cast<UWidgetBlueprint>(WidgetAsset);
	if (!Blueprint || !Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(UMothHUDWidget::StaticClass())
		|| !Blueprint->WidgetTree) return TEXT("{\"error\":\"Not a MothHUD Widget Blueprint\"}");
	const bool bCreated = Blueprint->WidgetTree->RootWidget == nullptr;
	if (bCreated)
	{
		Blueprint->Modify(); Blueprint->WidgetTree->Modify();
		MothHUDLayout::Build(Blueprint->WidgetTree);
	}
	int32 Bindings = 0;
	for (TFieldIterator<FObjectProperty> It(UMothHUDWidget::StaticClass()); It; ++It)
	{
		const FObjectProperty* Property = *It;
		if (!Property->HasMetaData(TEXT("BindWidget"))) continue;
		UWidget* Widget = Blueprint->WidgetTree->FindWidget(Property->GetFName());
		if (!Widget || !Widget->IsA(Property->PropertyClass))
			return FString::Printf(TEXT("{\"error\":\"Missing or wrong type: %s\"}"), *Property->GetName());
		if (bCreated) Widget->bIsVariable = true;
		++Bindings;
	}
	if (bCreated) FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
	if (Results.NumErrors > 0 || !Blueprint->GeneratedClass)
		return TEXT("{\"error\":\"Widget Blueprint compilation failed; inspect compiler log\"}");
	TArray<UWidget*> Widgets; Blueprint->WidgetTree->GetAllWidgets(Widgets);
	auto Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("created_layout"), bCreated);
	Report->SetNumberField(TEXT("widget_count"), Widgets.Num());
	Report->SetNumberField(TEXT("required_bindings"), Bindings);
	Report->SetNumberField(TEXT("compiler_errors"), Results.NumErrors);
	Report->SetNumberField(TEXT("compiler_warnings"), Results.NumWarnings);
	Report->SetStringField(TEXT("class"), Blueprint->GeneratedClass->GetPathName());
	FString Output; FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Output));
	return Output;
#else
	return TEXT("{\"error\":\"Designer scaffold creation requires an Editor build\"}");
#endif
}
