// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventorySlot.h"
#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"

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

void FRockInventorySlotContainer::SetOwningInventory(URockInventory* InOwningInventory)
{
	OwnerInventory = InOwningInventory;
}

void FRockInventorySlotContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	if (!OwnerInventory)
	{
		return;
	}
	for (const int32 Index : RemovedIndices)
	{
		if (AllSlots.IsValidIndex(Index))
		{
			OwnerInventory->BroadcastSlotChanged(AllSlots[Index].SlotHandle, ERockSlotChangeType::Removed);
		}
	}
}

void FRockInventorySlotContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (!OwnerInventory)
	{
		return;
	}
	for (const int32 Index : AddedIndices)
	{
		if (AllSlots.IsValidIndex(Index))
		{
			OwnerInventory->BroadcastSlotChanged(AllSlots[Index].SlotHandle, ERockSlotChangeType::Added);
		}
	}
}

void FRockInventorySlotContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (!OwnerInventory)
	{
		return;
	}
	for (const int32 Index : ChangedIndices)
	{
		if (AllSlots.IsValidIndex(Index))
		{
			OwnerInventory->BroadcastSlotChanged(AllSlots[Index].SlotHandle, ERockSlotChangeType::Changed);
		}
	}
}
