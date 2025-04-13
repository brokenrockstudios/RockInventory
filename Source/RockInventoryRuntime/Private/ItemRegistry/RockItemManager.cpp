// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "ItemRegistry/RockItemManager.h"

FRockItemStackHandle FRockItemManager::CreateItem(URockItemDefinition* ItemDefinition, int32 StackSize)
{
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Error, TEXT("Attempted to create item with null definition"));
		return FRockItemStackHandle::Invalid();
	}

	uint32 Index = INDEX_NONE;

	// Use a slot from the free list if available
	if (FreeIndices.Num() > 0)
	{
		Index = FreeIndices.Pop(EAllowShrinking::No);
	}
	// Otherwise, create a new slot
	else
	{
		Index = Items.AddDefaulted();
		Items[Index].Generation = 0;
	}

	// Set up the item
	Items[Index].Item = FRockItemStack(ItemDefinition, StackSize);
	Items[Index].bIsOccupied = true;

	// TODO: Mark the item as dirty for replication
	

	// Return handle with current index and generation
	return FRockItemStackHandle::Create(Index, Items[Index].Generation);
}

FRockItemStack FRockItemManager::GetItemStack(const FRockItemStackHandle& ItemId, int32 StackSize)
{
	if (!ValidateHandle(ItemId))
	{
		return FRockItemStack();
	}

	return Items[ItemId.GetIndex()].Item;
}

bool FRockItemManager::RemoveItem(const FRockItemStackHandle& ItemId)
{
	if (!ValidateHandle(ItemId))
	{
		return false;
	}

	const uint32 Index = ItemId.GetIndex();
	
	auto& Slot = Items[Index];
	// Reset the item
	// This could cause unnecessary replication churn
	Slot.Item.Reset();

	// Mark the slot as free
	Slot.bIsOccupied = false;

	// Add the index to the free list
	FreeIndices.Add(Index);

	// Increment the generation for the next item
	Slot.Generation++;

	// TODO: Mark the specific entry as dirty for replication
	// FFastSerializer -> MarkArrayEntryAsDirty(Slot);

	return true;
}

bool FRockItemManager::IsValidItem(const FRockItemStackHandle& ItemId) const
{
	return ValidateHandle(ItemId);
}

int32 FRockItemManager::GetActiveItemCount() const
{
	return Items.Num() - FreeIndices.Num();
}

int32 FRockItemManager::GetTotalCapacity() const
{
	return Items.Num();
}

void FRockItemManager::Clear()
{
	Items.Empty();
	FreeIndices.Empty();
}

bool FRockItemManager::ValidateHandle(const FRockItemStackHandle& ItemId) const
{
	if (!ItemId.IsValid())
	{
		return false;
	}

	const uint32 Index = ItemId.GetIndex();
	if (!Items.IsValidIndex(Index))
	{
		return false;
	}

	const FItemSlot& Slot = Items[Index];
	if (!Slot.bIsOccupied)
	{
		return false;
	}

	if (Slot.Generation != ItemId.GetGeneration())
	{
		return false;
	}

	return true;
}
