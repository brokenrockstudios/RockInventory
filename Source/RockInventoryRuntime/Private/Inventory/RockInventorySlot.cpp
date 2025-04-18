// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventorySlot.h"
#include "RockInventoryLogging.h"

FRockInventorySlotEntry::FRockInventorySlotEntry():
	ItemHandle(),
	SlotHandle(),
	Orientation(ERockItemOrientation::Horizontal),
	bIsLocked(false)
{
}

FRockInventorySlotEntry& FRockInventorySlotEntry::Invalid()
{
	static FRockInventorySlotEntry InvalidSlot;
	return InvalidSlot;
}

void FRockInventorySlotEntry::Reset()
{
	ItemHandle.Reset();
	Orientation = ERockItemOrientation::Horizontal;
	bIsLocked = false;
}

void FRockInventorySlotEntry::PreReplicatedRemove(const struct FRockInventorySlotContainer& InArraySerializer)
{
	// Notify any listeners that this slot is about to be removed
	// This could be used to clean up any references or perform pre-removal validation
}

void FRockInventorySlotEntry::PostReplicatedAdd(const struct FRockInventorySlotContainer& InArraySerializer)
{
	// Validate the slot after replication
	if (!IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid slot data received after replication"));
		Reset();
	}
}

void FRockInventorySlotEntry::PostReplicatedChange(const struct FRockInventorySlotContainer& InArraySerializer)
{
	// Validate the slot after changes
	if (!IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid slot data received after change"));
		Reset();
	}
}

bool FRockInventorySlotEntry::IsValid() const
{
	// A slot is valid if:
	// 1. The slot handle is valid
	if (!SlotHandle.IsValid())
	{
		return false;
	}
	return true;
}

// bool FRockInventorySlotEntry::IsEmpty() const
// {
// 	return !Item.IsValid();
// }

// bool FRockInventorySlotEntry::CanAcceptItem(const FRockItemStack& NewItem) const
// {
// 	// A slot can accept an item if:
// 	// 1. The slot is not locked
// 	// 2. The slot is either empty or can stack with the new item
// 	if (bIsLocked)
// 	{
// 		return false;
// 	}
//
// 	if (IsEmpty())
// 	{
// 		return true;
// 	}
//
// 	return Item.CanStackWith(NewItem);
// }
