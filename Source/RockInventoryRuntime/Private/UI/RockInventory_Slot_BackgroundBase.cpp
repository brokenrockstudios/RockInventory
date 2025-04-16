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
