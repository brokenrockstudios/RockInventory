// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryLibrary.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockItemStackLibrary.h"


bool URockInventoryLibrary::AddItemToInventory(
	URockInventory* Inventory, const FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess)
{
	OutExcess = 0;
	OutHandle = FRockInventorySlotHandle();
	if (!Inventory || !ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("AddItemToInventory: Invalid Inventory or ItemStack"));
		return false;
	}
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid ItemStack"));
		return false;
	}
	const FVector2D ItemSize = ItemStack.Definition->SlotDimensions;
	if (ItemSize.X <= 0 || ItemSize.Y <= 0)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Item Size"));
		return false;
	}

	// Create temporary occupancy grids for each tab
	TArray<bool> OccupancyGrids;
	PrecomputeOccupancyGrids(Inventory, OccupancyGrids);

	for (int32 SectionIndex = 0; SectionIndex < Inventory->SlotSections.Num(); ++SectionIndex)
	{
		const FRockInventorySectionInfo& SectionInfo = Inventory->SlotSections[SectionIndex];
		for (int32 SlotIndex = 0; SlotIndex < SectionInfo.GetNumSlots(); ++SlotIndex)
		{
			const int32 Column = (SlotIndex) % SectionInfo.Width;
			const int32 Row = (SlotIndex) / SectionInfo.Width;
			const int32 AbsoluteIndex = SectionInfo.FirstSlotIndex + SlotIndex;

			if (CanItemFitInGridPosition(OccupancyGrids, SectionInfo, Column, Row, ItemSize))
			{
				const FRockItemStackHandle& ItemHandle = Inventory->AddItemToInventory(ItemStack);
				OutHandle = FRockInventorySlotHandle(SectionIndex, AbsoluteIndex);
				PlaceItemAtLocation(Inventory, OutHandle, ItemHandle, ERockItemOrientation::Horizontal);
				return true;
			}
		}
	}
	return false;
}


void URockInventoryLibrary::PrecomputeOccupancyGrids(const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid)
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
	for (int32 ItemY = 0; ItemY < ItemSize.Y; ++ItemY)
	{
		for (int32 ItemX = 0; ItemX < ItemSize.X; ++ItemX)
		{
			const int32 GridIndex = TabInfo.FirstSlotIndex + ((Y + ItemY) * TabInfo.Width + (X + ItemX));
			checkf(0 <= GridIndex && GridIndex < OccupancyGrid.Num(),
				TEXT("Grid index out of range: %d (max: %d)"), GridIndex, OccupancyGrid.Num() - 1);
			if (OccupancyGrid[GridIndex])
			{
				return false; // Cell is occupied
			}
		}
	}
	return true;
}

bool URockInventoryLibrary::PlaceItemAtLocation(
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
		FRockInventorySlotEntry& Slot = Inventory->GetSlotRefByHandle(SlotHandle);
		
		Slot.ItemHandle = ItemStackHandle;
		Slot.Orientation = DesiredOrientation;
		Inventory->ItemData.MarkItemDirty(Slot);
		Inventory->BroadcastInventoryChanged(SlotHandle);
		return true;
	}

	UE_LOG(LogRockInventory, Warning, TEXT("PlaceItemAtLocation Failed"));
	return false;
}

bool URockInventoryLibrary::GetItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, FRockItemStack& OutItemStack)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemAtLocation: Invalid Inventory"));
		OutItemStack = FRockItemStack();
		return false;
	}
	OutItemStack = Inventory->GetItemBySlotHandle(SlotHandle);
	return OutItemStack.IsValid();
}

bool URockInventoryLibrary::RemoveItemAtLocation(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return false;
	}

	const int32 slotIndex = SlotHandle.GetIndex();
	if (slotIndex < 0 || slotIndex >= Inventory->SlotData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid SlotHandle: %s"), *SlotHandle.ToString());
		return false;
	}
	FRockInventorySlotEntry& Slot = Inventory->GetSlotRefByHandle(SlotHandle);
	Slot.ItemHandle = FRockItemStackHandle::Invalid();
	// When/how should we drop the item?
	// Inventory->ItemData[slotIndex].Reset();

	Inventory->BroadcastInventoryChanged(SlotHandle);
	return true;
}


bool URockInventoryLibrary::MoveItem(
	URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
	URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
	ERockItemOrientation DesiredOrientation)
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
	const FRockItemStack& TargetItem = TargetInventory->GetItemBySlotHandle(TargetSlotHandle);
	if (TargetItem.IsOccupied())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Target Slot is not empty. We don't support stacking or swapping yet"));
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	/// Move

	TArray<bool> OccupancyGrid;
	// item size . URockItemStackLibrary::GetItemSize(SourceSlot.Item)
	const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(SourceItem);
	PrecomputeOccupancyGrids(TargetInventory, OccupancyGrid);

	const FRockInventorySectionInfo& targetSection = TargetInventory->SlotSections[TargetSlotHandle.GetSectionIndex()];
	const int32 SectionIndex = TargetSlotHandle.GetIndex() - targetSection.FirstSlotIndex;
	const int32 Column = SectionIndex % targetSection.Width;
	const int32 Row = SectionIndex / targetSection.Width;

	if (CanItemFitInGridPosition(OccupancyGrid, targetSection, Column, Row, ItemSize)
	)
	{
		if (RemoveItemAtLocation(SourceInventory, SourceSlotHandle))
		{
			FRockInventorySlotHandle OutHandle;
			if (PlaceItemAtLocation(TargetInventory, TargetSlotHandle, SourceSlot.ItemHandle, DesiredOrientation))
			{
				// If these are the same inventory, should we handle this differently?
				// Technically we probably want a different 'event' for how this was handled?
				SourceInventory->BroadcastInventoryChanged(SourceSlotHandle);
				TargetInventory->BroadcastInventoryChanged(TargetSlotHandle);
				return true;
			}
		}
		UE_LOG(LogRockInventory, Warning, TEXT("Failed to place item at target location"));
	}


	//////////////////////////////////////////////////////////////////////
	/// TODO:Merge Item

	//////////////////////////////////////////////////////////////////////
	/// TODO: Swap Item


	return true;
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
			ItemCount += ItemStack.StackSize;
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
