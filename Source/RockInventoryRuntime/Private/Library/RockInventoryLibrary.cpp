// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryLibrary.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Library/RockItemStackLibrary.h"


bool URockInventoryLibrary::LootItemToInventory(
	URockInventory* Inventory, const FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess)
{
	// Start off with the full stack size in the event we can't place it
	OutExcess = ItemStack.GetStackSize();

	if (!Inventory || !ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("LootItemToInventory::Invalid Parameters. Inventory or ItemStack"));
		return false;
	}

	// Create temporary occupancy grids for each tab
	TArray<bool> OccupancyGrids;
	PrecomputeOccupancyGrids(Inventory, OccupancyGrids);

	TArray<FRockInventorySectionInfo> SlotSections = Inventory->SlotSections;
	const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(ItemStack);
	FRockItemStack ItemStackCopy = ItemStack;

	for (const FRockInventorySlotEntry Slot : Inventory->SlotData)
	{
		if (ItemStackCopy.GetStackSize() <= 0)
		{
			// No more items to place
			break;
		}
		FRockInventorySlotHandle SlotHandle = Slot.SlotHandle;
		const FRockInventorySectionInfo& SectionInfo = SlotSections[SlotHandle.GetSectionIndex()];
		const int32 SectionSlotIndex = SlotHandle.GetIndex() - SectionInfo.FirstSlotIndex;
		const int32 Column = SectionSlotIndex % SectionInfo.Width;
		const int32 Row = SectionSlotIndex / SectionInfo.Width;

		// TODO: If we have a stackable item, we should check if we can stack it instead of placing it
		// And continue to the next slot
		if (CanItemFitInGridPosition(OccupancyGrids, SectionInfo, Column, Row, ItemSize))
		{
			const FRockItemStackHandle& ItemHandle = Inventory->AddItemToInventory(ItemStackCopy);
			OutHandle = SlotHandle;
			OutExcess = 0;
			PlaceItemAtSlot(Inventory, OutHandle, ItemHandle, ERockItemOrientation::Horizontal);
			return true;
		}
		else if (CanMergeItemAtGridPosition(Inventory, SlotHandle, ItemStackCopy, ERockItemStackMergeCondition::Partial))
		{
			OutExcess = MergeItemAtGridPosition(Inventory, SlotHandle, ItemStackCopy);
			ItemStackCopy.StackSize = OutExcess;
		}
	}
	// What should we do if we were only able to partially place the item?
	// Update remaining OutExcess
	if (ItemStackCopy.StackSize <= 0)
	{
		OutExcess = 0;
		return true;
	}
	// We technically should have consumed SOME of the item. Make sure the caller updates their ItemStack with the out excess
	return false;
}


bool URockInventoryLibrary::PlaceItemAtSlot(
	URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, const FRockItemStackHandle& ItemStackHandle,
	ERockItemOrientation DesiredOrientation)
{
	if (!Inventory || !ItemStackHandle.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("PlaceItemAtLocation: Invalid Inventory or ItemStack"));
		return false;
	}
	if (SlotHandle.IsValid())
	{
		FRockInventorySlotEntry Slot = Inventory->GetSlotByHandle(SlotHandle);
		Slot.ItemHandle = ItemStackHandle;
		Slot.Orientation = DesiredOrientation;
		Inventory->SetSlotByHandle(SlotHandle, Slot);

		Inventory->BroadcastSlotChanged(SlotHandle);
		return true;
	}

	UE_LOG(LogRockInventory, Warning, TEXT("PlaceItemAtLocation Failed"));
	return false;
}

bool URockInventoryLibrary::PlaceItemAtSlot_Internal(
	URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, const FRockItemStack& ItemStack, ERockItemOrientation DesiredOrientation)
{
	if (SlotHandle.IsValid())
	{
		const FRockItemStackHandle& ItemHandle = Inventory->AddItemToInventory(ItemStack);
		return PlaceItemAtSlot(Inventory, SlotHandle, ItemHandle, DesiredOrientation);
	}
	return false;
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
		const FRockInventorySectionInfo& TabInfo = Inventory->SlotSections[TabIndex];
		const int32 TabOffset = TabInfo.FirstSlotIndex;

		// Mark occupied cells
		for (int32 SlotIndex = 0; SlotIndex < TabInfo.GetNumSlots(); ++SlotIndex)
		{
			checkf(0 <= SlotIndex && SlotIndex < Inventory->SlotData.Num(),
				TEXT("SlotIndex is out of range: %d (max: %d)"), SlotIndex, Inventory->SlotData.Num() - 1);

			const int32 Column = (SlotIndex) % TabInfo.Width;
			const int32 Row = (SlotIndex) / TabInfo.Width;

			const FRockInventorySlotEntry& ExistingItemSlot = Inventory->SlotData[TabOffset + SlotIndex];
			if (ExistingItemSlot.ItemHandle == IgnoreItemHandle)
			{
				continue; // Skip the item we are ignoring
			}

			const FRockItemStack& ExistingItemStack = Inventory->GetItemByHandle(ExistingItemSlot.ItemHandle);

			if (ExistingItemStack.IsValid())
			{
				const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(ExistingItemStack);

				// Mark all cells this item occupies as true
				for (int32 Y = 0; Y < ItemSize.Y; ++Y)
				{
					for (int32 X = 0; X < ItemSize.X; ++X)
					{
						const int32 GridX = Column + X;
						const int32 GridY = Row + Y;
						const int32 GridIndex = TabOffset + (GridY * TabInfo.Width + GridX);
						checkf(0 <= GridIndex && GridIndex < totalGridSize,
							TEXT("Grid index out of range: %d (max: %d)"), GridIndex, totalGridSize - 1);
						OutOccupancyGrid[GridIndex] = true;
					}
				}
			}
		}
	}
}

bool URockInventoryLibrary::CanItemFitInGridPosition(
	const TArray<bool>& OccupancyGrid, const FRockInventorySectionInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize)
{
	const int32 ItemSizeX = ItemSize.X;
	const int32 ItemSizeY = ItemSize.Y;

	// pre-check to avoid wasting time on partial fits
	if (X < 0 || Y < 0 || X + ItemSizeX > TabInfo.Width || Y + ItemSizeY > TabInfo.Height)
	{
		// Out of bounds
		return false;
	}


	for (int32 ItemY = 0; ItemY < ItemSizeY; ++ItemY)
	{
		for (int32 ItemX = 0; ItemX < ItemSizeX; ++ItemX)
		{
			const int32 GridIndex = TabInfo.FirstSlotIndex + ((Y + ItemY) * TabInfo.Width + (X + ItemX));
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


FRockItemStack URockInventoryLibrary::GetItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemAtLocation: Invalid Inventory"));
		return FRockItemStack::Invalid();
	}
	return Inventory->GetItemBySlotHandle(SlotHandle);
}

FRockItemStack URockInventoryLibrary::SplitItemStackAtLocation(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, int32 Quantity)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return FRockItemStack::Invalid();
	}

	const int32 slotIndex = SlotHandle.GetIndex();
	if (!Inventory->SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid SlotHandle: %s"), *SlotHandle.ToString());
		return FRockItemStack::Invalid();
	}

	FRockInventorySlotEntry Slot = Inventory->GetSlotByHandle(SlotHandle);
	FRockItemStack Item = Inventory->GetItemByHandle(Slot.ItemHandle);

	if (!Item.IsValid() || !Item.IsOccupied())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("No valid item at slot"));
		return FRockItemStack::Invalid();
	}

	const int32 CurrentStackSize = Item.GetStackSize();

	// If quantity is 0 or negative, remove the entire stack
	if (Quantity <= 0)
	{
		Quantity = CurrentStackSize;
	}

	// Create the return item stack with the requested quantity
	FRockItemStack OutItemStack = Item;
	OutItemStack.StackSize = FMath::Min(Quantity, CurrentStackSize);

	if (Quantity >= CurrentStackSize)
	{
		// Remove entire stack - invalidate the slot and item
		Slot.ItemHandle = FRockItemStackHandle::Invalid();
		Slot.Orientation = ERockItemOrientation::Horizontal;
		Inventory->SetSlotByHandle(SlotHandle, Slot);

		Item.bIsOccupied = false;
		Item.Generation++;
		Inventory->SetItemByHandle(Slot.ItemHandle, Item);
	}
	else
	{
		// Partial removal - just update the stack size
		Item.StackSize = CurrentStackSize - Quantity;
		Inventory->SetItemByHandle(Slot.ItemHandle, Item);
	}

	Inventory->BroadcastSlotChanged(SlotHandle);
	return OutItemStack;
}


bool URockInventoryLibrary::MoveItem(
	URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
	URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
	const FRockMoveItemParams& MoveItemParams)
{
	if (SourceInventory == TargetInventory && SourceSlotHandle == TargetSlotHandle)
	{
		// Nothing to do, item is already in the target location
		return true;
	}

	// If the TargetInventory is 'null', should we assume we are trying to 'drop' the item?
	if (!SourceInventory || !TargetInventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Source or Target Inventory"));
		return false;
	}

	const FRockInventorySlotEntry& SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
	if (!SourceSlot.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Source Slot Handle"));
		return false;
	}
	const FRockItemStack& SourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
	if (!SourceItem.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Source Slot is empty"));
		return false;
	}

	// Check TargetInventory if slot is empty
	const FRockInventorySlotEntry& TargetSlot = TargetInventory->GetSlotByHandle(TargetSlotHandle);
	if (!TargetSlot.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Target Slot Handle"));
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	/// Move
	TArray<bool> OccupancyGrid;
	PrecomputeOccupancyGrids(TargetInventory, OccupancyGrid, SourceSlot.ItemHandle);

	const FRockInventorySectionInfo& targetSection = TargetInventory->SlotSections[TargetSlotHandle.GetSectionIndex()];
	const int32 SectionIndex = TargetSlotHandle.GetIndex() - targetSection.FirstSlotIndex;
	const int32 Column = SectionIndex % targetSection.Width;
	const int32 Row = SectionIndex / targetSection.Width;
	const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(SourceItem);

	int32 MoveAmount = URockItemStackLibrary::CalculateMoveAmount(SourceItem, MoveItemParams);
	if (MoveAmount <= 0)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid move amount calculated"));
		return false;
	}

	if (CanItemFitInGridPosition(OccupancyGrid, targetSection, Column, Row, ItemSize))
	{
		if (SourceInventory == TargetInventory)
		{
			// If we're moving the entire stack, we can just move the item handle
			if (MoveAmount == SourceItem.GetStackSize())
			{
				// We don't use the Remove and Place in the same inventory because
				// those functions invalidates the itemslot and re-adds it, which is unnecessary and wasteful on same inventory
				FRockInventorySlotEntry SourceSlotRef = SourceInventory->GetSlotByHandle(SourceSlotHandle);
				SourceSlotRef.ItemHandle = FRockItemStackHandle::Invalid();
				SourceSlotRef.Orientation = ERockItemOrientation::Horizontal;
				SourceInventory->SetSlotByHandle(SourceSlotHandle, SourceSlotRef);

				FRockInventorySlotEntry TargetSlotRef = TargetInventory->GetSlotByHandle(TargetSlotHandle);
				TargetSlotRef.ItemHandle = SourceSlot.ItemHandle;
				TargetSlotRef.Orientation = MoveItemParams.DesiredOrientation;
				TargetInventory->SetSlotByHandle(TargetSlotHandle, TargetSlotRef);
			}
			else
			{
				// We need to split the stack
				const FRockItemStack ItemToMove = SplitItemStackAtLocation(SourceInventory, SourceSlotHandle, MoveAmount);
				if (!ItemToMove.IsValid())
				{
					UE_LOG(LogRockInventory, Warning, TEXT("Failed to split item stack"));
					return false;
				}

				// Place the split item in the target slot
				const FRockItemStackHandle& NewItemHandle = TargetInventory->AddItemToInventory(ItemToMove);
				FRockInventorySlotEntry TargetSlotRef = TargetInventory->GetSlotByHandle(TargetSlotHandle);
				TargetSlotRef.ItemHandle = NewItemHandle;
				TargetSlotRef.Orientation = MoveItemParams.DesiredOrientation;
				TargetInventory->SetSlotByHandle(TargetSlotHandle, TargetSlotRef);
			}

			SourceInventory->BroadcastSlotChanged(SourceSlotHandle);
			TargetInventory->BroadcastSlotChanged(TargetSlotHandle);
		}
		else
		{
			// Split the source item stack based on the move amount
			const FRockItemStack ItemToMove = SplitItemStackAtLocation(SourceInventory, SourceSlotHandle, MoveAmount);
			if (!ItemToMove.IsValid())
			{
				UE_LOG(LogRockInventory, Warning, TEXT("Failed to split item stack"));
				return false;
			}

			// Place the split item in the target inventory
			if (!PlaceItemAtSlot_Internal(TargetInventory, TargetSlotHandle, ItemToMove, MoveItemParams.DesiredOrientation))
			{
				UE_LOG(LogRockInventory, Warning, TEXT("Failed to place item in target inventory"));
				return false;
			}
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	/// Merge into an existing item
	if (CanMergeItemAtGridPosition(TargetInventory, TargetSlotHandle, SourceItem, ERockItemStackMergeCondition::Partial))
	{
		// Get the target item to calculate how much we can move
		const FRockItemStack& TargetItem = TargetInventory->GetItemByHandle(TargetSlot.ItemHandle);
		if (!TargetItem.IsValid())
		{
			UE_LOG(LogRockInventory, Warning, TEXT("Invalid target item for merging"));
			return false;
		}

		const int32 TargetCurrentStack = TargetItem.GetStackSize();
		const int32 TargetMaxStack = TargetItem.GetMaxStackSize();
		const int32 SourceCurrentStack = SourceItem.GetStackSize();

		// Calculate how much we can move, considering both the available space and the requested move amount
		const int32 AvailableSpace = TargetMaxStack - TargetCurrentStack;
		const int32 AmountToMove = FMath::Min3(AvailableSpace, SourceCurrentStack, MoveAmount);

		if (AmountToMove <= 0)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("No items can be merged"));
			return false;
		}

		// Update target item with new stack size
		FRockItemStack UpdatedTargetItem = TargetItem;
		UpdatedTargetItem.StackSize = TargetCurrentStack + AmountToMove;
		TargetInventory->SetItemByHandle(TargetSlot.ItemHandle, UpdatedTargetItem);

		// Update source item with remaining stack size
		FRockItemStack UpdatedSourceItem = SourceItem;
		UpdatedSourceItem.StackSize = SourceCurrentStack - AmountToMove;
		SourceInventory->SetItemByHandle(SourceSlot.ItemHandle, UpdatedSourceItem);

		// Broadcast changes
		SourceInventory->BroadcastSlotChanged(SourceSlotHandle);
		TargetInventory->BroadcastSlotChanged(TargetSlotHandle);

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// NOTE: Only cross this bridge when we get there.
	// TODO: Swap Item
	// Some games like Diablo support this but Tarkov does not.
	// The fact that some items can be placed 'into' other items makes this more complex.
	// We might not ever support this scenario.
	// return true;
	UE_LOG(LogRockInventory, Warning, TEXT("Item cannot be moved to target location"));
	
	return true;
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
	if (!ExistingItemStack.IsValid() || !ExistingItemStack.IsOccupied())
	{
		return false;
	}

	if (!ExistingItemStack.CanStackWith(ItemStack))
	{
		return false;
	}

	const int32 CurrentStackSize = ExistingItemStack.GetStackSize();
	const int32 MaxStackSize = ExistingItemStack.GetMaxStackSize();
	const int32 IncomingStackSize = ItemStack.GetStackSize();

	switch (MergeCondition)
	{
	case ERockItemStackMergeCondition::Full:
		// Can we merge the entire incoming stack?
		return (CurrentStackSize + IncomingStackSize) <= MaxStackSize;
	case ERockItemStackMergeCondition::Partial:
		// Is there any space at all in the existing stack?
		return CurrentStackSize < MaxStackSize;
	case ERockItemStackMergeCondition::None:
		// Cannot merge at all
		return CurrentStackSize >= MaxStackSize;
	default:
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid merge condition"));
		return false;
	}
}

int32 URockInventoryLibrary::MergeItemAtGridPosition(
	URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack)
{
	int32 stackSize = ItemStack.GetStackSize();
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return stackSize;
	}
	if (!SlotHandle.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Slot Handle"));
		return stackSize;
	}
	const FRockInventorySlotEntry Slot = Inventory->GetSlotByHandle(SlotHandle);
	FRockItemStack ExistingItemStack = Inventory->GetItemBySlotHandle(SlotHandle);

	if (ExistingItemStack.IsValid() && ExistingItemStack.IsOccupied())
	{
		const int32 NewStackSize = ExistingItemStack.GetStackSize() + stackSize;
		const int32 MaxStackSize = ExistingItemStack.GetMaxStackSize();

		if (NewStackSize > MaxStackSize)
		{
			ExistingItemStack.StackSize = MaxStackSize;
			stackSize = NewStackSize - MaxStackSize;
		}
		else
		{
			ExistingItemStack.StackSize = NewStackSize;
			stackSize = 0;
		}
		Inventory->SetItemByHandle(Slot.ItemHandle, ExistingItemStack);
		return stackSize;
	}
	else
	{
		// Should this ever even be reachable?
		UE_LOG(LogRockInventory, Warning, TEXT("Merging Failed: Invalid ItemStack"));
	}
	return stackSize;
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
		if (ItemStack.IsValid() && ItemStack.IsOccupied() && ItemStack.GetItemId() == ItemId)
		{
			ItemCount += ItemStack.GetStackSize();
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
