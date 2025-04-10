// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Library/RockItemInstanceLibrary.h"

#include "Inventory/RockContainerExperiment.h"
#include "Item/RockItemInstance.h"

bool URockItemInstanceLibrary::FindItemStackForInstance(const URockItemInstance* InstanceToFind, FRockItemStack& OutItemStack)
{
	if (!InstanceToFind)
	{
		return false;
	}

	const URockInventory* OwningInventory = InstanceToFind->GetOwningInventory();
	if (!OwningInventory)
	{
		return false;
	}

	for (const FRockInventorySlot& Slot : OwningInventory->InventoryData)
	{
		if (Slot.Item.RuntimeInstance == InstanceToFind)
		{
			OutItemStack = Slot.Item;
			return true;
		}
	}
	return false;
}

bool URockItemInstanceLibrary::FindItemSlotForInstance(const URockItemInstance* InstanceToFind, FRockInventorySlot& OutItemSlot)
{
	if (!InstanceToFind)
	{
		return false;
	}

	const URockInventory* OwningInventory = InstanceToFind->GetOwningInventory();
	if (!OwningInventory)
	{
		return false;
	}

	for (const FRockInventorySlot& Slot : OwningInventory->InventoryData)
	{
		if (Slot.Item.RuntimeInstance == InstanceToFind)
		{
			OutItemSlot = Slot;
			return true;
		}
	}
	return false;
}
