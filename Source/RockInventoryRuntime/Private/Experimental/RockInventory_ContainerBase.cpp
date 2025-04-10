// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Experimental/RockInventory_ContainerBase.h"

#include "RockInventoryLogging.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Experimental/RockInventory_Slot_BackgroundBase.h"
#include "Experimental/RockInventory_Slot_ItemBase.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"

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

	for (int32 slotIndex = TabInfo.FirstSlotIndex; slotIndex < TabInfo.FirstSlotIndex + TabInfo.NumSlots; ++slotIndex)
	{
		const FRockInventorySlot& TempSlot = Inventory->InventoryData[slotIndex];

		UUserWidget* newWidget = CreateWidget(this, ItemSlotWidgetClass_Empty,
			FName(*FString::Printf(TEXT("ItemSlot_%d_%d"), TempSlot.SlotHandle.Y, TempSlot.SlotHandle.X)));
		if (URockInventory_Slot_BackgroundBase* SlotBackground = Cast<URockInventory_Slot_BackgroundBase>(newWidget))
		{
			SlotBackground->Inventory = Inventory;
			SlotBackground->SlotHandle = TempSlot.SlotHandle;
		}
		UGridSlot* GridSlotWidget = GridPanel->AddChildToGrid(newWidget, TempSlot.SlotHandle.Y, TempSlot.SlotHandle.X);
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
		const FRockInventorySlot& TempSlot = Inventory->InventoryData[slotIndex];
		if (TempSlot.Item.IsValid())
		{
			UUserWidget* newWidget = CreateWidget(this, ItemSlotWidgetClass, FName(*FString::Printf(TEXT("Item_%d"), slotIndex)));

			if (URockInventory_Slot_ItemBase* WidgetItem = Cast<URockInventory_Slot_ItemBase>(newWidget))
			{
				WidgetItem->Inventory = Inventory;
				WidgetItem->SlotHandle = TempSlot.SlotHandle;
				
				WidgetItem->ItemIcon->SetBrushFromTexture(TempSlot.Item.GetDefinition()->Icon.LoadSynchronous());
				
				// Additional Setup
				// SlotBackground->ItemStack = TempSlot.Item;
			}

			UGridSlot* GridSlot = GridPanel->AddChildToGrid(newWidget, TempSlot.SlotHandle.Y, TempSlot.SlotHandle.X);
			if (GridSlot)
			{
				GridSlot->SetPadding(FMargin(4));

				int32 ColumnSpan = TempSlot.Item.GetDefinition()->SlotDimensions.X;
				int32 RowSpan = TempSlot.Item.GetDefinition()->SlotDimensions.Y;
				GridSlot->SetColumnSpan(ColumnSpan);
				GridSlot->SetRowSpan(RowSpan);

				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}
}
