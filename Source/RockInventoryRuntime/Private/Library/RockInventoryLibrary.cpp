// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryLibrary.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockItemStackLibrary.h"


bool URockInventoryLibrary::LootItemToInventory(
	URockInventory* Inventory, const FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess)
{
	// Start off with the full stack size in the event we can't place it
	OutExcess = ItemStack.GetStackSize();
	UE_LOG(LogRockInventory, Verbose, TEXT("LootItemToInventory::ItemStack: %s"), *ItemStack.GetDebugString());
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("LootItemToInventory::Invalid Parameters. Inventory"));
		return false;
	}
	if ( !ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("LootItemToInventory::Invalid Parameters. ItemStack"));
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

		// Then check if this slot is pending an operation. We don't want to overwrite any pending operations
		const FRockPendingSlotOperation TargetPendingSlot = Inventory->GetPendingSlotState(Slot.SlotHandle);
		if (TargetPendingSlot.SlotStatus == ERockSlotStatus::Pending)
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

TArray<FString> URockInventoryLibrary::GetInventoryContentsDebug(const URockInventory* Inventory)
{
	if (!Inventory)
	{
		return {TEXT("Invalid Inventory")};
	}

	TArray<FString> InventoryContents;
	for (const FRockInventorySlotEntry& Slot : Inventory->SlotData)
	{
		const FRockItemStack& ItemStack = Inventory->GetItemByHandle(Slot.ItemHandle);
		FString LineItem = FString::Printf(
			TEXT("Section:[%d] SlotIdx:[%d]; ItemIdx:[%s], Item:[%s] Count:[%d]"),
			Slot.SlotHandle.GetSectionIndex(), Slot.SlotHandle.GetIndex(), *Slot.ItemHandle.ToString(),
			ItemStack.GetDefinition() ? *ItemStack.GetDefinition()->Name.ToString() : TEXT("None"),
			ItemStack.GetStackSize());

		InventoryContents.Add(LineItem);
	}

	for (const FRockItemStack& ItemStack : Inventory->ItemData)
	{
		FString LineItem = FString::Printf(
			TEXT("ItemIdx:[%s], Item:[%s] Count:[%d]"),
			*ItemStack.ItemHandle.ToString(),
			ItemStack.GetDefinition() ? *ItemStack.GetDefinition()->Name.ToString() : TEXT("None"),
			ItemStack.GetStackSize());

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


FRockItemStack URockInventoryLibrary::GetItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemAtLocation: Invalid Inventory"));
		return FRockItemStack::Invalid();
	}
	return Inventory->GetItemBySlotHandle(SlotHandle);
}

FRockItemStack URockInventoryLibrary::SplitItemStackAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, int32 Quantity)
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

	FRockInventorySlotEntry SourceSlot = Inventory->GetSlotByHandle(SlotHandle);
	FRockItemStack Item = Inventory->GetItemByHandle(SourceSlot.ItemHandle);

	if (!Item.IsValid())
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
		Item.StackSize = CurrentStackSize - Quantity;
		checkf(Item.StackSize > 0, TEXT("ItemStack size is 0 or negative. Should have used full stack move path"));
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

	const FRockInventorySectionInfo& targetSection = TargetInventory->SlotSections[TargetSlotHandle.GetSectionIndex()];
	const int32 SectionIndex = TargetSlotHandle.GetIndex() - targetSection.FirstSlotIndex;
	const int32 Column = SectionIndex % targetSection.Width;
	const int32 Row = SectionIndex / targetSection.Width;
	const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(ValidatedSourceItem);

	if (CanItemFitInGridPosition(OccupancyGrid, targetSection, Column, Row, ItemSize))
	{
		FRockInventorySlotEntry targetSlot = ValidatedTargetSlot;
		const bool isFullStackMove = (MoveAmount == ValidatedSourceItem.GetStackSize());
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
				// Add to target
				targetSlot.ItemHandle = TargetInventory->AddItemToInventory(ValidatedSourceItem);
				// Release from source
				SourceInventory->RemoveItemFromInventory(ValidatedSourceItem);
			}

			// Set up target slot with existing item handle
			targetSlot.Orientation = InMoveParams.DesiredOrientation;
			TargetInventory->SetSlotByHandle(TargetSlotHandle, targetSlot);
		}
		else
		{
			// Partial move

			// Split the source item stack based on the move amount
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

		const int32 targetCurrentStack = TargetItem.GetStackSize();
		const int32 targetMaxStack = TargetItem.GetMaxStackSize();
		const int32 sourceCurrentStack = ValidatedSourceItem.GetStackSize();

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
		UpdatedTargetItem.StackSize = targetCurrentStack + amountToMove;
		checkf(UpdatedTargetItem.StackSize <= targetMaxStack,
			TEXT("Updated target item stack size exceeds max: %d > %d"), UpdatedTargetItem.StackSize, targetMaxStack);
		TargetInventory->SetItemByHandle(ValidatedTargetSlot.ItemHandle, UpdatedTargetItem);

		// Update source item with remaining stack size
		FRockItemStack UpdatedSourceItem = ValidatedSourceItem;
		UpdatedSourceItem.StackSize = sourceCurrentStack - amountToMove;
		checkf(UpdatedSourceItem.StackSize >= 0,
			TEXT("Updated source item stack size is negative: %d"), UpdatedSourceItem.StackSize);

		const bool isSourceEmptied = (UpdatedSourceItem.GetStackSize() <= 0);

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
	if (!ExistingItemStack.IsValid())
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
