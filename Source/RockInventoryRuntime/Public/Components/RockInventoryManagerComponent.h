// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Access/RockInventoryAccessSubsystem.h"
#include "Client/RockInventoryPrediction.h"
#include "Client/RockInventoryUndo.h"
#include "Inventory/RockPendingSlotOperation.h"
#include "Replication/RockInventoryReplication.h"
#include "StructUtils/InstancedStruct.h"
#include "Transactions/Implementations/RockDropItemTransaction.h"
#include "Transactions/Implementations/RockLootWorldItemTransaction.h"
#include "Transactions/Implementations/RockMoveItemTransaction.h"
#include "RockInventoryManagerComponent.generated.h"

class URockInventory;
class URockInventoryComponent;

/** One inventory's Revision after a command, in the server's answer. */
USTRUCT()
struct FRockInventoryRevision
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<URockInventory> Inventory = nullptr;
	UPROPERTY()
	int32 Revision = 0;
};

/** Input is held (or released again): too many moves wait for the server, or the oldest waits too long. A UI shows a pending indicator. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRockInputHoldChanged, bool /*bHeld*/);

namespace RockInventoryManager
{
	/** Where a group of moves comes from: the player's new action (recorded), or an undo or redo of the top history entry. */
	enum class EMoveOrigin : uint8
	{
		Player,
		Undo,
		Redo,
	};

	enum class ESendResult : uint8
	{
		Sent,
		/** The shown state refuses a move (or a command is malformed). */
		Refused,
		/** A move's preconditions do not hold on the shown state. */
		Stale,
		/** Input is held (in-flight cap or pending threshold). */
		Held,
	};
}

/** The undo history changed (recorded, undone, redone, dropped, severed or cleared). A UI refreshes its undo and redo buttons. */
DECLARE_MULTICAST_DELEGATE(FOnRockUndoHistoryChanged);
/** An undo or redo could not be done (its items moved since, or the server refused it) and its entry is gone. A UI tells the player quietly. */
DECLARE_MULTICAST_DELEGATE(FOnRockUndoEntryDropped);

// Should put this on the PlayerController?
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYRUNTIME_API URockInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URockInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// TODO: static URockInventoryManagerComponent* Get(UObject* WorldContextObject);

	/**
	 * Whether Instigator has at least the Required rights on Inventory. Every Server_* command checks this for each inventory it names.
	 * Asks URockInventoryAccessSubsystem: the inventory sits on the instigator, or the instigator opened it and is within reach (deny by default).
	 * Override for rules the registry cannot express.
	 */
	virtual bool CanAccess(const URockInventory* Inventory, const AController* Instigator, ERockInventoryRights Required = ERockInventoryRights::Full) const;

	/** Asks the server to open Inventory for this component's owning controller (a chest, a body, a dropped backpack). Refusals only log. */
	UFUNCTION(Server, Reliable)
	void Server_OpenInventory(URockInventory* Inventory);
	void Server_OpenInventory_Implementation(URockInventory* Inventory);
	UFUNCTION(Server, Reliable)
	void Server_CloseInventory(URockInventory* Inventory);
	void Server_CloseInventory_Implementation(URockInventory* Inventory);

	/** The manager component on this controller, its player state or its pawn. Null if none. */
	static URockInventoryManagerComponent* FindFor(const AController* Controller);
	/** The manager component of the first local player controller in the world (a client's own). Null if none. */
	static URockInventoryManagerComponent* FindLocal(const UWorld* World);

	/**
	 * Server: adds or removes Inventory in this player's Observed list, which replicates to the owning client only. The registry calls it when
	 * the player gains or loses View rights, so the client can tell Syncing, Live and Stale apart (URockInventory::GetSyncState).
	 */
	void ServerSetObserved(URockInventory* Inventory, bool bObserved);
	/** The grant for Inventory in the replicated list, or null. */
	const FRockObservedInventory* FindObserved(const URockInventory* Inventory) const;
	const TArray<FRockObservedInventory>& GetObserved() const { return Observed; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** The controller that owns this component's connection: the owner itself, its pawn's controller, or its player state's controller. Null if none. */
	AController* GetOwningController() const;

	/**
	 * Server-side gate for a client command: replaces the client-supplied Instigator with GetOwningController() and checks CanAccess for each inventory.
	 * Returns false (and logs) when there is no owning controller, an inventory is null, or access is refused.
	 */
	bool AuthorizeServerCommand(FRockItemTransactionBase& Command, TConstArrayView<const URockInventory*> Inventories) const;

	// Prediction (T-78) ---------------------------------------------------------------------------------------------------------
	// A client shows a move at once: the manager keeps the player's pending commands (FRockPredictionQueue), and every client model
	// applies them on top of the replicated data (ApplyPrediction), so the model is replicated data plus pending commands. The server
	// acks each command with the Revision of every inventory it touched; a command leaves the queue once the replicated state has
	// reached those revisions, and a refused command leaves at once, so the next rebuild snaps the item back.

	/** Makes the client models of this world apply this component's pending commands. Called from BeginPlay; tests call it directly. */
	void BindPredictionToModels();
	void UnbindPredictionFromModels();

	/** Lets the pending commands act on Data, the replicated state of Inventory (called by the client model on every rebuild). Settles first. */
	void ApplyPrediction(const URockInventory& Inventory, FRockInventoryData& Data);

	/** Commands sent and not yet settled. */
	int32 GetNumPendingCommands() const { return PredictionQueue.Num(); }
	const FRockPredictionQueue& GetPredictionQueue() const { return PredictionQueue; }
	/** Too many moves wait for the server, or the oldest waits longer than the threshold: MoveItem returns false until an answer arrives. */
	bool IsInputHeld() const;
	FOnRockInputHoldChanged OnInputHoldChanged;
	/** Re-evaluates IsInputHeld and fires OnInputHoldChanged on a change. The tick runs it while commands wait; the pending threshold passes with time. */
	void UpdateInputHold();

	// Undo and redo (T-80) -------------------------------------------------------------------------------------------------------
	// The client keeps the player's history; the server keeps none. Every plain move the player makes through MoveItem (move, split,
	// merge, rotate, equip and unequip, across the open inventories) is recorded with its inverse and the preconditions both ways
	// (FRockUndoHistory). Undo sends the inverse as an ordinary move, predicted and checked like any other; redo sends the original
	// again. A barrier (drop, loot, an item action) clears the history, closing a container severs it there, and closing the inventory
	// screen clears it. Entries are recorded when the command is sent; one the server refuses is dropped.

	/**
	 * Undoes the newest entry. False when there is none, input is held, a batch is open, or its items moved since (another player,
	 * the server); in that last case the entry is dropped and OnUndoEntryDropped fires.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Undo")
	bool Undo();
	/** Redoes the entry undone last. Same rules as Undo. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Undo")
	bool Redo();
	UFUNCTION(BlueprintPure, Category = "Inventory|Undo")
	bool CanUndo() const { return UndoHistory.CanUndo(); }
	UFUNCTION(BlueprintPure, Category = "Inventory|Undo")
	bool CanRedo() const { return UndoHistory.CanRedo(); }

	/** Something an inverse move cannot take back happened (a drop, a loot, an item action): undo stops here, so the history is cleared. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Undo")
	void AddUndoBarrier();
	UFUNCTION(BlueprintCallable, Category = "Inventory|Undo")
	void ClearUndoHistory();
	/**
	 * A container closed: drops the newest entry that touches it and everything older (undo cannot skip a gap), and the redo entries
	 * from the first one that touches it on. Newer entries on inventories still open stay. Inventories that follow it (a weapon's
	 * attachments, `ERockNestedVisibility::FollowsParent`) are severed with it. The UI calls it through RemoveInventoryView; the
	 * client calls it itself when the server ends a grant (closed, out of reach).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Undo")
	void SeverUndoHistory(URockInventory* Inventory);

	/**
	 * Groups the moves made until the matching EndUndoBatch into one entry (a sort, a take-all), so one undo reverts all of them.
	 * Nests; the outermost End records. A barrier or a refused command inside the batch discards it. Undo and redo wait for the end.
	 */
	void BeginUndoBatch();
	void EndUndoBatch();

	/**
	 * The UI reports the inventories it shows, once per widget (several widgets may show one inventory, one per section). While any
	 * is shown the inventory screen counts as open (undo input is live). When the last view of an inventory goes the history is
	 * severed there; when the last view of all goes the screen closed and the history is cleared.
	 */
	void AddInventoryView(const URockInventory* Inventory);
	void RemoveInventoryView(URockInventory* Inventory);
	UFUNCTION(BlueprintPure, Category = "Inventory|Undo")
	bool IsInventoryScreenOpen() const { return !InventoryViews.IsEmpty(); }

	const FRockUndoHistory& GetUndoHistory() const { return UndoHistory; }
	FOnRockUndoHistoryChanged OnUndoHistoryChanged;
	FOnRockUndoEntryDropped OnUndoEntryDropped;

protected:
	/** Whether MoveItem predicts: not on the authority (it executes at once) and not when the settings turn prediction off. Tests override it. */
	virtual bool ShouldPredict() const;
	/** Seconds, for the pending threshold. Tests override it. */
	virtual double GetPredictionTime() const;
	/** Sends a command to the server. Tests override it to hold the answer back. */
	virtual void SendMove(const FRockMoveItemTransaction& Command);
	virtual void SendDrop(const FRockDropItemTransaction& Command);
	virtual void SendLoot(const FRockLootWorldItemTransaction& Command);

private:
	/** Rebuilds the models of the given inventories (the ones whose shown state may have changed). */
	void RefreshModels(TConstArrayView<uint32> Keys, uint32 ExcludeKey = 0);
	URockInventory* FindInventory(uint32 Key) const;
	void ConfigureQueue();
	/** Removes settled commands; true if any left. */
	bool SettlePending();
	/** Abandons commands that never got confirmed (see FRockPredictionQueue::Expire) and refreshes the models they touched. */
	void ExpirePending();
	int32 NextSequence() { return ++SequenceCounter; }
	/** Server: refuses a sequence number it has already seen (a replay). 0 is unsequenced and always accepted. */
	bool AcceptSequence(int32 Sequence);
	/** Server: answers a command with the revision of every inventory it touched. */
	void SendResult(int32 Sequence, bool bSuccess, TConstArrayView<const URockInventory*> Touched);

	/**
	 * Checks Commands in order on the shown state (replicated data plus pending moves), each with the ones before it applied, and sends
	 * them all, or none if one fails: predicted when ShouldPredict, otherwise as is. A command's preconditions are taken from that state,
	 * except that an undo or redo keeps its own and must match it. Records the history for Origin before sending (an authority answers
	 * inside the send).
	 */
	RockInventoryManager::ESendResult SendMoves(TArray<FRockMoveItemTransaction>& Commands, RockInventoryManager::EMoveOrigin Origin);
	/** Undo (bUndo) or redo of the top entry. */
	bool StepHistory(bool bUndo);
	/** Records the steps of a player's action, or adds them to the open batch. A step that cannot be inverted is a barrier. */
	void RecordPlayerSteps(TArray<FRockUndoStep>&& Steps, TConstArrayView<ERockUndoStepResult> Results, TConstArrayView<int32> Sequences);
	/** The server answered a command: confirms or drops the history entry waiting for it. */
	void HandleHistoryAnswer(int32 Sequence, bool bSuccess);
	void DropUndoEntry(bool bUndo, const TCHAR* Why);
	/** Forgets inventories no entry names any more and tells the listeners. */
	void UndoHistoryChanged();
	URockInventory* FindHistoryInventory(uint32 Key) const;

	FRockPredictionQueue PredictionQueue;
	/** Inventories named by pending commands, by key; weak, kept until the queue is empty. */
	TMap<uint32, TWeakObjectPtr<URockInventory>> PredictionInventories;
	int32 SequenceCounter = 0;
	int32 LastAcceptedSequence = 0;
	bool bInputHeld = false;
	bool bBoundToModels = false;

	FRockUndoHistory UndoHistory;
	/** Inventories named by history entries, by key; weak, pruned when no entry names them. */
	TMap<uint32, TWeakObjectPtr<URockInventory>> HistoryInventories;
	/** The batch being collected (BeginUndoBatch), recorded as one entry by the outermost EndUndoBatch. */
	FRockUndoEntry OpenBatch;
	int32 BatchDepth = 0;
	bool bDiscardBatch = false;
	/** Commands sent by Undo or Redo and not answered yet: a refusal of one of them is told to the player (OnUndoEntryDropped). */
	TSet<int32> HistoryCommandSequences;
	/** Widgets showing each inventory, by key (AddInventoryView). */
	TMap<uint32, int32> InventoryViews;

	/** Inventories the server replicates to this player on top of their own (opened, nearby, shared). Owner only. */
	UPROPERTY(ReplicatedUsing = OnRep_Observed)
	TArray<FRockObservedInventory> Observed;
	UFUNCTION()
	void OnRep_Observed(const TArray<FRockObservedInventory>& OldObserved);

public:
	/**
	 * Send transaction result to client
	 *
	 * @param Sequence - The command's sequence number (TransactionID)
	 * @param bSuccess - Whether the command executed
	 * @param Touched - The Revision of each inventory the command changed, after it ran. The client keeps its prediction until the
	 *                  replicated inventories have reached these.
	 */
	UFUNCTION(Client, Reliable)
	void Client_TransactionResult(int32 Sequence, bool bSuccess, const TArray<FRockInventoryRevision>& Touched);
	void Client_TransactionResult_Implementation(int32 Sequence, bool bSuccess, const TArray<FRockInventoryRevision>& Touched);

	// Basic Inventory CRUD functions
	/** Sends a loot to the server. Not predicted; an undo barrier. */
	UFUNCTION(BlueprintCallable)
	void LootWorldItem(const FRockLootWorldItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_LootWorldItem(FRockLootWorldItemTransaction ItemTransaction);
	void Server_LootWorldItem_Implementation(FRockLootWorldItemTransaction ItemTransaction);

	// TODO: Give a 'preferred location' option, and what to do if it can't place it there (fallback to other slots or 'fail')
	// TODO: For a server to do an action like 'give players to the item' from a task reward or something.  Need a more fleshed out UX dev consumer pattern

	/**
	 * Client entry for a move. Checks it, gives it a sequence number and the preconditions, shows it at once on the client models
	 * (when ShouldPredict), records it for undo and sends it. Returns false when it was not sent: refused locally, or input is held
	 * (IsInputHeld).
	 */
	UFUNCTION(BlueprintCallable)
	bool MoveItem(const FRockMoveItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_MoveItem(FRockMoveItemTransaction ItemTransaction);
	void Server_MoveItem_Implementation(FRockMoveItemTransaction ItemTransaction);

	/** Sends a drop to the server. Not predicted; an undo barrier. */
	UFUNCTION(BlueprintCallable)
	void DropItem(const FRockDropItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_DropItem(FRockDropItemTransaction ItemTransaction);
	void Server_DropItem_Implementation(FRockDropItemTransaction ItemTransaction);

	/** The slot is claimed for this component's owning controller; the client cannot name another. */
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RegisterSlotStatus(URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus);
	void Server_RegisterSlotStatus_Implementation(URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_ReleaseSlotStatus(URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle);
	void Server_ReleaseSlotStatus_Implementation(URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle);
};
