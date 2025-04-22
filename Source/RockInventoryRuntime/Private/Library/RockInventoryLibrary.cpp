// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryLibrary.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Item/RockItemDefinition.h"
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

		// First check if the item can be placed in this section based on type restrictions
		if (!CanItemBePlacedInSection(ItemStackCopy, SectionInfo))
		{
			continue;
		}

		// Then check if we can stack it
		if (CanMergeItemAtGridPosition(Inventory, SlotHandle, ItemStackCopy, ERockItemStackMergeCondition::Partial))
		{
			OutExcess = MergeItemAtGridPosition(Inventory, SlotHandle, ItemStackCopy);
			ItemStackCopy.StackSize = OutExcess;
			continue;
		}

		// Finally check if it fits spatially
		if (CanItemFitInGridPosition(OccupancyGrids, SectionInfo, Column, Row, ItemSize))
		{
			const FRockItemStackHandle& ItemHandle = Inventory->AddItemToInventory(ItemStackCopy);
			OutExcess = 0;
			FRockInventorySlotEntry SlotEntry = Inventory->GetSlotByHandle(SlotHandle);
			SlotEntry.ItemHandle = ItemHandle;
			SlotEntry.Orientation = ERockItemOrientation::Horizontal;
			Inventory->SetSlotByHandle(SlotHandle, SlotEntry);
			Inventory->BroadcastSlotChanged(SlotHandle);
			OutHandle = SlotHandle;
			return true;
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
		const int32 TabOffset = SectionInfo.FirstSlotIndex;

		// Mark occupied cells
		for (int32 SlotIndex = 0; SlotIndex < SectionInfo.GetNumSlots(); ++SlotIndex)
		{
			checkf(0 <= SlotIndex && SlotIndex < Inventory->SlotData.Num(),
				TEXT("SlotIndex is out of range: %d (max: %d)"), SlotIndex, Inventory->SlotData.Num() - 1);

			const int32 Column = (SlotIndex) % SectionInfo.Width;
			const int32 Row = (SlotIndex) / SectionInfo.Width;

			const FRockInventorySlotEntry& ExistingItemSlot = Inventory->SlotData[TabOffset + SlotIndex];
			if (ExistingItemSlot.ItemHandle == IgnoreItemHandle)
			{
				continue; // Skip the item we are ignoring
			}

			const FRockItemStack& ExistingItemStack = Inventory->GetItemByHandle(ExistingItemSlot.ItemHandle);

			if (ExistingItemStack.IsValid())
			{
				const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(ExistingItemStack);
				auto SizePolicy = SectionInfo.SlotSizePolicy;
				
				// If size policy is IgnoreSize, only mark the single slot as occupied
				if (SizePolicy == ERockItemSizePolicy::IgnoreSize)
				{
					const int32 GridIndex = TabOffset + SlotIndex;
					checkf(0 <= GridIndex && GridIndex < totalGridSize,
						TEXT("Grid index out of range: %d (max: %d)"), GridIndex, totalGridSize - 1);
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
							const int32 GridIndex = TabOffset + (GridY * SectionInfo.Width + GridX);
							checkf(0 <= GridIndex && GridIndex < totalGridSize,
								TEXT("Grid index out of range: %d (max: %d)"), GridIndex, totalGridSize - 1);
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
	if (TabInfo.SlotSizePolicy == ERockItemSizePolicy::IgnoreSize)
	{
		const int32 GridIndex = TabInfo.FirstSlotIndex + (Y * TabInfo.Width + X);
		if (GridIndex < 0 || GridIndex >= OccupancyGrid.Num())
		{
			return false;
		}
		return !OccupancyGrid[GridIndex];
	}

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
		Inventory->FreeIndices.Add( Slot.ItemHandle.GetIndex() );
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
	if (!CanItemBePlacedInSection(ValidatedSourceItem, TargetInventory->SlotSections[TargetSlotHandle.GetSectionIndex()]))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Item cannot be placed in target section"));
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	/// Move
	TArray<bool> OccupancyGrid;
	PrecomputeOccupancyGrids(TargetInventory, OccupancyGrid, ValidatedSourceSlot.ItemHandle);

	const FRockInventorySectionInfo& targetSection = TargetInventory->SlotSections[TargetSlotHandle.GetSectionIndex()];
	const int32 SectionIndex = TargetSlotHandle.GetIndex() - targetSection.FirstSlotIndex;
	const int32 Column = SectionIndex % targetSection.Width;
	const int32 Row = SectionIndex / targetSection.Width;
	const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(ValidatedSourceItem);

	int32 MoveAmount = URockItemStackLibrary::CalculateMoveAmount(ValidatedSourceItem, MoveItemParams);
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
			if (MoveAmount == ValidatedSourceItem.GetStackSize())
			{
				// We don't use the Remove and Place in the same inventory because
				// those functions invalidates the itemslot and re-adds it, which is unnecessary and wasteful on same inventory
				FRockInventorySlotEntry SourceSlotRef = SourceInventory->GetSlotByHandle(SourceSlotHandle);
				SourceSlotRef.ItemHandle = FRockItemStackHandle::Invalid();
				SourceSlotRef.Orientation = ERockItemOrientation::Horizontal;
				SourceInventory->SetSlotByHandle(SourceSlotHandle, SourceSlotRef);

				FRockInventorySlotEntry TargetSlotRef = TargetInventory->GetSlotByHandle(TargetSlotHandle);
				TargetSlotRef.ItemHandle = ValidatedSourceSlot.ItemHandle;
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
			// This will broadcast the removal of the item from the source inventory
			const FRockItemStack ItemToMove = SplitItemStackAtLocation(SourceInventory, SourceSlotHandle, MoveAmount);
			if (!ItemToMove.IsValid())
			{
				UE_LOG(LogRockInventory, Warning, TEXT("Failed to split item stack"));
				return false;
			}

			// AddItemToInventory shouldn't fail
			FRockItemStackHandle ItemHandle = TargetInventory->AddItemToInventory(ItemToMove);
			FRockInventorySlotEntry newItemSlotLocation = TargetInventory->GetSlotByHandle(TargetSlotHandle);
			newItemSlotLocation.ItemHandle = ItemHandle;
			newItemSlotLocation.Orientation = MoveItemParams.DesiredOrientation;
			TargetInventory->SetSlotByHandle(TargetSlotHandle, newItemSlotLocation);
			TargetInventory->BroadcastSlotChanged(TargetSlotHandle);
			
			// Place the split item in the target inventory
			// if (!PlaceItemAtSlot_Internal(TargetInventory, TargetSlotHandle, ItemToMove, MoveItemParams.DesiredOrientation))
			// {
			// 	UE_LOG(LogRockInventory, Warning, TEXT("Failed to place item in target inventory"));
			// 	return false;
			// }
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

		const int32 TargetCurrentStack = TargetItem.GetStackSize();
		const int32 TargetMaxStack = TargetItem.GetMaxStackSize();
		const int32 SourceCurrentStack = ValidatedSourceItem.GetStackSize();

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
		TargetInventory->SetItemByHandle(ValidatedTargetSlot.ItemHandle, UpdatedTargetItem);

		// Update source item with remaining stack size
		FRockItemStack UpdatedSourceItem = ValidatedSourceItem;
		UpdatedSourceItem.StackSize = SourceCurrentStack - AmountToMove;
		
		if (UpdatedSourceItem.StackSize <= 0)
		{
			FRockInventorySlotEntry SourceSlotRef = SourceInventory->GetSlotByHandle(SourceSlotHandle);
			
			// It's removed, so we update the generation during removals.
			// When we increase generation, it basically nullifies existing stuff, but we don't need to reset the values, since it'll trigger
			// premature and unnecessary replications
			UpdatedSourceItem.Generation++;
			
			SourceInventory->FreeIndices.Add(SourceSlotRef.ItemHandle.GetIndex());
			
			// Remove the item from the source inventory
			SourceSlotRef.ItemHandle = FRockItemStackHandle::Invalid();
			SourceSlotRef.Orientation = ERockItemOrientation::Horizontal;
			SourceInventory->SetSlotByHandle(SourceSlotHandle, SourceSlotRef);
		}
		else
		{
			// There is still some stack left, so all we needed to do was update the stacksize, nothing else
		}
		SourceInventory->SetItemByHandle(ValidatedSourceSlot.ItemHandle, UpdatedSourceItem);

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

bool URockInventoryLibrary::CanItemBePlacedInSection(
	const FRockItemStack& ItemStack,
	const FRockInventorySectionInfo& SectionInfo)
{
	// Check if the section has any type restrictions
	if (SectionInfo.SectionFilter.IsEmpty())
	{
		return true;
	}
	return SectionInfo.SectionFilter.Matches(ItemStack.GetDefinition()->ItemType);
}
