// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemStackHandle.h"
#include "UObject/Object.h"
#include "RockItemManager.generated.h"

/**
 * Manages the lifecycle and storage of item stacks in the inventory system.
 * This class provides a centralized way to create, retrieve, and remove item stacks,
 * with built-in support for object pooling and generation-based handle validation.
 * 
 * The manager maintains a pool of item slots that can be reused, with each slot
 * having a generation counter to prevent handle reuse issues. This design allows
 * for efficient memory usage while maintaining handle stability.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemManager
{
	GENERATED_BODY()

public:
	/**
	 * Creates a new item stack from the given item definition.
	 * @param ItemDefinition - The definition of the item to create
	 * @param StackSize - The initial size of the stack (defaults to 1)
	 * @return A handle to the newly created item stack
	 */
	FRockItemStackHandle CreateItem(URockItemDefinition* ItemDefinition, int32 StackSize = 1);

	/**
	 * Retrieves an item stack using its handle.
	 * @param ItemId - The handle of the item stack to retrieve
	 * @param StackSize - The desired stack size (defaults to 1)
	 * @return The item stack if found, or an empty stack if invalid
	 */
	FRockItemStack GetItemStack(const FRockItemStackHandle& ItemId, int32 StackSize = 1);

	/**
	 * Removes an item stack from the manager.
	 * @param ItemId - The handle of the item stack to remove
	 * @return true if the item was successfully removed, false otherwise
	 */
	bool RemoveItem(const FRockItemStackHandle& ItemId);

	/**
	 * Checks if an item stack exists and is valid.
	 * @param ItemId - The handle to check
	 * @return true if the item exists and is valid, false otherwise
	 */
	bool IsValidItem(const FRockItemStackHandle& ItemId) const;

	/**
	 * Gets the current number of active item stacks.
	 * @return The number of active item stacks
	 */
	int32 GetActiveItemCount() const;

	/**
	 * Gets the total capacity of the item manager (active + free slots).
	 * @return The total capacity
	 */
	int32 GetTotalCapacity() const;

	/**
	 * Clears all item stacks and resets the manager to its initial state.
	 */
	void Clear();

private:
	/** Internal structure representing a slot in the item manager */
	struct FItemSlot
	{
		FRockItemStack Item;
		uint8 Generation = 0;
		bool bIsOccupied = false;
	};
    
	/** Array of item slots, both active and free */
	TArray<FItemSlot> Items;
	
	/** Stack of available slot indices for reuse */
	TArray<uint32> FreeIndices;

	/** Validates that a handle is valid and points to an occupied slot */
	bool ValidateHandle(const FRockItemStackHandle& ItemId) const;
};

