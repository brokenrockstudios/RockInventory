// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

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
	AddItem, // instantiate a wholly new item from nowhere. Such as a quest reward
	LootItem, // Get an item from a source that isn't necessarily another container? 
	MoveItem, // Move an item from any inventory to any other inventory.
	DropItem, // Drop an item from an inventory to the world

	EquipItem, // Equip an item from the inventory to a slot
	UnequipItem, // Unequip an item from a slot to the inventory
	// Alt: Activate/Deactivate Item?

	UseItem, // Use an item from the inventory. This could be a consumable or something else.
	// Consume? Inspect? 
	// Destroy an item

	Max UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ERockItemMoveMode : uint8
{
	FullStack,
	HalfStack, // Move Half Stack (Rounded up)
	SingleItem,
	CustomAmount, // Use MoveCount for a specific amount
};
