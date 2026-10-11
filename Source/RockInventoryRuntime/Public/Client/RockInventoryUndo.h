// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Client/RockInventoryPrediction.h"

/** One move in one direction, as plain data: what to send, and what the two slots must hold for it to be the move the player means. */
struct ROCKINVENTORYRUNTIME_API FRockUndoMove
{
	uint32 SourceKey = 0;
	FRockInventorySlotHandle SourceSlot;
	uint32 TargetKey = 0;
	FRockInventorySlotHandle TargetSlot;
	FRockMoveItemParams Params;
	/** Unset (bCheck false) on a move the player just made: its preconditions come from the state it is made on. */
	FRockSlotExpectation ExpectedSource;
	FRockSlotExpectation ExpectedTarget;

	bool Touches(uint32 Key) const { return SourceKey == Key || TargetKey == Key; }
};

/** What `FRockUndoStep::Make` did with a move. */
enum class ERockUndoStepResult : uint8
{
	/** Applied; the step holds the move and its inverse. */
	Undoable,
	/** Applied, but it changed nothing (same slot, same orientation): nothing to undo. */
	NoChange,
	/** Applied, but its inverse would not put the two slots back (a merged-away stack of an instanced item cannot be split off): a barrier. */
	NotInvertible,
	/** Not applied: a slot does not hold what the move expects (an undo or redo whose items moved since). */
	Stale,
	/** Not applied: the data refuses the move. */
	Refused,
};

/** A move the player made and its inverse, each with the preconditions that make it safe to send later. */
struct ROCKINVENTORYRUNTIME_API FRockUndoStep
{
	FRockUndoMove Forward;
	FRockUndoMove Inverse;

	/**
	 * Applies Move to State (the shown state; every inventory the move names must be in it) and fills OutStep: Forward is Move with
	 * the preconditions taken before it, Inverse moves the units that left the source back from the target slot to the source slot
	 * in the source's old orientation (a rotation in place rotates back), with the preconditions taken after it. Expectations already
	 * set on Move must match State first (Stale otherwise). The inverse is tried on a copy and must restore what both slots held.
	 * State changes for Undoable, NoChange and NotInvertible, never for Stale and Refused. OutRefusal says why a move was Refused.
	 */
	static ERockUndoStepResult Make(FRockPredictionState& State, const FRockUndoMove& Move, FRockUndoStep& OutStep, ERockMoveRefusal* OutRefusal = nullptr);
};

/** One undo: a single move, or every move of a batch (sort, take-all), undone and redone together. */
struct ROCKINVENTORYRUNTIME_API FRockUndoEntry
{
	/** In the order they were made: an undo sends the inverses newest first, a redo the forwards oldest first. */
	TArray<FRockUndoStep> Steps;
	/** Commands sent for this entry that the server has not answered yet. A refusal of one of them drops the entry. */
	TArray<int32> PendingSequences;

	bool Touches(uint32 Key) const;
	/** The moves an undo sends (the inverses, newest first) or a redo sends (the forwards, oldest first). */
	void GetMoves(bool bUndo, TArray<FRockUndoMove>& OutMoves) const;
};

/**
 * The client's undo history of one player (T-80). Plain data keyed by inventory (the same keys as `FRockPredictionQueue`), no
 * UObject, no world; `URockInventoryManagerComponent` drives it. The server keeps no history: an undo is an ordinary move command.
 *
 * Two stacks, each with its top at the end of the array: undo (newest entry last) and redo (the next entry to redo last).
 *  - `Record` pushes a new entry, clears redo and keeps at most MaxEntries (the oldest go first; 0 turns undo off).
 *  - `MarkUndone` / `MarkRedone` move the top entry to the other stack once its commands are sent.
 *  - `Sever(Key)` (a container closed): on each stack, the entry nearest the top that touches the inventory goes, with everything
 *    beyond it, since undo or redo cannot skip a gap. Entries above it, on inventories still open, stay.
 *  - `DropBySequence` (the server refused a command of an entry) and `DropTop` (an entry's items moved since) drop that entry only;
 *    the others keep their own preconditions.
 *  - `Clear`: a barrier (drop, loot, item action) or the inventory screen closed.
 */
class ROCKINVENTORYRUNTIME_API FRockUndoHistory
{
public:
	int32 MaxEntries = 25;

	bool CanUndo() const { return !UndoStack.IsEmpty(); }
	bool CanRedo() const { return !RedoStack.IsEmpty(); }
	bool IsEmpty() const { return UndoStack.IsEmpty() && RedoStack.IsEmpty(); }
	/** Oldest first. */
	const TArray<FRockUndoEntry>& GetUndoEntries() const { return UndoStack; }
	/** The next to redo last. */
	const TArray<FRockUndoEntry>& GetRedoEntries() const { return RedoStack; }
	const FRockUndoEntry* PeekUndo() const { return UndoStack.IsEmpty() ? nullptr : &UndoStack.Last(); }
	const FRockUndoEntry* PeekRedo() const { return RedoStack.IsEmpty() ? nullptr : &RedoStack.Last(); }

	/** A new player action. An entry without steps is ignored. */
	void Record(FRockUndoEntry Entry);
	/** The newest entry was undone with the commands Sequences: it moves to redo. */
	void MarkUndone(TConstArrayView<int32> Sequences);
	/** The next redo was sent with the commands Sequences: it moves back to undo. */
	void MarkRedone(TConstArrayView<int32> Sequences);
	/** Drops the top entry of the undo stack (bUndo) or of the redo stack. */
	void DropTop(bool bUndo);

	/** The server accepted a command: no entry waits for it any more. */
	void Confirm(int32 Sequence);
	/** The server refused a command: the entry waiting for it goes. True if there was one. */
	bool DropBySequence(int32 Sequence);

	/** Cuts both stacks at the entry nearest the top that touches Key (see the class comment). Returns how many entries went. */
	int32 Sever(uint32 Key);
	void Clear();

	/** Every inventory key an entry names, once each. */
	void GetKeys(TArray<uint32>& OutKeys) const;

private:
	void Trim();

	TArray<FRockUndoEntry> UndoStack;
	TArray<FRockUndoEntry> RedoStack;
};
