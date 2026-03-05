// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Inventory/InventoryReferenceHelper.h"

#include "Inventory/RockInventory.h"

bool FRockItemReference::IsValid() const
{
	return Inventory != nullptr && Inventory->IsHandleValid(ItemHandle);
}

FRockItemStack FRockItemReference::GetCopyOfItem() const
{
	if (Inventory == nullptr)
	{
		return FRockItemStack::Invalid();
	}
	
	return Inventory->GetItemByHandle(ItemHandle);
}

FRockItemReference::FRockItemReference(URockInventory* InInventory, FRockItemStackHandle InSlotHandle)
	: Inventory(InInventory), ItemHandle(InSlotHandle)
{
}

FRockInventorySlotEntry FRockSlotReference::GetSlotEntry() const
{
	if (Inventory == nullptr)
	{
		return FRockInventorySlotEntry::Invalid();
	}

	return Inventory->GetSlotByHandle(SlotHandle);
}

FRockSlotReference::FRockSlotReference(URockInventory* InInventory, FRockInventorySlotHandle InSlotHandle)
	: Inventory(InInventory), SlotHandle(InSlotHandle)
{
}
