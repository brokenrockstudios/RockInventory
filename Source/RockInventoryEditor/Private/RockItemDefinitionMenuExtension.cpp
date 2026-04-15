// Copyright Broken Rock Studios LLC. All Rights Reserved.

#define LOCTEXT_NAMESPACE "RockInventoryEditor"
#include "Item/RockItemDefinition.h"
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
					if (URockItemDefinition* Def = Cast<URockItemDefinition>(EditedObject))
					{
						FScopedTransaction Transaction(LOCTEXT("SortFragments", "Sort Fragments"));
						Def->Modify();
						Def->SortFragments();
					}
				}
			}
		}
	}

	// This is 100% used. Even if something says otherwise. 
	[[maybe_unused]]
	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("AssetEditor.RockItemDefinitionEditor.ToolBar");
			FToolMenuSection& Section = Menu->FindOrAddSection("RockInventory");
			Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				InSection.AddMenuEntry(
					"RockItemDefinition_SortFragments",
					LOCTEXT("SortFragments", "Sort Fragments"),
					LOCTEXT("SortFragmentsTooltip", "Sort fragments by sort order priority"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.SortDown"),
					FToolUIAction(FToolMenuExecuteAction::CreateStatic(&ExecuteSortFragments))
				);
			}));
		}));
	});
}

#undef LOCTEXT_NAMESPACE
