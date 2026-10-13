#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "UI/MothHUDWidget.h"
#include "UI/MothUIEditorLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDesignerEdits, "MothEffect.UI.DesignerBindingsPreservePresentationAndReparenting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDesignerEdits::RunTest(const FString&)
{
	auto* Saved = LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/MothEffect/UI/WBP_MothHUD.WBP_MothHUD"));
	if (!TestNotNull(TEXT("Saved Designer Blueprint"),Saved) || !TestNotNull(TEXT("Persisted Designer root"),Saved->WidgetTree->RootWidget.Get())) return false;
	// Compile a disposable Blueprint with an edited copy of the saved tree; never change the delivered asset.
	const FName Name = MakeUniqueObjectName(GetTransientPackage(),UWidgetBlueprint::StaticClass(),TEXT("T18DesignerProbe"));
	auto* Probe = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UMothHUDWidget::StaticClass(),GetTransientPackage(),Name,
		BPTYPE_Normal,UWidgetBlueprint::StaticClass(),UWidgetBlueprintGeneratedClass::StaticClass()));
	if (!TestNotNull(TEXT("Disposable Blueprint"),Probe)) return false;
	Probe->WidgetTree = DuplicateObject<UWidgetTree>(Saved->WidgetTree,Probe,TEXT("EditedDesignerTree"));
	auto* Health = Cast<UTextBlock>(Probe->WidgetTree->FindWidget(TEXT("HealthText")));
	auto* VitalSlot = Cast<UCanvasPanelSlot>(Probe->WidgetTree->FindWidget(TEXT("VitalPanel"))->Slot);
	auto* Instruction = Probe->WidgetTree->FindWidget(TEXT("TutorialText"));
	if (!Health || !VitalSlot || !Instruction) return false;
	FSlateFontInfo Font = Health->GetFont(); Font.Size = 41; Health->SetFont(Font);
	const FLinearColor Color(.3f,.6f,.9f); Health->SetColorAndOpacity(Color);
	VitalSlot->SetPosition(FVector2D(80,-60));
	Instruction->RemoveFromParent();
	auto* Wrapper = Probe->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("UserAddedWrapper"));
	Wrapper->SetContent(Instruction);
	Cast<UVerticalBox>(Probe->WidgetTree->FindWidget(TEXT("TeachingContent")))->AddChildToVerticalBox(Wrapper);
	const FString Report = UMothUIEditorLibrary::PrepareDesignerLayout(Probe);
	if (!TestFalse(TEXT("Edited tree compiles with the required controls"),Report.Contains(TEXT("\"error\"")))) return false;
	TestTrue(TEXT("Existing Designer tree is preserved"),Probe->WidgetTree->FindWidget(TEXT("UserAddedWrapper"))==Wrapper);
	auto* UI = NewObject<UMothHUDWidget>(GetTransientPackage(),Probe->GeneratedClass);
	UI->Initialize();
	int32 Count=0;
	for (TFieldIterator<FObjectProperty> It(UMothHUDWidget::StaticClass()); It; ++It)
	{
		if (!It->HasMetaData(TEXT("BindWidget"))) continue;
		TestTrue(*FString::Printf(TEXT("Generated binding %s resolves before native construction"),*It->GetName()),
			It->GetObjectPropertyValue_InContainer(UI)==UI->GetWidgetFromName(It->GetFName()));
		++Count;
	}
	TestTrue(TEXT("Generated required bindings are present"),Count>0);
	UI->TakeWidget(); UI->RefreshState(); UI->RefreshState();
	auto* RuntimeHealth=Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("HealthText")));
	TestEqual(TEXT("State refresh preserves Designer font"),RuntimeHealth->GetFont().Size,41.f);
	TestEqual(TEXT("State refresh preserves Designer colour"),RuntimeHealth->GetColorAndOpacity().GetSpecifiedColor(),Color);
	TestEqual(TEXT("State refresh preserves Designer canvas position"),Cast<UCanvasPanelSlot>(UI->GetWidgetFromName(TEXT("VitalPanel"))->Slot)->GetPosition(),FVector2D(80,-60));
	TestTrue(TEXT("State refresh tolerates a changed instruction hierarchy"),UI->GetWidgetFromName(TEXT("TutorialText"))->GetParent()==UI->GetWidgetFromName(TEXT("UserAddedWrapper")));
	UI->RemoveFromParent(); UI->ReleaseSlateResources(true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMothDesignerPreview, "MothEffect.UI.DesignerPreviewDoesNotBindGameplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMothDesignerPreview::RunTest(const FString&)
{
	UClass* Class=LoadClass<UMothHUDWidget>(nullptr,TEXT("/Game/MothEffect/UI/WBP_MothHUD.WBP_MothHUD_C"));
	if (!TestNotNull(TEXT("Saved Designer class"),Class)) return false;
	auto* UI=NewObject<UMothHUDWidget>(GetTransientPackage(),Class);
	UI->SetDesignerFlags(EWidgetDesignFlags::Designing | EWidgetDesignFlags::ExecutePreConstruct);
	*FindFProperty<FEnumProperty>(UMothHUDWidget::StaticClass(),TEXT("PreviewPage"))->ContainerPtrToValuePtr<EMothUIPage>(UI)=EMothUIPage::Tutorial;
	*FindFProperty<FEnumProperty>(UMothHUDWidget::StaticClass(),TEXT("PreviewTutorialStep"))->ContainerPtrToValuePtr<EMothTutorialStep>(UI)=EMothTutorialStep::AirborneEmitter;
	UI->Initialize(); UI->TakeWidget(); UI->RefreshState();
	TestEqual(TEXT("Worldless preview retains the selected teaching page"),UI->GetHUDState().Page,EMothUIPage::Tutorial);
	TestEqual(TEXT("Preview getter data survives refresh without a player"),UI->GetHUDState().Health,100.f);
	TestFalse(TEXT("Preview reads selected teaching copy"),Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("TutorialText")))->GetText().IsEmpty());
	TestEqual(TEXT("Preview explicitly shows the movable teaching panel"),UI->GetWidgetFromName(TEXT("TeachingPanel"))->GetVisibility(),ESlateVisibility::SelfHitTestInvisible);
	UI->ReleaseSlateResources(true);
	return true;
}
#endif
