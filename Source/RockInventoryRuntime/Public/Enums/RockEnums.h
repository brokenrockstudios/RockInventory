// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

/**
 * 
 */

// TODO: Split these into their own files

UENUM(BlueprintType)
enum class ERockInventoryChangeType : uint8
{
	None,
	ItemAdded,
	ItemRemoved,
	ItemMoved,
	ItemUpdated,

	TabAdded,
	TabRemoved,
	TabUpdated,

	SlotAdded,
	SlotRemoved,
	SlotUpdated
};

/**
 * 
 */
UENUM(BlueprintType)
enum class ERockItemOrientation : uint8
{
	Horizontal = 0,
	Vertical = 1,
};

// an enum to help determine if the stack can be fully merged, partially, or not at all
UENUM(BlueprintType)
enum class ERockItemStackMergeCondition : uint8
{
	// Will only return if it can be fully merged  
	Full = 0,
	// Will only return true if it can only be partially merged
	Partial = 1,
	// will return false if it cannot be merged at all
	None = 2,
};



// Maybe we want to support asynchronous transactions?
UENUM(BlueprintType)
enum class ERockTransactionState : uint8
{
	Pending,
	Executing,
	Completed,
	Failed,
	Undoing
};

UENUM(BlueprintType)
enum class ERockTransactionResult : uint8
{
	InProgress UMETA(DisplayName = "In Progress", ToolTip = "Transaction is still executing"),
	Complete UMETA(DisplayName = "Complete", ToolTip = "Transaction has completed successfully"),
	Failed UMETA(DisplayName = "Failed", ToolTip = "Transaction has failed"),
};


UENUM(BlueprintType)
enum class ERockInventoryTransactionType : uint8
{
	None,
	AddItem, // instantiate a wholly new item from nowhere
	LootItem, // Get an item from a source that isn't necessarily another container? 
	MoveItem, // Move an item from any inventory to any other inventory.

	// Destroy or remove or consume item?
	// Modify an item somehow?

	// ItemUpdated,

	// TabAdded,
	// TabRemoved,
	// TabUpdated,

	// SlotAdded,
	// SlotRemoved,
	// SlotUpdated

	Max UMETA(Hidden)
};


UENUM(BlueprintType)
enum class ERockItemMoveAmount : uint8
{
	All,	// Move Entire Stack
	Half,	// Move Half Stack (Rounded up)
	One,	// Move Single item
	Custom, // Use MoveCount for a specific amount
};
