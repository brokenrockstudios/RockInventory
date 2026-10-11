// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Client/RockInventoryUndo.h"

namespace
{
	/** Removes the entry nearest the top (the end) that touches Key and everything below it. Returns how many went. */
	int32 CutAt(TArray<FRockUndoEntry>& Stack, uint32 Key)
	{
		for (int32 Index = Stack.Num() - 1; Index >= 0; --Index)
		{
			if (Stack[Index].Touches(Key))
			{
				Stack.RemoveAt(0, Index + 1);
				return Index + 1;
			}
		}
		return 0;
	}

	void AddKeys(const TArray<FRockUndoEntry>& Stack, TArray<uint32>& OutKeys)
	{
		for (const FRockUndoEntry& Entry : Stack)
		{
			for (const FRockUndoStep& Step : Entry.Steps)
			{
				OutKeys.AddUnique(Step.Forward.SourceKey);
				OutKeys.AddUnique(Step.Forward.TargetKey);
			}
		}
	}
}

ERockUndoStepResult FRockUndoStep::Make(FRockPredictionState& State, const FRockUndoMove& Move, FRockUndoStep& OutStep, ERockMoveRefusal* OutRefusal)
{
	if (OutRefusal)
	{
		*OutRefusal = ERockMoveRefusal::None;
	}
	FRockInventoryData* Source = State.Find(Move.SourceKey);
	FRockInventoryData* Target = State.Find(Move.TargetKey);
	if (!Source || !Target)
	{
		if (OutRefusal)
		{
			*OutRefusal = ERockMoveRefusal::InvalidSourceSlot;
		}
		return ERockUndoStepResult::Refused;
	}
	if (!Move.ExpectedSource.Matches(*Source, Move.SourceSlot) || !Move.ExpectedTarget.Matches(*Target, Move.TargetSlot))
	{
		return ERockUndoStepResult::Stale;
	}
	const ERockMoveRefusal Refusal = FRockInventoryData::CanMove(*Source, Move.SourceSlot, *Target, Move.TargetSlot, Move.Params);
	if (Refusal != ERockMoveRefusal::None)
	{
		if (OutRefusal)
		{
			*OutRefusal = Refusal;
		}
		return ERockUndoStepResult::Refused;
	}

	OutStep.Forward = Move;
	OutStep.Forward.ExpectedSource = FRockSlotExpectation::Capture(*Source, Move.SourceSlot);
	OutStep.Forward.ExpectedTarget = FRockSlotExpectation::Capture(*Target, Move.TargetSlot);
	const FRockItemStack* SourceStack = Source->GetSlotStack(Move.SourceSlot);
	const int32 CountBefore = SourceStack ? SourceStack->GetStackCount() : 0;
	const FRockInventorySlotEntry* SourceEntry = Source->GetSlot(Move.SourceSlot);
	const ERockItemOrientation OrientationBefore = SourceEntry ? SourceEntry->Orientation : ERockItemOrientation::Horizontal;

	FRockInventoryChangeSet Changes;
	FRockInventoryData::ApplyMove(*Source, Move.SourceSlot, *Target, Move.TargetSlot, Move.Params, Changes);
	if (Changes.IsEmpty())
	{
		return ERockUndoStepResult::NoChange;
	}

	FRockUndoMove& Inverse = OutStep.Inverse;
	Inverse.SourceKey = Move.TargetKey;
	Inverse.SourceSlot = Move.TargetSlot;
	Inverse.TargetKey = Move.SourceKey;
	Inverse.TargetSlot = Move.SourceSlot;
	Inverse.Params = FRockMoveItemParams();
	Inverse.Params.DesiredOrientation = OrientationBefore;
	if (Move.SourceKey != Move.TargetKey || Move.SourceSlot != Move.TargetSlot)
	{
		// Exactly the units that left the source: back into its stack (a split, a partial merge) or into its empty slot
		const FRockItemStack* Left = Source->GetSlotStack(Move.SourceSlot);
		Inverse.Params.MoveMode = ERockItemMoveMode::CustomAmount;
		Inverse.Params.MoveCount = CountBefore - (Left ? Left->GetStackCount() : 0);
	}
	Inverse.ExpectedSource = FRockSlotExpectation::Capture(*Target, Move.TargetSlot);
	Inverse.ExpectedTarget = FRockSlotExpectation::Capture(*Source, Move.SourceSlot);

	// An inverse that does not restore both slots would undo something else: try it on a copy of the two inventories.
	FRockPredictionState Check;
	Check.Add(Move.SourceKey, *Source);
	Check.Add(Move.TargetKey, *Target);
	FRockInventoryData& CheckSource = Check.FindChecked(Inverse.SourceKey);
	FRockInventoryData& CheckTarget = Check.FindChecked(Inverse.TargetKey);
	FRockInventoryChangeSet Ignored;
	if (FRockInventoryData::ApplyMove(CheckSource, Inverse.SourceSlot, CheckTarget, Inverse.TargetSlot, Inverse.Params, Ignored) != ERockMoveRefusal::None
		|| !OutStep.Forward.ExpectedSource.Matches(Check.FindChecked(Move.SourceKey), Move.SourceSlot)
		|| !OutStep.Forward.ExpectedTarget.Matches(Check.FindChecked(Move.TargetKey), Move.TargetSlot))
	{
		return ERockUndoStepResult::NotInvertible;
	}
	return ERockUndoStepResult::Undoable;
}

bool FRockUndoEntry::Touches(uint32 Key) const
{
	return Steps.ContainsByPredicate([Key](const FRockUndoStep& Step) { return Step.Forward.Touches(Key); });
}

void FRockUndoEntry::GetMoves(bool bUndo, TArray<FRockUndoMove>& OutMoves) const
{
	OutMoves.Reset(Steps.Num());
	if (bUndo)
	{
		for (int32 Index = Steps.Num() - 1; Index >= 0; --Index)
		{
			OutMoves.Add(Steps[Index].Inverse);
		}
	}
	else
	{
		for (const FRockUndoStep& Step : Steps)
		{
			OutMoves.Add(Step.Forward);
		}
	}
}

void FRockUndoHistory::Record(FRockUndoEntry Entry)
{
	if (Entry.Steps.IsEmpty())
	{
		return;
	}
	RedoStack.Reset();
	UndoStack.Add(MoveTemp(Entry));
	Trim();
}

void FRockUndoHistory::Trim()
{
	const int32 Excess = UndoStack.Num() - FMath::Max(MaxEntries, 0);
	if (Excess > 0)
	{
		UndoStack.RemoveAt(0, Excess);
	}
}

void FRockUndoHistory::MarkUndone(TConstArrayView<int32> Sequences)
{
	if (UndoStack.IsEmpty())
	{
		return;
	}
	FRockUndoEntry Entry = UndoStack.Pop();
	// Keep the older ones: a move still unanswered when it is undone can be refused yet, and then its undo is meaningless too
	Entry.PendingSequences.Append(Sequences.GetData(), Sequences.Num());
	RedoStack.Add(MoveTemp(Entry));
}

void FRockUndoHistory::MarkRedone(TConstArrayView<int32> Sequences)
{
	if (RedoStack.IsEmpty())
	{
		return;
	}
	FRockUndoEntry Entry = RedoStack.Pop();
	Entry.PendingSequences.Append(Sequences.GetData(), Sequences.Num());
	UndoStack.Add(MoveTemp(Entry));
	Trim();
}

void FRockUndoHistory::DropTop(bool bUndo)
{
	TArray<FRockUndoEntry>& Stack = bUndo ? UndoStack : RedoStack;
	if (!Stack.IsEmpty())
	{
		Stack.Pop();
	}
}

void FRockUndoHistory::Confirm(int32 Sequence)
{
	for (FRockUndoEntry& Entry : UndoStack)
	{
		Entry.PendingSequences.Remove(Sequence);
	}
	for (FRockUndoEntry& Entry : RedoStack)
	{
		Entry.PendingSequences.Remove(Sequence);
	}
}

bool FRockUndoHistory::DropBySequence(int32 Sequence)
{
	const auto Waits = [Sequence](const FRockUndoEntry& Entry) { return Entry.PendingSequences.Contains(Sequence); };
	return UndoStack.RemoveAll(Waits) + RedoStack.RemoveAll(Waits) > 0;
}

int32 FRockUndoHistory::Sever(uint32 Key)
{
	return CutAt(UndoStack, Key) + CutAt(RedoStack, Key);
}

void FRockUndoHistory::Clear()
{
	UndoStack.Reset();
	RedoStack.Reset();
}

void FRockUndoHistory::GetKeys(TArray<uint32>& OutKeys) const
{
	AddKeys(UndoStack, OutKeys);
	AddKeys(RedoStack, OutKeys);
}
