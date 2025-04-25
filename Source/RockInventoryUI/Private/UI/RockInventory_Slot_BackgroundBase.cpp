// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "UI/RockInventory_Slot_BackgroundBase.h"

URockInventory_Slot_BackgroundBase::URockInventory_Slot_BackgroundBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set default values for properties
	SlotHandle = FRockInventorySlotHandle();
	Inventory = nullptr;

	// Initialize any other properties here
	// This is where we might want to set up the slot background
	// For example, we might want to set a default texture or color for the background
	//Tags.Add(FName("SlotBackground"));
}

void URockInventory_Slot_BackgroundBase::NativeOnDragEnter(
	const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	// Update the inventory as the mouse is currently over this, and to help with updating drag/drop indicators
}

void URockInventory_Slot_BackgroundBase::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	// Update the inventory as the mouse is currently over this, and to help with updating drag/drop indicators

	// Note: We need both enter and leave, because while sometimes the mouse go from 1 bg slot to another,  it could also go off the inventory all together
}
