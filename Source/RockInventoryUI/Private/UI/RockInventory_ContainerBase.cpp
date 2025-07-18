// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "UI/RockInventory_ContainerBase.h"

#include "RockInventoryUILogging.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "UI/RockInventory_Slot_BackgroundBase.h"
#include "UI/RockInventory_Slot_ItemBase.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockItemStackLibrary.h"

void URockInventory_ContainerBase::SetInventory(URockInventory* InInventory, const FName& InSectionName)
{
	if (InInventory)
	{
		Inventory = InInventory;
		TabInfo = Inventory->GetSectionInfo(InSectionName);
	}

	GenerateGrid();
	GenerateItems();
}

void URockInventory_ContainerBase::GenerateGrid()
{
	GridPanel->ClearChildren();

	int32 SectionIndex = INDEX_NONE;
	if (Inventory)
	{
		SectionIndex = Inventory->GetSectionIndexById(TabInfo.SectionName);
	}

	for (int32 slotIndex = 0; slotIndex < TabInfo.GetNumSlots(); ++slotIndex)
	{
		const int32 Column = slotIndex % TabInfo.Width;
		const int32 Row = slotIndex / TabInfo.Width;
		const int32 AbsoluteIndex = TabInfo.FirstSlotIndex + slotIndex;

		UUserWidget* newWidget = CreateWidget(this, ItemSlotWidgetClass_Empty, FName(*FString::Printf(TEXT("ItemSlot_%d"), slotIndex)));
		if (URockInventory_Slot_BackgroundBase* SlotBackground = Cast<URockInventory_Slot_BackgroundBase>(newWidget))
		{
			SlotBackground->Inventory = Inventory;
			SlotBackground->SlotHandle = FRockInventorySlotHandle(SectionIndex, AbsoluteIndex);
		}

		UGridSlot* GridSlotWidget = GridPanel->AddChildToGrid(newWidget, Row, Column);
		if (GridSlotWidget)
		{
			GridSlotWidget->SetHorizontalAlignment(HAlign_Fill);
			GridSlotWidget->SetVerticalAlignment(VAlign_Fill);
			GridSlotWidget->SetColumnSpan(1);
			GridSlotWidget->SetRowSpan(1);
			// Move to Background
			GridSlotWidget->SetLayer(-100);
		}
	}
}

void URockInventory_ContainerBase::ClearItemsFromGrid()
{
	for (int32 i = GridPanel->GetChildrenCount() - 1; i >= 0; --i)
	{
		UWidget* Child = GridPanel->GetChildAt(i);
		if (!Child || Cast<URockInventory_Slot_BackgroundBase>(Child))
		{
			// Skip background widgets. Remove everything else
			continue;
		}
		if (URockInventory_Slot_ItemBase* ItemSlot = Cast<URockInventory_Slot_ItemBase>(Child))
		{
			// Explicitly clear the inventory reference to avoid dangling pointers
			ItemSlot->Inventory = nullptr;
			ItemSlot->FallbackIcon = nullptr;
		}
		GridPanel->RemoveChildAt(i);
	}
}

void URockInventory_ContainerBase::GenerateItems()
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventoryUI, Warning, TEXT( "GenerateItems: Inventory is null"));
		return;
	}

	// TODO: Optimize this
	// Instead of 'redrawing' everything for any 'single change'
	// We should probably try to update the existing 'items' instead?
	ClearItemsFromGrid();

	ERockItemSizePolicy SizePolicy = ERockItemSizePolicy::RespectSize;
	if (Inventory)
	{
		SizePolicy = Inventory->GetSectionInfo(TabInfo.SectionName).SlotSizePolicy;
	}

	//////////////////////////////////////////////////////////////////////////
	/// Calculate uniform grid cell size to calculate proper positions of canvas slots
	// float CellWidth = 0.0f;
	// float CellHeight = 0.0f;
	//
	// // If we couldn't get cell size from existing children, use default values
	// if (CellWidth <= 0.0f || CellHeight <= 0.0f)
	// {
	// 	CellWidth = 64.0f; // Default cell width, adjust as needed
	// 	CellHeight = 64.0f; // Default cell height, adjust as needed
	// }
	//////////////////////////////////////////////////////////////////////////

	for (int32 slotIndex = 0; slotIndex < TabInfo.GetNumSlots(); ++slotIndex)
	{
		const int32 AbsoluteIndex = TabInfo.FirstSlotIndex + slotIndex;
		const FRockInventorySlotEntry& SlotEntry = Inventory->GetSlotByAbsoluteIndex(AbsoluteIndex);

		if (!SlotEntry.ItemHandle.IsValid())
		{
			continue;
		}
		const FRockItemStack& ItemStack = Inventory->GetItemByHandle(SlotEntry.ItemHandle);
		if (!ItemStack.IsValid())
		{
			continue;
		}
		const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(ItemStack);
		URockInventory_Slot_ItemBase* WidgetItem = CreateWidget<URockInventory_Slot_ItemBase>(this, ItemSlotWidgetClass,
			FName(*FString::Printf(TEXT("Item_%d"), slotIndex)));
		if (WidgetItem)
		{
			const int32 Column = slotIndex % TabInfo.Width;
			const int32 Row = slotIndex / TabInfo.Width;

			// Note: For reasons this needs to happen before SetupBindings >:( 
			// Add to grid with proper sizing
			UGridSlot* GridSlot = GridPanel->AddChildToGrid(WidgetItem, Row, Column);

			// Setup Widget properties
			WidgetItem->Inventory = Inventory;
			WidgetItem->SlotHandle = SlotEntry.SlotHandle;
			WidgetItem->ItemHandle = SlotEntry.ItemHandle;
			WidgetItem->SetIconData(ItemStack.GetDefinition()->IconData);
			WidgetItem->SetupBindings();

			if (GridSlot)
			{
				// Setting this more than 0 breaks things. Find another way to do 'padding' or internal spacing
				GridSlot->SetPadding(FMargin(0));
				// If this section is ignore size, we should use 1

				if (SizePolicy == ERockItemSizePolicy::IgnoreSize)
				{
					GridSlot->SetColumnSpan(1);
					GridSlot->SetRowSpan(1);
				}
				else
				{
					// If this section is respect size, we should use the item size
					GridSlot->SetColumnSpan(ItemSize.X);
					GridSlot->SetRowSpan(ItemSize.Y);
				}

				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}
}
