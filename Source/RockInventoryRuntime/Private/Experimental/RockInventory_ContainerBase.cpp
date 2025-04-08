// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Experimental/RockInventory_ContainerBase.h"

#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Experimental/RockInventory_Slot_BackgroundBase.h"
#include "Inventory/RockInventory.h"

void URockInventory_ContainerBase::SetInventory(URockInventory* InInventory)
{
	Inventory = InInventory;
}

void URockInventory_ContainerBase::GenerateGrid()
{
	UniformGrid->ClearChildren();
	
	// If we don't have an inventory, do we still want to have support for 'preview grid'
	int tempWidth = Width;
	int tempHeight = Height;
	if (Inventory)
	{
		auto TabInfo = Inventory->GetTabInfo(TabIndex);
		if (TabInfo)
		{
			tempWidth = TabInfo->GetWidth();
			tempHeight = TabInfo->GetHeight();
		}
	}
	// We probably need to do something with the TabIndex?
	
	
	for (int dy = 0; dy < tempHeight; dy++)
	{
		for (int dx = 0; dx < tempWidth; dx++)
		{
			UUserWidget* newWidget = CreateWidget(this, ItemSlotWidgetClass_Empty, FName(*FString::Printf(TEXT("ItemSlot_%d_%d"), dy, dx)));
			if (URockInventory_Slot_BackgroundBase* SlotBackground = Cast<URockInventory_Slot_BackgroundBase>(newWidget))
			{
				SlotBackground->Inventory = Inventory;
				SlotBackground->SlotHandle = FRockInventorySlotHandle(TabIndex, dy, dx);
				
			}
			UUniformGridSlot* GridSlotWidget = UniformGrid->AddChildToUniformGrid(newWidget,  dy, dx);
			if (GridSlotWidget)
			{
				GridSlotWidget->SetHorizontalAlignment(HAlign_Fill);
				GridSlotWidget->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}
}
