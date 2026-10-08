// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockInventoryData.h"
#include "Transactions/Core/RockSlotExpectation.h"

/** The data of every inventory a prediction touches, by inventory key (`URockInventory::GetUniqueID()` in game code, any number in tests). */
using FRockPredictionState = TMap<uint32, FRockInventoryData>;

/** One inventory's Revision, as acked by the server. */
using FRockKeyedRevision = TPair<uint32, int32>;

/**
 * A move the client has predicted and sent (T-78). Plain data: no UObject inventory, no world.
 * The command is the player's intent plus the preconditions (what the slots held when it was predicted); the rest is bookkeeping
 * for the queue.
 */
struct ROCKINVENTORYRUNTIME_API FRockPredictedMove
{
	/** Per-player sequence number, also the transaction id the server acks. */
	int32 Sequence = 0;
	uint32 SourceKey = 0;
	FRockInventorySlotHandle SourceSlot;
	uint32 TargetKey = 0;
	FRockInventorySlotHandle TargetSlot;
	FRockMoveItemParams Params;
	/** Filled by `FRockPredictionQueue::Predict` from the predicted state the move was made on. */
	FRockSlotExpectation ExpectedSource;
	FRockSlotExpectation ExpectedTarget;

	/** When it was sent (seconds). */
	double SentTime = 0.0;
	/** The server answered. An acked move stays in the queue until the replicated state includes it (see Settle). */
	bool bAcked = false;
	/** When the ack arrived (seconds); for the abandon timeout of an acked move that never settles. */
	double AckTime = 0.0;
	/** For an acked move: the revision each touched inventory has to reach before the replicated state includes the move. */
	TArray<FRockKeyedRevision> AckedRevisions;
};

/**
 * The client's pending commands for one player, and the rule that turns replicated data into the state to show:
 * model = replicated data + pending commands re-applied in order, with the same `ApplyMove` the server runs.
 *
 *  - `Predict` checks a new move on the shown state, captures its preconditions, applies it and queues it.
 *  - `Rebase` runs on every replicated update: it applies the pending moves in order on fresh replicated data. A move whose
 *    preconditions no longer hold (the replicated data already includes it, or someone else changed the slot) or that the data
 *    refuses is skipped for that update and never half-applied.
 *  - `Ack` marks a move answered; a refused move leaves the queue at once, so the next rebase shows the item back where it was.
 *  - `Settle` drops an acked move once every inventory it touched has replicated up to the revision the ack named: the confirmation
 *    is tied to the state that contains it, so the replicated state, not the ack, ends the prediction.
 *
 * In-flight cap and pending threshold: `CanPredict` is false once `MaxInFlight` moves wait for an answer, or the oldest waits
 * longer than `PendingThresholdSeconds` (`IsHolding`); the owner then stops predicting, shows a pending indicator and holds input.
 */
class ROCKINVENTORYRUNTIME_API FRockPredictionQueue
{
public:
	int32 MaxInFlight = 4;
	double PendingThresholdSeconds = 0.5;
	/** A move still in the queue this long after it was sent (unanswered) or acked (never settled) is abandoned by Expire. */
	double AbandonSeconds = 5.0;

	bool IsEmpty() const { return Pending.IsEmpty(); }
	int32 Num() const { return Pending.Num(); }
	/** Moves sent and not answered yet. */
	int32 NumInFlight() const;
	const TArray<FRockPredictedMove>& GetPending() const { return Pending; }
	const FRockPredictedMove* Find(int32 Sequence) const;

	/** The oldest unanswered move has waited longer than the threshold. */
	bool IsHolding(double Now) const;
	/** Room for another move: under the in-flight cap and not holding. */
	bool CanPredict(double Now) const;

	/**
	 * Checks the move on State (the state being shown, pending moves already applied), captures the preconditions into Move, applies
	 * it to State and queues it. On refusal nothing changes. Every inventory the move names has to be in State.
	 */
	ERockMoveRefusal Predict(FRockPredictionState& State, FRockPredictedMove Move, double Now);

	/** The server answered. A refusal removes the move; an acceptance keeps it until Settle, with the revisions the server reached. */
	void Ack(int32 Sequence, bool bSuccess, TConstArrayView<FRockKeyedRevision> Revisions, double Now = 0.0);

	/**
	 * Removes acked moves whose inventories all reached the acked revision. IsReached(Key, Revision) says whether the replicated
	 * inventory is at or past that revision. Returns how many were removed.
	 */
	int32 Settle(const TFunctionRef<bool(uint32 Key, int32 Revision)>& IsReached);

	/**
	 * Recovery: drops every move that has waited longer than AbandonSeconds, unanswered or acked but never settled (the answer was lost,
	 * or the inventory stopped replicating to this player). The shown state then falls back to the replicated data. Returns how many.
	 */
	int32 Expire(double Now);

	/** Applies the pending moves, oldest first, to State (fresh replicated data for the touched inventories). Returns how many were skipped. */
	int32 Rebase(FRockPredictionState& State) const;

	/** Every inventory key a pending move names, once each. */
	void GetTouchedKeys(TArray<uint32>& OutKeys) const;

	void Reset() { Pending.Reset(); }

	/** Wrap-aware revision comparison (Revision is a uint32 that wraps; the wire carries it as int32). */
	static bool RevisionReached(int32 Current, int32 Required) { return static_cast<int32>(static_cast<uint32>(Current) - static_cast<uint32>(Required)) >= 0; }

private:
	static bool ApplyOne(FRockPredictionState& State, const FRockPredictedMove& Move);

	TArray<FRockPredictedMove> Pending;
};
