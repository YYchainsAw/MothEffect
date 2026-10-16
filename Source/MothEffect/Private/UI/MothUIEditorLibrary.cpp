#include "UI/MothUIEditorLibrary.h"

#if WITH_EDITOR
#include "UI/MothHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "UObject/UnrealType.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#endif

FString UMothUIEditorLibrary::PrepareDesignerLayout(UObject* WidgetAsset)
{
#if WITH_EDITOR
	auto* Blueprint = Cast<UWidgetBlueprint>(WidgetAsset);
	if (!Blueprint || !Blueprint->ParentClass
		|| !Blueprint->WidgetTree) return TEXT("{\"error\":\"Not a MothHUD Widget Blueprint\"}");
	if (!Blueprint->WidgetTree->RootWidget) return TEXT("{\"error\":\"Empty Designer layout\"}");
	int32 Bindings = 0;
	for (TFieldIterator<FObjectProperty> It(Blueprint->ParentClass); It; ++It)
	{
		const FObjectProperty* Property = *It;
		if (!Property->HasMetaData(TEXT("BindWidget"))) continue;
		UWidget* Widget = Blueprint->WidgetTree->FindWidget(Property->GetFName());
		if (!Widget || !Widget->IsA(Property->PropertyClass))
			return FString::Printf(TEXT("{\"error\":\"Missing or wrong type: %s\"}"), *Property->GetName());
		Widget->bIsVariable = true;
		++Bindings;
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
	if (Results.NumErrors > 0 || !Blueprint->GeneratedClass)
		return TEXT("{\"error\":\"Widget Blueprint compilation failed; inspect compiler log\"}");
	TArray<UWidget*> Widgets; Blueprint->WidgetTree->GetAllWidgets(Widgets);
	auto Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("created_layout"), false);
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

UWidgetTree* UMothUIEditorLibrary::GetDesignerTree(UObject* WidgetAsset)
{
#if WITH_EDITOR
	if (auto* BP=Cast<UWidgetBlueprint>(WidgetAsset)) return BP->WidgetTree;
#endif
	return nullptr;
}

UWidget* UMothUIEditorLibrary::NewDesignerWidget(UObject* WidgetAsset, TSubclassOf<UWidget> WidgetClass, FName Name)
{
#if WITH_EDITOR
	if (auto* Tree=GetDesignerTree(WidgetAsset); Tree && WidgetClass && !Tree->FindWidget(Name))
	{
		auto* Widget=Tree->ConstructWidget<UWidget>(WidgetClass,Name);
		if (Widget)
		{
			Widget->bIsVariable=true;
			if (auto* BP=Cast<UWidgetBlueprint>(WidgetAsset); BP && !BP->WidgetVariableNameToGuidMap.Contains(Name))
				BP->OnVariableAdded(Name);
		}
		return Widget;
	}
#endif
	return nullptr;
}

FString UMothUIEditorLibrary::ResetDesignerLayout(UObject* WidgetAsset, const TArray<FName>& PreserveWidgets)
{
#if WITH_EDITOR
	auto* Tree=GetDesignerTree(WidgetAsset);
	if (!Tree) return TEXT("{\"error\":\"Missing Designer tree\"}");
	TArray<UWidget*> All; Tree->GetAllWidgets(All);
	TSet<UWidget*> Keep;
	for (const auto Name:PreserveWidgets)
	{
		auto* Widget=Tree->FindWidget(Name);
		if (!Widget) return FString::Printf(TEXT("{\"error\":\"Missing preserved widget %s\"}"),*Name.ToString());
		Keep.Add(Widget);
		TArray<UWidget*> Children; Tree->GetChildWidgets(Widget,Children);
		for (auto* Child:Children) Keep.Add(Child);
	}
	Tree->Modify(); WidgetAsset->Modify();
	for (const auto Name:PreserveWidgets) Tree->FindWidget(Name)->RemoveFromParent();
	Tree->RootWidget=nullptr;
	int32 Removed=0;
	for (auto* Widget:All) if (!Keep.Contains(Widget))
	{
		if (auto* BP=Cast<UWidgetBlueprint>(WidgetAsset); BP && BP->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			BP->OnVariableRemoved(Widget->GetFName());
		Widget->RemoveFromParent();
		Widget->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
		++Removed;
	}
	return FString::Printf(TEXT("{\"removed_widgets\":%d,\"preserved_widgets\":%d}"),Removed,Keep.Num());
#else
	return TEXT("{\"error\":\"Requires Editor\"}");
#endif
}

bool UMothUIEditorLibrary::SetDesignerRoot(UObject* WidgetAsset, UWidget* Root)
{
#if WITH_EDITOR
	if (auto* Tree=GetDesignerTree(WidgetAsset); Tree && Root && Root->GetOuter()==Tree)
	{
		Tree->RootWidget=Root;
		return true;
	}
#endif
	return false;
}

TArray<UWidget*> UMothUIEditorLibrary::ListDesignerWidgets(UObject* WidgetAsset)
{
	TArray<UWidget*> Widgets;
#if WITH_EDITOR
	if (auto* Tree=GetDesignerTree(WidgetAsset)) Tree->GetAllWidgets(Widgets);
#endif
	return Widgets;
}

UWidget* UMothUIEditorLibrary::GetNamedWidget(UUserWidget* Widget, FName Name)
{
	return Widget?Widget->GetWidgetFromName(Name):nullptr;
}

bool UMothUIEditorLibrary::CaptureViewport(UObject* WorldContextObject, const FString& Filename)
{
#if WITH_EDITOR
	UWorld* World=GEngine?GEngine->GetWorldFromContextObject(WorldContextObject,EGetWorldErrorMode::ReturnNull):nullptr;
	UGameViewportClient* Client=World?World->GetGameViewport():nullptr;
	if (!Client || !Filename.EndsWith(TEXT(".png")) || !FSlateApplication::IsInitialized()) return false;
	const auto Viewport=Client->GetGameViewportWidget();
	if (!Viewport.IsValid()) return false;
	TArray<FColor> Pixels; FIntVector Size;
	if (!FSlateApplication::Get().TakeScreenshot(Viewport.ToSharedRef(),Pixels,Size)) return false;
	for (auto& Pixel:Pixels) Pixel.A=255;
	TArray64<uint8> Png;
	FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Png);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
	return FFileHelper::SaveArrayToFile(Png,*Filename);
#else
	return false;
#endif
}
