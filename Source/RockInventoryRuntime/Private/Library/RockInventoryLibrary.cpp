// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryLibrary.h"

#include "RockInventoryLogging.h"
#include "Components/RockInventoryComponent.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventoryInterface.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockItemStackLibrary.h"


namespace
{
	/** Replicated inventory state is written on the authority only. A client call is refused and changes nothing. */
	bool HasMutationAuthority(URockInventory* Inventory, const TCHAR* Operation)
	{
		const AActor* OwningActor = Inventory ? Inventory->GetOwningActor() : nullptr;
		if (OwningActor && OwningActor->HasAuthority())
		{
			return true;
		}
		UE_LOG(LogRockInventory, Warning, TEXT("%s - refused, %s is not owned by an actor with authority"), Operation, *GetNameSafe(Inventory));
		return false;
	}
}

// What LootItemToInventory decided to do, computed without touching the inventory (see DecideLoot / CommitLoot).
struct URockInventoryLibrary::FLootDecision
{
	/** Partial stacks to top up, in slot order. */
	struct FMerge
	{
		FRockInventorySlotHandle SlotHandle;
		int32 Count = 0;
	};
	TArray<FMerge, TInlineAllocator<8>> Merges;

	/** Set when what is left after the merges goes into an empty slot as a new stack. */
	bool bPlaceNewStack = false;
	FRockInventorySlotHandle NewStackSlot;
	ERockItemOrientation NewStackOrientation = ERockItemOrientation::Horizontal;
	int32 NewStackCount = 0;

	/** Count that finds no home. 0 when everything is placed. */
	int32 Excess = 0;
};

void URockInventoryLibrary::DecideLoot(const URockInventory* Inventory, const FRockItemStack& ItemStack, FLootDecision& OutDecision)
{
	int32 Remaining = ItemStack.GetStackCount();
	const FIntPoint ItemSize = URockItemStackLibrary::GetItemSize(ItemStack);

	TArray<bool> OccupancyGrid;
	PrecomputeOccupancyGrids(Inventory, OccupancyGrid);

	// Slots another operation holds. The first operation listed for a slot is the one that counts.
	TBitArray<> PendingSlots;
	if (!Inventory->PendingSlotOperations.IsEmpty())
	{
		const int32 NumSlots = Inventory->SlotData.Num();
		PendingSlots.Init(false, NumSlots);
		TBitArray<> Seen(false, NumSlots);
		for (const FRockPendingSlotOperation& Operation : Inventory->PendingSlotOperations)
		{
			const int32 SlotIndex = Operation.SlotHandle.GetAbsoluteIndex();
			if (!Operation.SlotHandle.IsValid() || !Seen.IsValidIndex(SlotIndex) || Seen[SlotIndex])
			{
				continue;
			}
			Seen[SlotIndex] = true;
			PendingSlots[SlotIndex] = Operation.SlotStatus == ERockSlotStatus::Pending;
		}
	}

	// Sections are contiguous and in config order, so this visits slots in the same order as SlotData.
	for (const FRockInventorySectionInfo& Section : Inventory->SlotSections)
	{
		if (Remaining <= 0)
		{
			break;
		}
		const int32 FirstSlotIndex = Section.GetFirstSlotIndex();
		const int32 NumSectionSlots = Section.GetNumSlots();
		if (FirstSlotIndex == INDEX_NONE || NumSectionSlots <= 0)
		{
			continue;
		}
		// The type restriction is the same for every slot of the section
		if (!CanItemBePlacedInSection(ItemStack, Section))
		{
			continue;
		}

		const int32 Columns = Section.GetColumns();
		for (int32 LocalSlotIndex = 0; LocalSlotIndex < NumSectionSlots && Remaining > 0; ++LocalSlotIndex)
		{
			const int32 AbsoluteIndex = FirstSlotIndex + LocalSlotIndex;
			// We don't want to overwrite any pending operations
			if (PendingSlots.IsValidIndex(AbsoluteIndex) && PendingSlots[AbsoluteIndex])
			{
				continue;
			}
			const FRockInventorySlotEntry& Slot = Inventory->SlotData[AbsoluteIndex];

			// Same rule as CanMergeItemAtGridPosition with the Partial condition, without copying the stack
			const FRockItemStack* Existing = Inventory->GetItemByHandlePtr(Slot.ItemHandle);
			if (Existing && Existing->IsValid() && Existing->CanStackWith(ItemStack) && Existing->GetStackCount() < Existing->GetMaxStackCount())
			{
				const int32 MergeCount = FMath::Min(Existing->GetMaxStackCount() - Existing->GetStackCount(), Remaining);
				OutDecision.Merges.Add({Slot.SlotHandle, MergeCount});
				Remaining -= MergeCount;
				continue;
			}

			// Finally check if it fits spatially. Prefer the default orientation, then fall back to rotated for non-square items.
			const int32 Column = LocalSlotIndex % Columns;
			const int32 Row = LocalSlotIndex / Columns;
			ERockItemOrientation FitOrientation = ERockItemOrientation::Horizontal;
			bool bFits = CanItemFitInGridPosition(OccupancyGrid, Section, Column, Row, FVector2D(ItemSize));
			if (!bFits && ItemSize.X != ItemSize.Y)
			{
				FitOrientation = ERockItemOrientation::Vertical;
				bFits = CanItemFitInGridPosition(OccupancyGrid, Section, Column, Row, FVector2D(ItemSize.Y, ItemSize.X));
			}
			if (bFits)
			{
				OutDecision.bPlaceNewStack = true;
				OutDecision.NewStackSlot = Slot.SlotHandle;
				OutDecision.NewStackOrientation = FitOrientation;
				OutDecision.NewStackCount = Remaining;
				OutDecision.Excess = 0;
				return;
			}
		}
	}
	OutDecision.Excess = Remaining;
}

void URockInventoryLibrary::CommitLoot(
	URockInventory* Inventory, const FRockItemStack& ItemStack, const FLootDecision& Decision, FRockInventorySlotHandle& OutHandle)
{
	if (Decision.Merges.IsEmpty() && !Decision.bPlaceNewStack)
	{
		return;
	}

	// The one copy: the merge and add calls take the stack, and its count is the part being placed
	FRockItemStack Placing = ItemStack;
	for (const FLootDecision::FMerge& Merge : Decision.Merges)
	{
		Placing.StackCount = Merge.Count;
		MergeItemAtGridPosition(Inventory, Merge.SlotHandle, Placing);
	}
	if (Decision.bPlaceNewStack)
	{
		Placing.StackCount = Decision.NewStackCount;
		const FRockItemStackHandle& ItemHandle = Inventory->AddItemToInventory(Placing);
		FRockInventorySlotEntry SlotEntry = Inventory->GetSlotByHandle(Decision.NewStackSlot);
		SlotEntry.ItemHandle = ItemHandle;
		SlotEntry.Orientation = Decision.NewStackOrientation;
		Inventory->SetSlotByHandle(Decision.NewStackSlot, SlotEntry);
		OutHandle = Decision.NewStackSlot;
	}
}

bool URockInventoryLibrary::LootItemToInventory(
	URockInventory* Inventory, const FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess)
{
	// Start off with the full stack size in the event we can't place it
	OutExcess = ItemStack.GetStackCount();
	UE_LOG(LogRockInventory, Verbose, TEXT("LootItemToInventory::ItemStack: %s"), *ItemStack.GetDebugString());
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("LootItemToInventory::Invalid Parameters. Inventory"));
		return false;
	}
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("LootItemToInventory::Invalid Parameters. ItemStack"));
		return false;
	}
	if (!HasMutationAuthority(Inventory, TEXT("LootItemToInventory")))
	{
		return false;
	}

	FLootDecision Decision;
	DecideLoot(Inventory, ItemStack, Decision);
	CommitLoot(Inventory, ItemStack, Decision, OutHandle);

	// A partial placement still consumed some of the item. The caller must update their ItemStack with the excess.
	OutExcess = Decision.Excess;
	return Decision.Excess <= 0;
}

FRockItemStack URockInventoryLibrary::SplitItemStackAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, int32 Quantity)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return FRockItemStack::Invalid();
	}
	if (!HasMutationAuthority(Inventory, TEXT("SplitItemStackAtLocation")))
	{
		return FRockItemStack::Invalid();
	}
	const int32 slotIndex = SlotHandle.GetAbsoluteIndex();
	if (!Inventory->SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid SlotHandle: %s"), *SlotHandle.ToString());
		return FRockItemStack::Invalid();
	}

	FRockInventorySlotEntry SourceSlot = Inventory->GetSlotByHandle(SlotHandle);
	FRockItemStack Item = Inventory->GetItemByHandle(SourceSlot.ItemHandle);

	if (!Item.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("No valid item at slot"));
		return FRockItemStack::Invalid();
	}

	const int32 CurrentStackSize = Item.GetStackCount();

	// If quantity is 0 or negative, remove the entire stack
	if (Quantity <= 0)
	{
		Quantity = CurrentStackSize;
	}

	// Create the return item stack with the requested quantity
	FRockItemStack OutItemStack = Item;
	OutItemStack.StackCount = FMath::Min(Quantity, CurrentStackSize);

	const FRockItemStackHandle CachedItemHandle = SourceSlot.ItemHandle;

	const bool bIsFullStackMove = (Quantity >= CurrentStackSize);
	if (bIsFullStackMove)
	{
		// They should be the same at this point, otherwise something else might be going on?
		checkf(Item.ItemHandle == SourceSlot.ItemHandle, TEXT("ItemHandle mismatch"));

		// Remove the item from the inventory. It's up to the caller to add it back if needed.
		Inventory->RemoveItemFromInventory(Item);

		// Remove entire stack - invalidate the slot and item
		SourceSlot.ItemHandle = FRockItemStackHandle::Invalid();
		SourceSlot.Orientation = ERockItemOrientation::Horizontal;
	}
	else
	{
		// Partial removal - just update the stack size
		Item.StackCount = CurrentStackSize - Quantity;
		checkf(Item.GetStackCount() > 0, TEXT("ItemStack size is 0 or negative. Should have used full stack move path"));
		Inventory->SetItemByHandle(CachedItemHandle, Item);
	}
	Inventory->SetSlotByHandle(SlotHandle, SourceSlot);
	// Set the item handle to invalid, since we are returning a new item stack. It's not the same item anymore.
	// If it gets added to an inventory, it will get a new handle.
	OutItemStack.ItemHandle = FRockItemStackHandle::Invalid();
	return OutItemStack;
}

bool URockInventoryLibrary::MoveItem(
	URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
	URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
	const FRockMoveItemParams& InMoveParams)
{
	if ((SourceInventory && !HasMutationAuthority(SourceInventory, TEXT("MoveItem")))
		|| (TargetInventory && !HasMutationAuthority(TargetInventory, TEXT("MoveItem"))))
	{
		return false;
	}
	if (SourceInventory && SourceInventory == TargetInventory && SourceSlotHandle == TargetSlotHandle)
	{
		const FRockInventorySlotEntry& CurrentSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
		if (!CurrentSlot.IsValid() || !CurrentSlot.ItemHandle.IsValid() || CurrentSlot.Orientation == InMoveParams.DesiredOrientation)
		{
			// Nothing to do, item is already in the target location
			return true;
		}

		// Same slot, different orientation: rotate in place if the rotated footprint fits.
		const FRockItemStack& RotatingItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
		if (!RotatingItem.IsValid())
		{
			return true;
		}
		const FRockInventorySectionInfo& RotatingSection = SourceInventory->GetSectionInfoBySlotHandle(SourceSlotHandle);
		const int32 RotatingLocalIndex = RotatingSection.GetLocalIndex(SourceSlotHandle.GetAbsoluteIndex());
		TArray<bool> RotatingGrid;
		PrecomputeOccupancyGrids(SourceInventory, RotatingGrid, CurrentSlot.ItemHandle);
		const FVector2D RotatedSize = FVector2D(URockItemStackLibrary::GetItemSizeForOrientation(RotatingItem, InMoveParams.DesiredOrientation));
		if (!CanItemFitInGridPosition(RotatingGrid, RotatingSection, RotatingLocalIndex % RotatingSection.GetColumns(), RotatingLocalIndex / RotatingSection.GetColumns(), RotatedSize))
		{
			return false;
		}
		FRockInventorySlotEntry RotatedSlot = CurrentSlot;
		RotatedSlot.Orientation = InMoveParams.DesiredOrientation;
		SourceInventory->SetSlotByHandle(SourceSlotHandle, RotatedSlot);
		return true;
	}

	// If the TargetInventory is 'null', should we assume we are trying to 'drop' the item?
	if (!SourceInventory || !TargetInventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Source or Target Inventory"));
		return false;
	}
	const FRockInventorySlotEntry& ValidatedSourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
	if (!ValidatedSourceSlot.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Source Slot Handle"));
		return false;
	}
	const FRockItemStack& ValidatedSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
	if (!ValidatedSourceItem.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Source Slot is empty"));
		return false;
	}
	// Check TargetInventory if slot is empty
	const FRockInventorySlotEntry& ValidatedTargetSlot = TargetInventory->GetSlotByHandle(TargetSlotHandle);
	if (!ValidatedTargetSlot.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Target Slot Handle"));
		return false;
	}
	// Can CanItemBePlacedInSection of TargetInventory
	FRockInventorySectionInfo TargetSection = TargetInventory->GetSectionInfoBySlotHandle(TargetSlotHandle);
	if (!CanItemBePlacedInSection(ValidatedSourceItem, TargetSection))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Item cannot be placed in target section"));
		return false;
	}
	int32 MoveAmount = URockItemStackLibrary::CalculateMoveAmount(ValidatedSourceItem, InMoveParams.MoveMode, InMoveParams.MoveCount);
	if (MoveAmount <= 0)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid move amount calculated"));
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	/// Move
	TArray<bool> OccupancyGrid;

	// If moving between 2 different inventories, the ItemHandle at destination could in theory have the same index as the source
	// which means we need to only ignore the item if we are moving internal to the same inventory
	if (SourceInventory == TargetInventory)
	{
		PrecomputeOccupancyGrids(TargetInventory, OccupancyGrid, ValidatedSourceSlot.ItemHandle);
	}
	else
	{
		// Don't ignore any items in the target inventory
		PrecomputeOccupancyGrids(TargetInventory, OccupancyGrid);
	}
	const FRockInventorySectionInfo& targetSection = TargetInventory->GetSectionInfoBySlotHandle(TargetSlotHandle);
	const int32 localIndex = targetSection.GetLocalIndex(TargetSlotHandle.GetAbsoluteIndex());
	const int32 Column = localIndex % targetSection.GetColumns();
	const int32 Row = localIndex / targetSection.GetColumns();
	const FVector2D ItemSize = FVector2D(URockItemStackLibrary::GetItemSizeForOrientation(ValidatedSourceItem, InMoveParams.DesiredOrientation));

	if (CanItemFitInGridPosition(OccupancyGrid, targetSection, Column, Row, ItemSize))
	{
		FRockInventorySlotEntry targetSlot = ValidatedTargetSlot;
		const bool isFullStackMove = (MoveAmount == ValidatedSourceItem.GetStackCount());
		const bool isSameInventory = (SourceInventory == TargetInventory);

		if (isFullStackMove)
		{
			FRockInventorySlotEntry sourceSlot = ValidatedSourceSlot;
			// Invalidate the source slot
			sourceSlot.ItemHandle = FRockItemStackHandle::Invalid();
			sourceSlot.Orientation = ERockItemOrientation::Horizontal;
			SourceInventory->SetSlotByHandle(SourceSlotHandle, sourceSlot);

			if (isSameInventory)
			{
				// Same Inventory - just move the handle to the target slot.
				targetSlot.ItemHandle = ValidatedSourceItem.ItemHandle;
			}
			else
			{
				// Different inventory.
				// Release from source first. RemoveItemFromInventory unregisters the RuntimeInstance from its current
				// replication owner, so doing it after the add would unregister it from the target's owner instead.
				SourceInventory->RemoveItemFromInventory(ValidatedSourceItem);
				// Add to target
				targetSlot.ItemHandle = TargetInventory->AddItemToInventory(ValidatedSourceItem);
			}

			// Set up target slot with existing item handle
			targetSlot.Orientation = InMoveParams.DesiredOrientation;
			TargetInventory->SetSlotByHandle(TargetSlotHandle, targetSlot);
		}
		else
		{
			// Partial move

			// Split the source item stack based on the move amount
			auto ItemDef = ValidatedSourceItem.GetDefinition();

			// We currently aren't supporting partial moves of items that require runtime instances.
			if (!ItemDef->RuntimeInstanceClass.IsNull())
			{
				UE_LOG(LogRockInventory, Warning, TEXT("Partial moves of items that require runtime instances are not supported"));
				return false;
			}

			const FRockItemStack ItemToMove = SplitItemStackAtLocation(SourceInventory, SourceSlotHandle, MoveAmount);
			if (!ItemToMove.IsValid())
			{
				UE_LOG(LogRockInventory, Warning, TEXT("Failed to split item stack"));
				return false;
			}

			// Add split to target inventory.
			const FRockItemStackHandle& newItemHandle = TargetInventory->AddItemToInventory(ItemToMove);

			// Update target slot with new item
			targetSlot.ItemHandle = newItemHandle;
			targetSlot.Orientation = InMoveParams.DesiredOrientation;
			TargetInventory->SetSlotByHandle(TargetSlotHandle, targetSlot);
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////
	/// Merge into an existing item
	if (CanMergeItemAtGridPosition(TargetInventory, TargetSlotHandle, ValidatedSourceItem, ERockItemStackMergeCondition::Partial))
	{
		// Get the target item to calculate how much we can move
		const FRockItemStack& TargetItem = TargetInventory->GetItemByHandle(ValidatedTargetSlot.ItemHandle);
		if (!TargetItem.IsValid())
		{
			UE_LOG(LogRockInventory, Warning, TEXT("Invalid target item for merging"));
			return false;
		}

		const int32 targetCurrentStack = TargetItem.GetStackCount();
		const int32 targetMaxStack = TargetItem.GetMaxStackCount();
		const int32 sourceCurrentStack = ValidatedSourceItem.GetStackCount();

		// Calculate how much we can move
		const int32 availableSpace = targetMaxStack - targetCurrentStack;
		const int32 amountToMove = FMath::Min3(availableSpace, sourceCurrentStack, MoveAmount);

		if (amountToMove <= 0)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("No items can be merged"));
			return false;
		}

		// Update target item with new stack size
		FRockItemStack UpdatedTargetItem = TargetItem;
		UpdatedTargetItem.StackCount = targetCurrentStack + amountToMove;
		checkf(UpdatedTargetItem.GetStackCount() <= targetMaxStack,
		       TEXT("Updated target item stack size exceeds max: %d > %d"),
		       UpdatedTargetItem.GetStackCount(),
		       targetMaxStack);
		TargetInventory->SetItemByHandle(ValidatedTargetSlot.ItemHandle, UpdatedTargetItem);

		// Update source item with remaining stack size
		FRockItemStack UpdatedSourceItem = ValidatedSourceItem;
		UpdatedSourceItem.StackCount = sourceCurrentStack - amountToMove;
		checkf(UpdatedSourceItem.GetStackCount() >= 0,
		       TEXT("Updated source item stack size is negative: %d"),
		       UpdatedSourceItem.GetStackCount());

		const bool isSourceEmptied = (UpdatedSourceItem.GetStackCount() <= 0);

		if (isSourceEmptied)
		{
			FRockInventorySlotEntry sourceSlot = ValidatedSourceSlot;

			// Release the item
			SourceInventory->RemoveItemFromInventory(ValidatedSourceItem);

			// Clear the slot
			sourceSlot.ItemHandle = FRockItemStackHandle::Invalid();
			sourceSlot.Orientation = ERockItemOrientation::Horizontal;
			SourceInventory->SetSlotByHandle(SourceSlotHandle, sourceSlot);
		}
		else
		{
			// Source item still has items left, so we need to broadcast that it changed.
			SourceInventory->SetItemByHandle(ValidatedSourceSlot.ItemHandle, UpdatedSourceItem);
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// NOTE: Only cross this bridge when we get there.
	// TODO: Swap Item
	// Some games like Diablo support this but Tarkov does not.
	// The fact that some items can be placed 'into' other items makes this more complex.
	// We might not ever support this scenario.
	UE_LOG(LogRockInventory, Warning, TEXT("Item cannot be moved to target location"));
	return false;
}

bool URockInventoryLibrary::CanMergeItemAtGridPosition(
	const URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack,
	ERockItemStackMergeCondition MergeCondition)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return false;
	}
	if (!SlotHandle.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Slot Handle"));
		return false;
	}
	const FRockItemStack& ExistingItemStack = Inventory->GetItemBySlotHandle(SlotHandle);
	if (!ExistingItemStack.IsValid())
	{
		return false;
	}

	if (!ExistingItemStack.CanStackWith(ItemStack))
	{
		return false;
	}

	const int32 CurrentStackSize = ExistingItemStack.GetStackCount();
	const int32 MaxStackCount = ExistingItemStack.GetMaxStackCount();
	const int32 IncomingStackSize = ItemStack.GetStackCount();

	switch (MergeCondition)
	{
	case ERockItemStackMergeCondition::Full:
		// Can we merge the entire incoming stack?
		return (CurrentStackSize + IncomingStackSize) <= MaxStackCount;
	case ERockItemStackMergeCondition::Partial:
		// Is there any space at all in the existing stack?
		return CurrentStackSize < MaxStackCount;
	case ERockItemStackMergeCondition::None:
		// Cannot merge at all
		return CurrentStackSize >= MaxStackCount;
	default:
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid merge condition"));
		return false;
	}
}

int32 URockInventoryLibrary::MergeItemAtGridPosition(
	URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack)
{
	int32 stackSize = ItemStack.GetStackCount();
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return stackSize;
	}
	if (!HasMutationAuthority(Inventory, TEXT("MergeItemAtGridPosition")))
	{
		return stackSize;
	}
	if (!SlotHandle.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Slot Handle"));
		return stackSize;
	}
	const FRockInventorySlotEntry Slot = Inventory->GetSlotByHandle(SlotHandle);
	if (!Slot.IsValid() || !Slot.ItemHandle.IsValid())
	{
		// might be noisy?
		// UE_LOG(LogRockInventory, Warning, TEXT("Invalid Slot Handle"));
		return stackSize;
	}

	FRockItemStack ExistingItemStack = Inventory->GetItemBySlotHandle(SlotHandle);
	if (!ExistingItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Merging Failed: Invalid ItemStack"));
		return stackSize;
	}

	const int32 NewStackSize = ExistingItemStack.GetStackCount() + stackSize;
	const int32 MaxStackCount = ExistingItemStack.GetMaxStackCount();

	if (NewStackSize > MaxStackCount)
	{
		ExistingItemStack.StackCount = MaxStackCount;
		stackSize = NewStackSize - MaxStackCount;
	}
	else
	{
		ExistingItemStack.StackCount = NewStackSize;
		stackSize = 0;
	}
	Inventory->SetItemByHandle(Slot.ItemHandle, ExistingItemStack);
	return stackSize;
}

FRockItemStack URockInventoryLibrary::GetItemBySlotHandle(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemAtLocation: Invalid Inventory"));
		return FRockItemStack::Invalid();
	}
	return Inventory->GetItemBySlotHandle(SlotHandle);
}

FRockItemStack URockInventoryLibrary::GetItemByItemHandle(URockInventory* Inventory, const FRockItemStackHandle& ItemHandle)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemAtLocation: Invalid Inventory"));
		return FRockItemStack::Invalid();
	}
	return Inventory->GetItemByHandle(ItemHandle);
}

int32 URockInventoryLibrary::GetItemCount(const URockInventory* Inventory, const FName& ItemId)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return 0;
	}
	int32 ItemCount = 0;
	for (const FRockItemStack& ItemStack : Inventory->ItemData)
	{
		if (ItemStack.IsValid() && ItemStack.GetItemId() == ItemId)
		{
			ItemCount += ItemStack.GetStackCount();
		}
	}
	return ItemCount;
}

// bool URockInventoryLibrary::DropItem(URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle)
// {
// 	// Should we just call RemoveItem for 'drop' instead?
// 	// Since this Library shouldn't be responsible for 'spawning' the item, because who knows where we want to spawn it.
// 	
// }

bool URockInventoryLibrary::CanItemBePlacedInSection(
	const FRockItemStack& ItemStack,
	const FRockInventorySectionInfo& SectionInfo)
{
	// Check if the section has any type restrictions
	if (SectionInfo.GetSectionFilter().IsEmpty())
	{
		return true;
	}

	return SectionInfo.GetSectionFilter().Matches(ItemStack.GetDefinition()->GetAllTags());
}


void URockInventoryLibrary::PrecomputeOccupancyGrids(
	const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid, FRockItemStackHandle IgnoreItemHandle)
{
	const int32 totalGridSize = Inventory->SlotData.Num();
	OutOccupancyGrid.SetNum(totalGridSize);
	// Initialize all cells to false (unoccupied)
	OutOccupancyGrid.Init(false, totalGridSize);

	for (int32 TabIndex = 0; TabIndex < Inventory->SlotSections.Num(); ++TabIndex)
	{
		const FRockInventorySectionInfo& SectionInfo = Inventory->SlotSections[TabIndex];
		const int32 TabOffset = SectionInfo.GetFirstSlotIndex();

		// Mark occupied cells
		for (int32 SlotIndex = 0; SlotIndex < SectionInfo.GetNumSlots(); ++SlotIndex)
		{
			checkf(0 <= SlotIndex && SlotIndex < Inventory->SlotData.Num(),
			       TEXT("SlotIndex is out of range: %d (max: %d)"),
			       SlotIndex,
			       Inventory->SlotData.Num() - 1);

			const int32 Column = (SlotIndex) % SectionInfo.GetColumns();
			const int32 Row = (SlotIndex) / SectionInfo.GetColumns();

			const FRockInventorySlotEntry& ExistingItemSlot = Inventory->SlotData[TabOffset + SlotIndex];
			if (ExistingItemSlot.ItemHandle == IgnoreItemHandle)
			{
				continue; // Skip the item we are ignoring
			}

			const FRockItemStack& ExistingItemStack = Inventory->GetItemByHandle(ExistingItemSlot.ItemHandle);

			if (ExistingItemStack.IsValid())
			{
				const FVector2D ItemSize = FVector2D(URockItemStackLibrary::GetItemSizeForOrientation(ExistingItemStack, ExistingItemSlot.Orientation));
				auto SizePolicy = SectionInfo.GetSlotSizePolicy();

				// If size policy is IgnoreSize, only mark the single slot as occupied
				if (SizePolicy == ERockItemSizePolicy::IgnoreSize)
				{
					const int32 GridIndex = TabOffset + SlotIndex;
					checkf(0 <= GridIndex && GridIndex < totalGridSize,
					       TEXT("Grid index out of range: %d (max: %d)"),
					       GridIndex,
					       totalGridSize - 1);
					OutOccupancyGrid[GridIndex] = true;
				}
				else
				{
					// Mark all cells this item occupies as true
					for (int32 Y = 0; Y < ItemSize.Y; ++Y)
					{
						for (int32 X = 0; X < ItemSize.X; ++X)
						{
							const int32 GridX = Column + X;
							const int32 GridY = Row + Y;
							const int32 GridIndex = TabOffset + (GridY * SectionInfo.GetColumns() + GridX);
							checkf(0 <= GridIndex && GridIndex < totalGridSize,
							       TEXT("Grid index out of range: %d (max: %d)"),
							       GridIndex,
							       totalGridSize - 1);
							OutOccupancyGrid[GridIndex] = true;
						}
					}
				}
			}
		}
	}
}

bool URockInventoryLibrary::CanItemFitInGridPosition(
	const TArray<bool>& OccupancyGrid, const FRockInventorySectionInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize)
{
	// If this is an unrestricted section, we only need to check if the slot is occupied
	// Item size is irrelevant.
	if (TabInfo.GetSlotSizePolicy() == ERockItemSizePolicy::IgnoreSize)
	{
		// Without this, an out-of-range X would wrap into the next row (or another section).
		if (X < 0 || Y < 0 || X >= TabInfo.GetColumns() || Y >= TabInfo.GetRows())
		{
			return false;
		}
		const int32 GridIndex = TabInfo.GetFirstSlotIndex() + (Y * TabInfo.GetColumns() + X);
		if (GridIndex < 0 || GridIndex >= OccupancyGrid.Num())
		{
			return false;
		}
		return !OccupancyGrid[GridIndex];
	}

	const int32 ItemSizeX = ItemSize.X;
	const int32 ItemSizeY = ItemSize.Y;

	// pre-check to avoid wasting time on partial fits
	if (X < 0 || Y < 0 || X + ItemSizeX > TabInfo.GetColumns() || Y + ItemSizeY > TabInfo.GetRows())
	{
		// Out of bounds
		return false;
	}

	for (int32 ItemY = 0; ItemY < ItemSizeY; ++ItemY)
	{
		for (int32 ItemX = 0; ItemX < ItemSizeX; ++ItemX)
		{
			const int32 GridIndex = TabInfo.GetFirstSlotIndex() + ((Y + ItemY) * TabInfo.GetColumns() + (X + ItemX));
			if (GridIndex < 0 || GridIndex >= OccupancyGrid.Num())
			{
				// Out of bounds
				return false;
			}
			if (OccupancyGrid[GridIndex])
			{
				// Cell is occupied
				return false;
			}
		}
	}
	return true;
}

TArray<FString> URockInventoryLibrary::GetInventoryContentsDebug(const URockInventory* Inventory)
{
	if (!Inventory)
	{
		return {TEXT("Invalid Inventory")};
	}

	TArray<FString> InventoryContents;
	for (const FRockInventorySlotEntry& Slot : Inventory->SlotData)
	{
		FRockInventorySectionInfo section = Inventory->GetSectionInfoBySlotHandle(Slot.SlotHandle);
		const int32 localSlotIndex = section.GetLocalIndex(Slot.SlotHandle.GetAbsoluteIndex());

		const FRockItemStack& ItemStack = Inventory->GetItemByHandle(Slot.ItemHandle);
		auto SectionInfo = Inventory->GetSectionInfoBySlotHandle(Slot.SlotHandle);

		FString LineItem = FString::Printf(
			TEXT("Section:[%s] SlotIdx:[%d]; localIndex:[%d] ItemIdx:[%s], Item:[%s] Count:[%d]"),
			*SectionInfo.GetSectionTag().ToString(),
			Slot.SlotHandle.GetAbsoluteIndex(),
			localSlotIndex,
			*Slot.ItemHandle.ToString(),
			ItemStack.GetDefinition() ? *ItemStack.GetDefinition()->Name.ToString() : TEXT("None"),
			ItemStack.GetStackCount());

		InventoryContents.Add(LineItem);
	}

	for (const FRockItemStack& ItemStack : Inventory->ItemData)
	{
		FString LineItem = FString::Printf(
			TEXT("ItemIdx:[%s], Item:[%s] Count:[%d]"),
			*ItemStack.ItemHandle.ToString(),
			ItemStack.GetDefinition() ? *ItemStack.GetDefinition()->Name.ToString() : TEXT("None"),
			ItemStack.GetStackCount());

		InventoryContents.Add(LineItem);
	}
	return InventoryContents;
}

UObject* URockInventoryLibrary::GetTopLevelOwner(UObject* Instance)
{
	UObject* Current = Instance;
	while (Current)
	{
		if (AActor* Actor = Cast<AActor>(Current))
		{
			return Actor;
		}
		else if (UActorComponent* Comp = Cast<UActorComponent>(Current))
		{
			return Comp;
		}
		else if (const URockInventory* Inv = Cast<URockInventory>(Current))
		{
			Current = Inv->GetOwner();
			if (!Current)
			{
				// If we didn't have a proper owner, try and get the outer instead
				Current = Inv->GetOuter();
			}
		}
		else if (const URockItemInstance* ItemInstance = Cast<URockItemInstance>(Current))
		{
			Current = ItemInstance->OwningInventory;

			// This might happen if the item is on a WorldItem and not in an inventory
			if (!Current)
			{
				Current = ItemInstance->GetOuter();
			}
		}
		else
		{
			UE_LOG(LogRockInventory, Warning, TEXT("GetOwningActor Failed"));
			break;
		}
	}
	return nullptr;
}

URockInventory* URockInventoryLibrary::GetInventory(AActor* Actor, bool bFindComponentByClass)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	// Fast path
	if (const IRockInventoryOwnerInterface* InventoryOwner = Cast<IRockInventoryOwnerInterface>(Actor))
	{
		if (URockInventory* inventory = InventoryOwner->GetInventory())
		{
			return inventory;
		}
		// If the interface returns null, we could still try to find the component thru the actor's components
	}
	// Slow path
	if (bFindComponentByClass)
	{
		// If we didn't find it directly, search through all components
		if (URockInventoryComponent* Component = Actor->FindComponentByClass<URockInventoryComponent>())
		{
			return Component->Inventory;
		}
	}

	return nullptr;
}

int32 URockInventoryLibrary::GetSlotIndex(const FRockInventorySlotHandle& SlotHandle)
{
	return SlotHandle.GetAbsoluteIndex();
}

void URockInventoryLibrary::SetCustomValue1(URockInventory* Inventory, const FRockItemStackHandle& ItemHandle, int32 NewValue)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetCustomValue1: Invalid Inventory"));
		return;
	}
	if (!HasMutationAuthority(Inventory, TEXT("SetCustomValue1")))
	{
		return;
	}
	FRockItemStack ItemStack = Inventory->GetItemByHandle(ItemHandle);
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetCustomValue1: Invalid ItemHandle"));
		return;
	}
	ItemStack.CustomValue1 = NewValue;
	Inventory->SetItemByHandle(ItemHandle, ItemStack);
}

void URockInventoryLibrary::SetCustomValue2(URockInventory* Inventory, const FRockItemStackHandle& ItemHandle, int32 NewValue)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetCustomValue2: Invalid Inventory"));
		return;
	}
	if (!HasMutationAuthority(Inventory, TEXT("SetCustomValue2")))
	{
		return;
	}
	FRockItemStack ItemStack = Inventory->GetItemByHandle(ItemHandle);
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetCustomValue2: Invalid ItemHandle"));
		return;
	}
	ItemStack.CustomValue2 = NewValue;
	Inventory->SetItemByHandle(ItemHandle, ItemStack);
}

FRockInventorySlotHandle URockInventoryLibrary::FindFirstSlotInSection(URockInventory* Inventory, FGameplayTag SectionTag)
{
	if (!Inventory) { return FRockInventorySlotHandle::Invalid(); }

	// We use the Core Query system to build the specific search
	FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithSectionTag(SectionTag);
	if (const FRockInventorySlotEntry* Slot = Inventory->FindFirstSlot(Q))
	{
		return Slot->SlotHandle; // Return the handle (safe for BP/external use)
	}

	return FRockInventorySlotHandle::Invalid();
}

TArray<FRockInventorySlotHandle> URockInventoryLibrary::FindAllSlotsInSection(URockInventory* Inventory, FGameplayTag SectionTag)
{
	TArray<FRockInventorySlotHandle> Slots;
	if (!Inventory) { return Slots; }

	// We use the Core Query system to build the specific search
	FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithSectionTag(SectionTag);
	TArray<FRockInventorySlotEntry> FoundSlots = Inventory->FindAllSlots(Q);
	for (const FRockInventorySlotEntry& Slot : FoundSlots)
	{
		Slots.Add(Slot.SlotHandle);
	}
	return Slots;
}

FRockInventorySlotHandle URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(URockInventory* Inventory, FGameplayTag SectionMetaTag)
{
	if (!Inventory) { return FRockInventorySlotHandle::Invalid(); }

	// We use the Core Query system to build the specific search
	FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithMetaTag(SectionMetaTag);
	if (const FRockInventorySlotEntry* Slot = Inventory->FindFirstSlot(Q))
	{
		return Slot->SlotHandle; // Return the handle (safe for BP/external use)
	}

	return FRockInventorySlotHandle::Invalid();
}

TArray<FRockInventorySlotHandle> URockInventoryLibrary::FindAllSlotsInSectionsWithMetaTag(URockInventory* Inventory, FGameplayTag SectionMetaTag)
{
	TArray<FRockInventorySlotHandle> Slots;
	if (!Inventory) { return Slots; }

	// We use the Core Query system to build the specific search
	FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithMetaTag(SectionMetaTag);
	TArray<FRockInventorySlotEntry> FoundSlots = Inventory->FindAllSlots(Q);
	for (FRockInventorySlotEntry slot : FoundSlots)
	{
		Slots.Add(slot.SlotHandle);
	}
	return Slots;
}
