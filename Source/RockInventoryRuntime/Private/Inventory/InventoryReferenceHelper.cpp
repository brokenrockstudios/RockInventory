// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Inventory/InventoryReferenceHelper.h"

#include "Inventory/RockInventory.h"

FRockItemStack FRockItemReference::GetCopyOfItem() const
{
	if (Inventory == nullptr)
	{
		return FRockItemStack::Invalid();
	}
	
	return Inventory->GetItemByHandle(ItemHandle);
}

FRockInventorySlotEntry FRockSlotReference::GetSlotEntry() const
{
	if (Inventory == nullptr)
	{
		return FRockInventorySlotEntry::Invalid();
	}

	return Inventory->GetSlotByHandle(SlotHandle);
}
