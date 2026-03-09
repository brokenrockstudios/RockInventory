// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Inventory/InventoryReferenceHelper.h"

#include "Inventory/RockInventory.h"


FRockItemReference::FRockItemReference(URockInventory* InInventory, FRockItemStackHandle InSlotHandle)
	: Inventory(InInventory), ItemHandle(InSlotHandle)
{
}

FRockItemStack FRockItemReference::GetCopyOfItem() const
{
	if (Inventory == nullptr)
	{
		return FRockItemStack::Invalid();
	}

	return Inventory->GetItemByHandle(ItemHandle);
}

TOptional<int32> FRockItemReference::GetCustomValueByTag(FGameplayTag Tag) const
{
	const FRockItemStack* Stack = Inventory->GetItemByHandlePtr(ItemHandle);
	if (!Stack) { return {}; }
	return Stack->GetCustomValueByTag(Tag);
}

bool FRockItemReference::IsValid() const
{
	if (Inventory == nullptr)
	{
		return false;
	}
	if (!Inventory->IsHandleValid(ItemHandle))
	{
		return false;
	}
	return Inventory->GetItemByHandle(ItemHandle).IsValid();
}

URockItemDefinition* FRockItemReference::GetDefinition() const
{
	if (Inventory == nullptr)
	{
		return nullptr;
	}
	if (!Inventory->IsHandleValid(ItemHandle))
	{
		return nullptr;
	}
	return Inventory->GetItemByHandle(ItemHandle).GetDefinition();
}

URockItemInstance* FRockItemReference::GetRuntimeInstanceOrNull() const
{
	if (Inventory == nullptr)
	{
		return nullptr;
	}
	if (!Inventory->IsHandleValid(ItemHandle))
	{
		return nullptr;
	}
	return Inventory->GetItemByHandle(ItemHandle).GetRuntimeInstance();
}

int32 FRockItemReference::GetStackCount() const
{
	if (Inventory == nullptr)
	{
		return 0;
	}
	if (!Inventory->IsHandleValid(ItemHandle))
	{
		return 0;
	}
	return Inventory->GetItemByHandle(ItemHandle).GetStackCount();
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
