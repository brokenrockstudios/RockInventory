// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryLibrary.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockItemStackLibrary.h"


bool URockInventoryLibrary::AddItemToInventory(
	URockInventory* Inventory, const FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess)
{
	OutExcess = 0;
	OutHandle = FRockInventorySlotHandle();
	if (!Inventory || !ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory or ItemStack"));
		return false;
	}
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid ItemStack"));
		return false;
	}

	const FVector2D ItemSize = ItemStack.GetDefinition()->SlotDimensions;
	if (ItemSize.X <= 0 || ItemSize.Y <= 0)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Item Size"));
		return false;
	}

	// Create temporary occupancy grids for each tab
	TArray<bool> OccupancyGrids;
	PrecomputeOccupancyGrids(Inventory, OccupancyGrids);

	for (int32 TabIndex = 0; TabIndex < Inventory->Tabs.Num(); ++TabIndex)
	{
		const FRockInventoryTabInfo& TabInfo = Inventory->Tabs[TabIndex];
		const int32 TabOffset = TabInfo.FirstSlotIndex;

		for (int32 Y = 0; Y <= TabInfo.Height - ItemSize.Y; ++Y)
		{
			for (int32 X = 0; X <= TabInfo.Width - ItemSize.X; ++X)
			{
				if (CanItemFitInGridPosition(OccupancyGrids, TabInfo, X, Y, ItemSize))
				{
					OutHandle = FRockInventorySlotHandle(TabIndex, X, Y);
					PlaceItemAtLocation(Inventory, OutHandle, ItemStack, ERockItemOrientation::Horizontal);
					return true;
				}
			}
		}
	}
	return false;
}


void URockInventoryLibrary::PrecomputeOccupancyGrids(const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid)
{
	const int32 totalGridSize = Inventory->InventoryData.Num();
	OutOccupancyGrid.SetNum(totalGridSize);
	// Initialize all cells to false (unoccupied)
	OutOccupancyGrid.Init(false, totalGridSize);

	for (int32 TabIndex = 0; TabIndex < Inventory->Tabs.Num(); ++TabIndex)
	{
		const FRockInventoryTabInfo& TabInfo = Inventory->Tabs[TabIndex];
		const int32 TabOffset = TabInfo.FirstSlotIndex;

		// Mark occupied cells
		for (int32 SlotIndex = TabOffset; SlotIndex < TabOffset + TabInfo.NumSlots; ++SlotIndex)
		{
			checkf(0 <= SlotIndex && SlotIndex < Inventory->InventoryData.Num(),
				TEXT("SlotIndex is out of range: %d (max: %d)"), SlotIndex, Inventory->InventoryData.Num() - 1);

			const FRockInventorySlotEntry& ExistingItem = Inventory->InventoryData[SlotIndex];
			if (ExistingItem.Item.IsValid())
			{
				const FVector2D ItemSize = URockItemStackLibrary::GetItemSize(ExistingItem.Item);

				// Mark all cells this item occupies as true
				for (int32 Y = 0; Y < ItemSize.Y; ++Y)
				{
					for (int32 X = 0; X < ItemSize.X; ++X)
					{
						const int32 GridX = ExistingItem.SlotHandle.X + X;
						const int32 GridY = ExistingItem.SlotHandle.Y + Y;
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
	const TArray<bool>& OccupancyGrid, const FRockInventoryTabInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize)
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
	URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, const FRockItemStack& ItemStack, ERockItemOrientation DesiredOrientation)
{
	if (!Inventory || !ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory or ItemStack"));
		return false;
	}
	if (SlotHandle.IsValid())
	{
		const int32 slotIndex = Inventory->GetSlotIndex(SlotHandle.TabIndex, SlotHandle.X, SlotHandle.Y);
		if (slotIndex < 0 || slotIndex >= Inventory->InventoryData.Num())
		{
			UE_LOG(LogRockInventory, Warning, TEXT("Invalid SlotHandle: %s"), *SlotHandle.ToString());
			return false;
		}
		
		Inventory->InventoryData[slotIndex].Item = ItemStack;
		Inventory->InventoryData[slotIndex].Orientation = DesiredOrientation;
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

	const int32 slotIndex = Inventory->GetSlotIndex(SlotHandle.TabIndex, SlotHandle.X, SlotHandle.Y);
	if (slotIndex < 0 || slotIndex >= Inventory->InventoryData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemAtLocation: Invalid SlotHandle: %s"), *SlotHandle.ToString());
		OutItemStack = FRockItemStack();
		return false;
	}

	OutItemStack = Inventory->InventoryData[slotIndex].Item;
	return true;
}

bool URockInventoryLibrary::RemoveItemAtLocation(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return false;
	}

	const int32 slotIndex = Inventory->GetSlotIndex(SlotHandle.TabIndex, SlotHandle.X, SlotHandle.Y);

	if (slotIndex < 0 || slotIndex >= Inventory->InventoryData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid SlotHandle: %s"), *SlotHandle.ToString());
		return false;
	}

	Inventory->InventoryData[slotIndex].Reset();
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

	// Check TargetInventory if slot is empty
	const FRockInventorySlotEntry& TargetSlot = TargetInventory->GetSlotByHandle(TargetSlotHandle);
	if (!TargetSlot.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Target Slot Handle"));
		return false;
	}

	if (TargetSlot.Item.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Target Slot is not empty. We don't support stacking or swapping yet"));
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	/// Move

	TArray<bool> OccupancyGrid;
	PrecomputeOccupancyGrids(TargetInventory, OccupancyGrid);
	if (CanItemFitInGridPosition(OccupancyGrid, TargetInventory->Tabs[TargetSlotHandle.TabIndex], TargetSlotHandle.X, TargetSlotHandle.Y,
			URockItemStackLibrary::GetItemSize(SourceSlot.Item))
	)
	{
		// Make a copy, since we will reset the SourceSlot version
		FRockItemStack itemToMove = SourceSlot.Item;

		if (RemoveItemAtLocation(SourceInventory, SourceSlotHandle))
		{
			FRockInventorySlotHandle OutHandle;
			if (PlaceItemAtLocation(TargetInventory, TargetSlotHandle, itemToMove, DesiredOrientation))
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
	for (const FRockInventorySlotEntry& Slot : Inventory->InventoryData)
	{
		if (Slot.Item.IsValid() && Slot.Item.GetItemId() == ItemId)
		{
			ItemCount += Slot.Item.StackSize;
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
