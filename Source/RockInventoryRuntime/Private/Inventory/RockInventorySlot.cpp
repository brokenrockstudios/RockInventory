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
			FRockInventorySlotEntry& Slot = AllSlots[Index];
			
			// Initialize a tracking handle so PostReplicatedChange can detect transitions
			Slot.LastKnownItemHandle = Slot.ItemHandle;
			auto isItemValid = OwnerInventory->GetItemByHandle(Slot.LastKnownItemHandle).IsValid();
			ERockSlotChangeType ChangeType = isItemValid ?  ERockSlotChangeType::ItemAdded : ERockSlotChangeType::None;
			
			if (ChangeType != ERockSlotChangeType::None)
			{
				OwnerInventory->BroadcastSlotChanged(Slot.SlotHandle, ChangeType);
			}
		}
	}
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
			FRockInventorySlotEntry& Slot = AllSlots[Index];
			// Defensive:
			// If a slot being removed still references a valid item when the slot itself is removed,
			// we broadcast ItemRemoved so listeners could clean up. Normally items should be ejected
			// before slots are removed, but replication ordering isn't always guaranteed.
			if (OwnerInventory->GetItemByHandle(Slot.ItemHandle).IsValid())
			{
				OwnerInventory->BroadcastSlotChanged(AllSlots[Index].SlotHandle, ERockSlotChangeType::ItemRemoved);
			}
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
			FRockInventorySlotEntry& Slot = AllSlots[Index];
			const bool bHadItem = OwnerInventory->GetItemByHandle(Slot.LastKnownItemHandle).IsValid();
			const bool bHasItem = OwnerInventory->GetItemByHandle(Slot.ItemHandle).IsValid();
			ERockSlotChangeType ChangeType = ERockSlotChangeType::None;
			if (!bHadItem && bHasItem)
			{
				ChangeType = ERockSlotChangeType::ItemAdded;
			}
			else if (bHadItem && !bHasItem)
			{
				ChangeType = ERockSlotChangeType::ItemRemoved;
			}
			else if (bHadItem && bHasItem && Slot.ItemHandle != Slot.LastKnownItemHandle)
			{
				ChangeType = ERockSlotChangeType::ItemChanged;
			}
			else
			{
				// Something has changed, and if it wasn't an item, it was anything else (orientation, lock state, etc.)
				ChangeType = ERockSlotChangeType::PropertiesChanged;
			}
			// Update tracking for next change
			Slot.LastKnownItemHandle = Slot.ItemHandle;
			OwnerInventory->BroadcastSlotChanged(AllSlots[Index].SlotHandle, ChangeType);
		}
	}
}
