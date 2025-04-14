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
#include "Library/RockItemStackLibrary.h"

void URockInventory_ContainerBase::SetInventory(URockInventory* InInventory, int32 InTabIndex)
{
	if (InInventory)
	{
		Inventory = InInventory;
		TabIndex = InTabIndex;
		TabInfo = *Inventory->GetTabInfo(TabIndex);
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("URockInventory_ContainerBase::SetInventory - Invalid inventory, falling back to preview"));
		TabInfo = FRockInventoryTabInfo();
		TabInfo.TabID = FName("Preview");
		TabInfo.Width = Width;
		TabInfo.Height = Height;
		TabInfo.NumSlots = Width * Height;
	}
	// if invalid, fall back on preview sizes?


	GenerateGrid();
	GenerateItems();
}


void URockInventory_ContainerBase::GenerateGrid()
{
	GridPanel->ClearChildren();


	for (int32 slotIndex = 0; slotIndex < TabInfo.NumSlots; ++slotIndex)
	{
		const int32 X = slotIndex % TabInfo.Width;
		const int32 Y = slotIndex / TabInfo.Width;

		UUserWidget* newWidget = CreateWidget(this, ItemSlotWidgetClass_Empty, FName(*FString::Printf(TEXT("ItemSlot_%d"), slotIndex)));
		if (URockInventory_Slot_BackgroundBase* SlotBackground = Cast<URockInventory_Slot_BackgroundBase>(newWidget))
		{
			SlotBackground->Inventory = Inventory;
			SlotBackground->SlotHandle = FRockInventorySlotHandle(TabIndex, X, Y);
		}
		UGridSlot* GridSlotWidget = GridPanel->AddChildToGrid(newWidget, Y, X);
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


	for (int32 slotIndex = TabInfo.FirstSlotIndex; slotIndex < TabInfo.FirstSlotIndex + TabInfo.NumSlots; ++slotIndex)
	{
		const FRockInventorySlotEntry& TempSlot = Inventory->GetSlotByAbsoluteIndex(slotIndex);
		const FRockItemStack& ItemStack = Inventory->GetItemByHandle(TempSlot.ItemHandle);
		const URockItemDefinition* ItemDefinition = URockItemStackLibrary::GetItemDefinition(ItemStack.ItemId);
		int32 ColumnSpan = 1;
		int32 RowSpan = 1;
		TSoftObjectPtr<UTexture2D> ItemIcon = nullptr;
		if (ItemDefinition)
		{
			ColumnSpan = ItemDefinition->SlotDimensions.X;
			RowSpan = ItemDefinition->SlotDimensions.Y;
			ItemIcon = ItemDefinition->Icon;
		}
		else
		{
			UE_LOG(LogRockInventory, Warning, TEXT("GenerateItems - Item definition not found for item ID: %s"), *ItemStack.ItemId.ToString());
			continue;
		}

		if (ItemStack.IsValid())
		{
			UUserWidget* newWidget = CreateWidget(this, ItemSlotWidgetClass, FName(*FString::Printf(TEXT("Item_%d"), slotIndex)));

			if (URockInventory_Slot_ItemBase* WidgetItem = Cast<URockInventory_Slot_ItemBase>(newWidget))
			{
				WidgetItem->Inventory = Inventory;
				WidgetItem->SlotHandle = TempSlot.SlotHandle;
				WidgetItem->SetItemIcon(ItemIcon);
				// Initialize the item count display
				WidgetItem->UpdateItemCount();
				// Additional Setup
				// SlotBackground->ItemStack = TempSlot.Item;
			}

			UGridSlot* GridSlot = GridPanel->AddChildToGrid(newWidget, TempSlot.SlotHandle.GetY(), TempSlot.SlotHandle.GetX());
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
