// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Experimental/RockInventory_ContainerBase.h"

#include "RockInventoryLogging.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Experimental/RockInventory_Slot_BackgroundBase.h"
#include "Experimental/RockInventory_Slot_ItemBase.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"

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
			// FRockInventorySlotHandle(TabIndex, AbsoluteIndex);
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

		const URockInventory_Slot_ItemBase* ItemWidget = Cast<URockInventory_Slot_ItemBase>(Child);
		if (ItemWidget)
		{
			// Remove the item widget from the grid
			GridPanel->RemoveChildAt(i);
		}
	}
}

void URockInventory_ContainerBase::GenerateItems()
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("URockInventory_ContainerBase::GenerateItems - Inventory is null"));
		return;
	}

	ClearItemsFromGrid();

	int32 SectionIndex = INDEX_NONE;
	if (Inventory)
	{
		SectionIndex = Inventory->GetSectionIndexById(TabInfo.SectionName);
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
		const int32 Column = slotIndex % TabInfo.Width;
		const int32 Row = slotIndex / TabInfo.Width;
		const int32 AbsoluteIndex = TabInfo.FirstSlotIndex + slotIndex;
		const FRockInventorySlotEntry& TempSlot = Inventory->GetSlotByAbsoluteIndex(AbsoluteIndex);

		const FRockItemStack& ItemStack = Inventory->GetItemByHandle(TempSlot.ItemHandle);
		if (ItemStack.IsValid())
		{
			const int32 ColumnSpan = ItemStack.Definition->SlotDimensions.X;
			const int32 RowSpan = ItemStack.Definition->SlotDimensions.Y;

			UUserWidget* newWidget = CreateWidget(this, ItemSlotWidgetClass, FName(*FString::Printf(TEXT("Item_%d"), slotIndex)));

			if (URockInventory_Slot_ItemBase* WidgetItem = Cast<URockInventory_Slot_ItemBase>(newWidget))
			{
				WidgetItem->Inventory = Inventory;
				WidgetItem->SlotHandle = FRockInventorySlotHandle(SectionIndex, AbsoluteIndex);
				WidgetItem->SetItemIcon(ItemStack.Definition->Icon);
				// Initialize the item count display
				WidgetItem->UpdateItemCount();
				// Additional Setup
				// SlotBackground->ItemStack = TempSlot.Item;
			}

			UGridSlot* GridSlot = GridPanel->AddChildToGrid(newWidget, Row, Column);
			if (GridSlot)
			{
				// Setting this more than 0 breaks things. Find another way to do 'padding' or internal spacing
				GridSlot->SetPadding(FMargin(0));
				GridSlot->SetColumnSpan(ColumnSpan);
				GridSlot->SetRowSpan(RowSpan);
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}
}
