// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventorySectionInfo.h"
#include "RockInventorySlot.h"
#include "RockSlotHandle.h"
#include "Enums/RockItemOrientation.h"
#include "Item/RockItemStack.h"
#include "Item/RockItemStackHandle.h"
#include "Item/RockMoveItemParams.h"

class URockInventory;

/** Which of the two inventories of a move a change belongs to. Both are the same data when a move stays inside one inventory. */
enum class ERockInventorySide : uint8
{
	Source,
	Target,
};

/** Why a move is refused. None means it is allowed. */
enum class ERockMoveRefusal : uint8
{
	None,
	InvalidSourceSlot,
	EmptySource,
	InvalidTargetSlot,
	/** The target section's filter rejects the item. */
	SectionRejectsItem,
	/** The move mode and count resolve to zero items. */
	InvalidAmount,
	/** A part of a stack whose item needs a runtime instance cannot be split off yet. */
	PartialMoveOfInstancedItem,
	/** The target holds a stack of the same item that is already full. */
	NothingToMerge,
	/** The footprint does not fit at the target and the target stack cannot take the items. */
	NoRoom,
	/** Same slot, other orientation, and the rotated footprint does not fit. */
	NoRoomToRotate,
};

/** Why an add is refused. None means it is allowed. */
enum class ERockAddRefusal : uint8
{
	None,
	/** The stack has no definition or no items. */
	InvalidStack,
	InvalidSlot,
	/** The section's filter rejects the item. */
	SectionRejectsItem,
	/** The slot holds a stack of the same item that is already full. */
	NothingToMerge,
	/** The footprint does not fit at the slot and its stack cannot take the items. */
	NoRoom,
	/** Only from URockInventoryLibrary::AddItemToSlot: no inventory, or it is not owned by an actor with authority. */
	NotAllowed,
};

/** Why a remove is refused. None means it is allowed. */
enum class ERockRemoveRefusal : uint8
{
	None,
	InvalidSlot,
	EmptySlot,
	/** More items asked for than the stack holds. */
	NotEnoughItems,
};

enum class ERockDataChangeType : uint8
{
	/** A slot got another item handle, orientation or lock state. */
	Slot,
	/** A stack appeared (its handle is the one it has in the data that was changed). */
	StackCreated,
	/** A stack was released. */
	StackRemoved,
	/** A stack kept its handle and changed (count). */
	StackModified,
};

/** One before/after pair. Only the fields of its Type are meaningful. */
struct ROCKINVENTORYRUNTIME_API FRockInventoryChange
{
	ERockDataChangeType Type = ERockDataChangeType::Slot;
	ERockInventorySide Side = ERockInventorySide::Source;

	FRockInventorySlotHandle Slot;
	FRockInventorySlotEntry SlotBefore;
	FRockInventorySlotEntry SlotAfter;

	FRockItemStackHandle Stack;
	/** Empty for StackCreated. Not GC-visible: the definition and instance are kept alive by the data it came from. */
	FRockItemStack StackBefore;
	/** Empty for StackRemoved. */
	FRockItemStack StackAfter;
};

/**
 * What one operation did, in the order it has to be committed to keep the events of the old code
 * (clear the source slot, release or shrink the source stack, create the target stack, set the target slot).
 * Transient: build it, read it, throw it away.
 */
struct ROCKINVENTORYRUNTIME_API FRockInventoryChangeSet
{
	TArray<FRockInventoryChange> Changes;

	bool IsEmpty() const { return Changes.IsEmpty(); }
	const FRockInventoryChange* FindSlotChange(ERockInventorySide Side, FRockInventorySlotHandle Slot) const;
	const FRockInventoryChange* FindStackChange(ERockInventorySide Side, FRockItemStackHandle Stack) const;
};

/**
 * Layer 0 of the inventory: sections, slots and stacks as plain data. No UObject inventory, no events, no authority, no world,
 * so the same rules run on the server, on a client's prediction model and in a test. (Item definitions are still UObjects.)
 *
 * `CanMove` answers without touching anything; `ApplyMove` changes this data and reports the changes. URockInventory is
 * Layer 1: `URockInventoryLibrary::MoveItem` snapshots the inventories into this, applies, and commits the change set.
 */
struct ROCKINVENTORYRUNTIME_API FRockInventoryData
{
	TArray<FRockInventorySectionInfo> Sections;
	/** Indexed by absolute slot index. */
	TArray<FRockInventorySlotEntry> Slots;
	/** Indexed by the index part of an item handle. An entry that is not IsValid() is free. */
	TArray<FRockItemStack> Stacks;

	/** Copies the layout, slots and stacks of a live inventory. */
	static FRockInventoryData FromInventory(const URockInventory* Inventory);

	/** Lays out empty slots for the given sections, in order (same rules as URockInventory::Init). */
	void Init(const TArray<FRockInventorySectionInfo>& InSections);

	/** The slot, or null for an out-of-range or uninitialized handle. */
	const FRockInventorySlotEntry* GetSlot(const FRockInventorySlotHandle& SlotHandle) const;
	/** The stack, or null for an invalid, out-of-range, stale or empty handle. */
	const FRockItemStack* GetStack(const FRockItemStackHandle& StackHandle) const;
	/** The stack at the slot (its anchor cell), or null. */
	const FRockItemStack* GetSlotStack(const FRockInventorySlotHandle& SlotHandle) const;
	const FRockInventorySectionInfo* FindSection(const FRockInventorySlotHandle& SlotHandle) const;

	/** Stores a copy of Stack in the slot, bypassing placement rules. For arranging a scenario. Returns the new stack's handle. */
	FRockItemStackHandle PlaceStack(const FRockItemStack& Stack, const FRockInventorySlotHandle& SlotHandle, ERockItemOrientation Orientation = ERockItemOrientation::Horizontal);

	/** One flag per slot: covered by a stack's footprint (a single cell in an IgnoreSize section). The stack with IgnoreHandle does not count. */
	TArray<bool> BuildOccupancy(const FRockItemStackHandle& IgnoreHandle = FRockItemStackHandle::Invalid()) const;

	/**
	 * The one occupancy implementation: BuildOccupancy and URockInventoryLibrary::PrecomputeOccupancyGrids both use it. GetStack maps a
	 * handle to its stack (null for none), so a live inventory can be read without copying it.
	 */
	static void FillOccupancy(
		const TArray<FRockInventorySectionInfo>& InSections, const TArray<FRockInventorySlotEntry>& InSlots,
		const TFunctionRef<const FRockItemStack*(const FRockItemStackHandle&)>& GetStack,
		const FRockItemStackHandle& IgnoreHandle, TArray<bool>& OutOccupancy);

	/** Marks the cells a stack of Size would cover from (Column, Row) in the section (a single cell in an IgnoreSize section). Cells outside the grid are skipped. */
	static void MarkFootprint(TArray<bool>& InOutOccupancy, const FRockInventorySectionInfo& Section, int32 Column, int32 Row, FIntPoint Size);

	/**
	 * Would the move be allowed? Moving a slot onto itself with the same orientation is allowed and changes nothing; with another
	 * orientation it rotates in place. A different slot moves, merges or splits depending on what is there.
	 * Source and Target may be the same object (a move inside one inventory).
	 */
	static ERockMoveRefusal CanMove(
		const FRockInventoryData& Source, const FRockInventorySlotHandle& SourceSlot,
		const FRockInventoryData& Target, const FRockInventorySlotHandle& TargetSlot,
		const FRockMoveItemParams& Params);

	/** Does the move. On refusal nothing is changed and OutChanges is empty. */
	static ERockMoveRefusal ApplyMove(
		FRockInventoryData& Source, const FRockInventorySlotHandle& SourceSlot,
		FRockInventoryData& Target, const FRockInventorySlotHandle& TargetSlot,
		const FRockMoveItemParams& Params, FRockInventoryChangeSet& OutChanges);

	/**
	 * Would adding Stack at Slot be allowed? An empty cell takes a new stack when the footprint fits; the anchor of a stack that
	 * stacks with Stack takes what fits up to the max. At most MaxStackCount items are added to a new stack.
	 * Pending slot operations are a URockInventory concern and not seen here.
	 */
	ERockAddRefusal CanAdd(const FRockInventorySlotHandle& Slot, const FRockItemStack& Stack, ERockItemOrientation Orientation = ERockItemOrientation::Horizontal) const;

	/**
	 * Adds Stack at Slot (see CanAdd). OutAdded is how many items went in (the rest of Stack is the caller's excess). On refusal nothing changes.
	 * Changes are on the Target side. A new stack keeps Stack's bInitialized flag: the instance and OnItemCreated hooks run when
	 * the change set is committed to a URockInventory, not here, so a stack created here may still gain custom values at commit.
	 */
	ERockAddRefusal ApplyAdd(
		const FRockInventorySlotHandle& Slot, const FRockItemStack& Stack, ERockItemOrientation Orientation,
		FRockInventoryChangeSet& OutChanges, int32& OutAdded);

	/** Would removing Count items from the stack at Slot be allowed? Count <= 0 means the whole stack. */
	ERockRemoveRefusal CanRemove(const FRockInventorySlotHandle& Slot, int32 Count = 0) const;

	/**
	 * Removes Count items from the stack at Slot (Count <= 0: the whole stack). A stack that reaches zero is released and its slot cleared.
	 * Changes are on the Source side. On refusal nothing changes.
	 */
	ERockRemoveRefusal ApplyRemove(const FRockInventorySlotHandle& Slot, int32 Count, FRockInventoryChangeSet& OutChanges);

	/** Units across every stack the predicate accepts. */
	int32 CountMatching(const TFunctionRef<bool(const FRockItemStack&)>& Matches) const;

	/**
	 * Removes up to Count units (> 0) from the stacks the predicate accepts, visiting slots in index order, so the lowest slot is emptied first.
	 * With bAllOrNothing nothing changes unless Count units exist. Returns how many were removed. Changes are on the Source side.
	 */
	int32 RemoveMatching(const TFunctionRef<bool(const FRockItemStack&)>& Matches, int32 Count, bool bAllOrNothing, FRockInventoryChangeSet& OutChanges);

private:
	/** Shared by CanAdd and ApplyAdd. Amount is what would be added. */
	ERockAddRefusal PlanAdd(const FRockInventorySlotHandle& Slot, const FRockItemStack& Stack, ERockItemOrientation Orientation, bool& bOutMerge, int32& OutAmount) const;
	FRockItemStackHandle AllocateStack(const FRockItemStack& Copy);
	void FreeStack(const FRockItemStackHandle& StackHandle);

	TArray<int32> FreeStackIndices;
};
