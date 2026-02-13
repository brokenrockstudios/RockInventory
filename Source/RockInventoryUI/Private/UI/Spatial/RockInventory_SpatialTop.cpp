// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "UI/Spatial/RockInventory_SpatialTop.h"

#include "Item/RockItemDefinition.h"
#include "UI/ItemDescription/RockInventory_ItemDescription.h"

void URockInventory_SpatialTop::OnItemHovered(const FRockItemStack& ItemStack)
{
	// if (HasHoverItem())
	// {
	// 	return;
	// }
	//
	// //const auto& Manifest = ItemStack->GetItemManifest();
	// // assert that the item stack has definition
	// checkf(ItemStack.IsValid(), TEXT("SetActiveDragItem called with invalid ItemStack"));
	// auto ItemDef = ItemStack.GetDefinition();
	//
	// URockInventory_ItemDescription* DescriptionWidget = GetItemDescription();
	// if (!DescriptionWidget)
	// {
	// 	UE_LOG(LogRockInventoryUI, Error, TEXT("Item Description Widget is not valid."))
	// 	return;
	// }
	// DescriptionWidget->SetVisibility(ESlateVisibility::Collapsed);
	//
	// if (UWorld* World = GetWorld())
	// {
	// 	World->GetTimerManager().ClearTimer(DescriptionTimerHandle);
	//
	// 	FTimerDelegate DescriptionTimerDelegate;
	// 	DescriptionTimerDelegate.BindLambda([this, ItemStack, DescriptionWidget]()
	// 	{
	// 		// This only is based upon the ItemDefinition, and not the InstanceData or Item
	// 		// Fragment->Assimilate
	// 		// ItemDefinitionAssimilate?
	// 		// TODO: I don't like this whole approach. 
	// 		AssimilateInventoryFragments(ItemStack, DescriptionWidget);
	// 		DescriptionWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	// 	});
	// 	World->GetTimerManager().SetTimer(DescriptionTimerHandle, DescriptionTimerDelegate, DescriptionTimerDelay, false);
	// }
}


void URockInventory_SpatialTop::AssimilateInventoryFragments(const FRockItemStack& ItemStack, URockInventory_ItemDescription* DescriptionComposite)
{
	
	// auto ItemDef = ItemStack.GetDefinition();
	//
	// // In original they did GetFragmentByType and only assimilated appropriate 'widget' fragments.
	// // 
	// const TArray<FRockItemFragmentInstance>& InventoryItemFragments = ItemDef->GetAllFragments();
	// for (auto& Fragment : InventoryItemFragments)
	// {
	// 	DescriptionComposite->ApplyFunction([Fragment](URockInventory_CompositeBase* Widget)
	// 	{
	// 		Assimilate(Widget, Fragment);
	// 	});
	// }
	//
	// // BRS TODO:
	// // 2) Synthetic Defaults (only fill in, if not already filled in by real fragments)
	// // BuildSyntheticFragments(...);
	// // for (const auto* Fragment : SyntheticFragments)
	// // {
	// // 	Composite->ApplyFunction([Fragment](UInv_CompositeBase* Widget)
	// // 	{
	// // 		if (Widget->IsCollapsed or not already filled in?)
	// // 		{
	// // 			Fragment->Assimilate(Widget);
	// // 		}
	// // 	});
	// // }
}

void URockInventory_SpatialTop::Assimilate(URockInventory_CompositeBase* widget, const FRockItemFragmentInstance& fragment)
{
	// const FRockItemFragment_SetStats* Fragment = fragment.GetFragmentData<FRockItemFragment_SetStats>();
	// auto Frag = Cast<FRockItemFragment_SetStats>(Fragment);
}

void URockInventory_SpatialTop::OnItemUnhovered()
{
	// GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	// GetOwningPlayer()->GetWorldTimerManager().ClearTimer(DescriptionTimerHandle);
}

bool URockInventory_SpatialTop::HasHoverItem() const
{
	return Super::HasHoverItem();
}

URockInventory_HoverItem* URockInventory_SpatialTop::GetHoverItem() const
{
	// Get it from one of the children?
	//if (!ActiveGrid.IsValid()) return nullptr;
	//return ActiveGrid->GetHoverItem();
	return nullptr;
}

float URockInventory_SpatialTop::GetTileSize() const
{
	// Grab one of the grids to get the tile size from?
	// But if we don't have any default to?
	return 64.f;
}


URockInventory_ItemDescription* URockInventory_SpatialTop::GetItemDescription()
{
	return nullptr;
	// if (!IsValid(ItemDescription))
	// {
	// 	ItemDescription = CreateWidget<URockInventory_ItemDescription>(GetOwningPlayer(), ItemDescriptionClass);
	// 	CanvasPanel->AddChild(ItemDescription);
	// }
	// return ItemDescription;
}
