// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockItemInstanceLibrary.h"

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

	for (const FRockItemStack& ItemStack : OwningInventory->ItemData)
	{
		if (ItemStack.RuntimeInstance == InstanceToFind)
		{
			OutItemStack = ItemStack;
			return true;
		}
	}
	return false;
}

bool URockItemInstanceLibrary::FindItemSlotForInstance(const URockItemInstance* InstanceToFind, FRockInventorySlotEntry& OutItemSlot)
{
	if (!InstanceToFind)
	{
		return false;
	}

	// should we have required the inventory to be passed in instead of relying on this?
	const URockInventory* OwningInventory = InstanceToFind->GetOwningInventory();
	if (!OwningInventory)
	{
		return false;
	}

	for (const auto& Slot : OwningInventory->SlotData)
	{
		const FRockItemStack& TempItemStack = OwningInventory->GetItemByHandle(Slot.ItemHandle);
		if (TempItemStack.RuntimeInstance == InstanceToFind)
		{
			OutItemSlot = Slot;
			return true;
		}
	}
	return false;
}
