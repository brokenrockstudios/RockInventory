// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventorySlot.h"
#include "RockInventoryLogging.h"

void FRockInventorySlot::Reset()
{
	Item.Reset();
	Orientation = ERockItemOrientation::Horizontal;
	bIsLocked = false;
}

void FRockInventorySlot::PreReplicatedRemove(const struct FRockInventoryData& InArraySerializer)
{
	// Notify any listeners that this slot is about to be removed
	// This could be used to clean up any references or perform pre-removal validation
}

void FRockInventorySlot::PostReplicatedAdd(const struct FRockInventoryData& InArraySerializer)
{
	// Validate the slot after replication
	if (!IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid slot data received after replication"));
		Reset();
	}
}

void FRockInventorySlot::PostReplicatedChange(const struct FRockInventoryData& InArraySerializer)
{
	// Validate the slot after changes
	if (!IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid slot data received after change"));
		Reset();
	}
}

bool FRockInventorySlot::IsValid() const
{
	// A slot is valid if:
	// 1. The slot handle is valid
	// 2. If the item is empty, the orientation is horizontal (default)
	// Note: We don't have to have an item to be valid. 
	if (!SlotHandle.IsValid())
	{
		return false;
	}
	if (!Item.IsValid())
	{
		return Orientation == ERockItemOrientation::Horizontal;
	}
	return true;
}

bool FRockInventorySlot::IsEmpty() const
{
	return !Item.IsValid();
}

bool FRockInventorySlot::CanAcceptItem(const FRockItemStack& NewItem) const
{
	// A slot can accept an item if:
	// 1. The slot is not locked
	// 2. The slot is either empty or can stack with the new item
	if (bIsLocked)
	{
		return false;
	}

	if (IsEmpty())
	{
		return true;
	}

	return Item.CanStackWith(NewItem);
}
