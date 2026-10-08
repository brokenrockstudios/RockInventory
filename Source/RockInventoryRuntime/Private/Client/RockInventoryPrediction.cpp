// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Client/RockInventoryPrediction.h"

int32 FRockPredictionQueue::NumInFlight() const
{
	int32 Count = 0;
	for (const FRockPredictedMove& Move : Pending)
	{
		Count += Move.bAcked ? 0 : 1;
	}
	return Count;
}

const FRockPredictedMove* FRockPredictionQueue::Find(int32 Sequence) const
{
	return Pending.FindByPredicate([Sequence](const FRockPredictedMove& Move) { return Move.Sequence == Sequence; });
}

bool FRockPredictionQueue::IsHolding(double Now) const
{
	for (const FRockPredictedMove& Move : Pending)
	{
		if (!Move.bAcked)
		{
			// Pending is in send order, so the first unanswered move is the oldest
			return Now - Move.SentTime > PendingThresholdSeconds;
		}
	}
	return false;
}

bool FRockPredictionQueue::CanPredict(double Now) const
{
	return NumInFlight() < MaxInFlight && !IsHolding(Now);
}

bool FRockPredictionQueue::ApplyOne(FRockPredictionState& State, const FRockPredictedMove& Move)
{
	FRockInventoryData* Source = State.Find(Move.SourceKey);
	FRockInventoryData* Target = State.Find(Move.TargetKey);
	if (!Source || !Target)
	{
		return false;
	}
	if (!Move.ExpectedSource.Matches(*Source, Move.SourceSlot) || !Move.ExpectedTarget.Matches(*Target, Move.TargetSlot))
	{
		return false;
	}
	FRockInventoryChangeSet Changes;
	return FRockInventoryData::ApplyMove(*Source, Move.SourceSlot, *Target, Move.TargetSlot, Move.Params, Changes) == ERockMoveRefusal::None;
}

ERockMoveRefusal FRockPredictionQueue::Predict(FRockPredictionState& State, FRockPredictedMove Move, double Now)
{
	FRockInventoryData* Source = State.Find(Move.SourceKey);
	FRockInventoryData* Target = State.Find(Move.TargetKey);
	if (!Source || !Target)
	{
		return ERockMoveRefusal::InvalidSourceSlot;
	}
	const ERockMoveRefusal Refusal = FRockInventoryData::CanMove(*Source, Move.SourceSlot, *Target, Move.TargetSlot, Move.Params);
	if (Refusal != ERockMoveRefusal::None)
	{
		return Refusal;
	}
	Move.ExpectedSource = FRockSlotExpectation::Capture(*Source, Move.SourceSlot);
	Move.ExpectedTarget = FRockSlotExpectation::Capture(*Target, Move.TargetSlot);
	Move.SentTime = Now;
	Move.bAcked = false;
	Move.AckedRevisions.Reset();

	FRockInventoryChangeSet Changes;
	const ERockMoveRefusal Applied = FRockInventoryData::ApplyMove(*Source, Move.SourceSlot, *Target, Move.TargetSlot, Move.Params, Changes);
	if (Applied != ERockMoveRefusal::None)
	{
		return Applied;
	}
	Pending.Add(MoveTemp(Move));
	return ERockMoveRefusal::None;
}

void FRockPredictionQueue::Ack(int32 Sequence, bool bSuccess, TConstArrayView<FRockKeyedRevision> Revisions, double Now)
{
	const int32 Index = Pending.IndexOfByPredicate([Sequence](const FRockPredictedMove& Move) { return Move.Sequence == Sequence; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	if (!bSuccess)
	{
		Pending.RemoveAt(Index);
		return;
	}
	Pending[Index].bAcked = true;
	Pending[Index].AckTime = Now;
	Pending[Index].AckedRevisions = Revisions;
}

int32 FRockPredictionQueue::Settle(const TFunctionRef<bool(uint32 Key, int32 Revision)>& IsReached)
{
	return Pending.RemoveAll([&IsReached](const FRockPredictedMove& Move)
	{
		if (!Move.bAcked)
		{
			return false;
		}
		for (const FRockKeyedRevision& Entry : Move.AckedRevisions)
		{
			if (!IsReached(Entry.Key, Entry.Value))
			{
				return false;
			}
		}
		return true;
	});
}

int32 FRockPredictionQueue::Expire(double Now)
{
	return Pending.RemoveAll([this, Now](const FRockPredictedMove& Move)
	{
		return Now - (Move.bAcked ? Move.AckTime : Move.SentTime) > AbandonSeconds;
	});
}

int32 FRockPredictionQueue::Rebase(FRockPredictionState& State) const
{
	int32 Skipped = 0;
	for (const FRockPredictedMove& Move : Pending)
	{
		Skipped += ApplyOne(State, Move) ? 0 : 1;
	}
	return Skipped;
}

void FRockPredictionQueue::GetTouchedKeys(TArray<uint32>& OutKeys) const
{
	for (const FRockPredictedMove& Move : Pending)
	{
		OutKeys.AddUnique(Move.SourceKey);
		OutKeys.AddUnique(Move.TargetKey);
	}
}
