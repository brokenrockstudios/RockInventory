// Copyright Broken Rock Studios LLC. All Rights Reserved.

#define LOCTEXT_NAMESPACE "RockInventoryEditor"
#include "Item/RockItemDefinition.h"
#include "Misc/DataValidation.h"
#include "Misc/UObjectToken.h"
#include "Toolkits/AssetEditorToolkitMenuContext.h"

namespace MenuExtension_RockItemDefinition
{
	static void ExecuteSortFragments(const FToolMenuContext& InContext)
	{
		if (const UAssetEditorToolkitMenuContext* AssetContext = InContext.FindContext<UAssetEditorToolkitMenuContext>())
		{
			if (TSharedPtr<FAssetEditorToolkit> Toolkit = AssetContext->Toolkit.Pin())
			{
				for (UObject* EditedObject : *Toolkit->GetObjectsCurrentlyBeingEdited())
				{
					if (URockItemDefinition* ItemDef = Cast<URockItemDefinition>(EditedObject))
					{
						FScopedTransaction Transaction(LOCTEXT("SortFragments", "Sort Fragments"));
						ItemDef->Modify();
						ItemDef->SortFragments();
					}
				}
			}
		}
	}

	static void ExecuteValidateItem(const FToolMenuContext& InContext)
	{
		const UAssetEditorToolkitMenuContext* AssetContext = InContext.FindContext<UAssetEditorToolkitMenuContext>();
		if (!AssetContext) { return; }
		TSharedPtr<FAssetEditorToolkit> Toolkit = AssetContext->Toolkit.Pin();
		if (!Toolkit) return;

		FMessageLog MessageLog("AssetCheck");
		MessageLog.NewPage(LOCTEXT("ValidateFragmentsPage", "Validate Fragments"));

		int32 NumValid = 0;
		int32 NumInvalid = 0;

		for (UObject* EditedObject : *Toolkit->GetObjectsCurrentlyBeingEdited())
		{
			URockItemDefinition* ItemDef = Cast<URockItemDefinition>(EditedObject);
			if (!ItemDef) { continue; }

			FDataValidationContext ValidationContext;
			EDataValidationResult Result = ItemDef->IsDataValid(ValidationContext);

			// Surface results however you like - Message Log is the idiomatic path
			TArray<FText> Warnings;
			TArray<FText> Errors;
			ValidationContext.SplitIssues(Warnings, Errors);

			for (const FText& Warning : Warnings)
			{
				MessageLog.Warning()
				          ->AddToken(FUObjectToken::Create(ItemDef))
				          ->AddToken(FTextToken::Create(Warning));
			}
			for (const FText& Error : Errors)
			{
				MessageLog.Error()
				          ->AddToken(FUObjectToken::Create(ItemDef))
				          ->AddToken(FTextToken::Create(Error));
			}

			if (Result == EDataValidationResult::Invalid) { ++NumInvalid; }
			else { ++NumValid; }
		}

		MessageLog.Info(
			FText::Format(
				LOCTEXT("ValidateFragmentsSummary", "Validated {0} asset(s): {1} valid, {2} invalid"),
				FText::AsNumber(NumValid + NumInvalid), FText::AsNumber(NumValid), FText::AsNumber(NumInvalid)));

		// Opens the Message Log tab automatically if there are issues
		MessageLog.Open(EMessageSeverity::Warning, /*bOpenEvenIfEmpty*/ false);
	}


	// This is 100% used. Even if something says otherwise. 
	[[maybe_unused]]
	static FDelayedAutoRegisterHelper DelayedAutoRegister(
		EDelayedRegisterRunPhase::EndOfEngineInit, []
		{
			UToolMenus::RegisterStartupCallback(
				FSimpleMulticastDelegate::FDelegate::CreateLambda(
					[]()
					{
						FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
						UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("AssetEditor.RockItemDefinitionEditor.ToolBar");
						FToolMenuSection& Section = Menu->FindOrAddSection("RockInventory");
						Section.AddDynamicEntry(
							NAME_None, FNewToolMenuSectionDelegate::CreateLambda(
								[](FToolMenuSection& InSection)
								{
									InSection.AddMenuEntry(
										"RockItemDefinition_SortFragments",
										LOCTEXT("SortFragments", "Sort Fragments"),
										LOCTEXT("SortFragmentsTooltip", "Sort fragments by sort order priority"),
										FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.SortDown"),
										FToolUIAction(FToolMenuExecuteAction::CreateStatic(&ExecuteSortFragments))
									);
								}));

						Section.AddDynamicEntry(
							NAME_None, FNewToolMenuSectionDelegate::CreateLambda(
								[](FToolMenuSection& InSection)
								{
									InSection.AddMenuEntry(
										"RockItemDefinition_ValidateItem",
										LOCTEXT("ValidateItem", "Validate"),
										LOCTEXT("ValidateItemTooltip", "Validate"),
										FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Validate"),
										FToolUIAction(FToolMenuExecuteAction::CreateStatic(&ExecuteValidateItem))
									);
								}));
					}));
		});
}

#undef LOCTEXT_NAMESPACE
