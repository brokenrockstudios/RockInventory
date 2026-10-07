// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InventoryReferenceHelper.h"
#include "RockInventoryConfig.h"
#include "RockInventoryQuery.h"
#include "RockInventorySlot.h"
#include "RockPendingSlotOperation.h"
#include "RockSlotHandle.h"
#include "Events/RockInventoryChangeBatch.h"
#include "Events/RockItemDelta.h"
#include "Events/RockSlotChangeType.h"
#include "Events/RockSlotDelta.h"
#include "Item/RockItemStack.h"
#include "Replication/RockInventoryReplication.h"
#include "UObject/Object.h"

#include "RockInventory.generated.h"

/**
 * Delegate that is broadcast when the inventory changes.
 * @param Inventory - The inventory that changed
 * @param SlotHandle - The handle of the slot that was modified
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotChanged, const FRockSlotDelta&, SlotDelta);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemStackChanged, const FRockItemDelta&, ItemDelta);

/** One call per finished operation (server) or per replication update (client). See FRockInventoryChangeBatch. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChangeBatch, const FRockInventoryChangeBatch&, ChangeBatch);

/** Client only: what this client knows about the inventory changed (T-76). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventorySyncStateChanged, URockInventory&, ERockInventorySyncState);

/**
 * The root class for the Rock Inventory System.
 * 
 * This class manages a variable-sized grid inventory system with support for multiple tabs.
 * Each tab represents a collection of slots (e.g., a chest rig might have 4 tabs, each with 2x1 slots).
 * 
 * Features:
 * - Multi-tab inventory system
 * - Variable-sized items
 * - Network replication support
 */
UCLASS(Blueprintable, BlueprintType)
class ROCKINVENTORYRUNTIME_API URockInventory : public UObject
{
	GENERATED_BODY()
public:
	URockInventory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
private:
	/** The item data */
	UPROPERTY(VisibleAnywhere, Replicated)
	FRockInventoryItemContainer ItemData;

	/** Stack of available slot indices for reuse */
	UPROPERTY()
	TArray<uint32> FreeIndices;

	/** The grid slot data */
	UPROPERTY(VisibleAnywhere, Replicated)
	FRockInventorySlotContainer SlotData;

	/** Tab configuration for the inventory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, meta = (AllowPrivateAccess = true))
	TArray<FRockInventorySectionInfo> SlotSections;

	/** Pending slot operations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_PendingSlotOperations, meta = (AllowPrivateAccess = true))
	TArray<FRockPendingSlotOperation> PendingSlotOperations;
	UFUNCTION()
	void OnRep_PendingSlotOperations();

	/** Snapshot of the previous replication state; diffed in OnRep to detect added/removed pending operations. */
	UPROPERTY()
	TArray<FRockPendingSlotOperation> PreviousPendingSlotOperations;
	/** Rises by one per server operation that changed anything. Replicated with the data, so it names the state the data is in. */
	UPROPERTY(Replicated)
	uint32 Revision = 0;

	/** Changes recorded by the operation in progress (server) or the replication update being applied (client). */
	FRockInventoryChangeBatch PendingBatch;
	/** Nesting depth of FRockInventoryOperationScope. The batch is flushed when the outermost scope ends. */
	int32 OperationDepth = 0;
	/** Replicated array callbacks recorded changes; PostNetReceive flushes them once every array has been applied. */
	bool bAwaitingNetReceive = false;

	ERockInventorySyncState CachedSyncState = ERockInventorySyncState::Unknown;
	/** A grant for this inventory was seen on this client at some point: without one now, the state is Stale rather than Unknown. */
	bool bEverObserved = false;

	/** Item index -> absolute index of the slot holding that item (INDEX_NONE when none). Derived from SlotData. */
	mutable TArray<int32> ItemSlotIndex;
	mutable bool bItemSlotIndexDirty = true;

	void RebuildItemSlotIndex() const;
	void UpdateItemSlotIndex(int32 SlotIndex, const FRockItemStackHandle& OldHandle, const FRockItemStackHandle& NewHandle) const;
	/** Broadcasts the pending batch (Revision rises first when bBumpRevision) and replays it through the legacy delegates. */
	void FlushPendingChanges(bool bBumpRevision);

public:
	/** Broadcast once per finished operation with everything it changed: one move is one batch holding both slots. */
	UPROPERTY(BlueprintAssignable, Category = "Rock|Inventory")
	FOnInventoryChangeBatch OnChangeBatch;

	/**
	 * Legacy per-delta delegates, kept as adapters: they are replayed from the change batch, in the order the changes were made,
	 * when the operation is complete (server) or the replication update has been applied (client).
	 * Broadcast when a slot's state changes (item assigned, removed, etc).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Rock|Inventory")
	FOnInventorySlotChanged OnSlotChanged;

	/** Legacy adapter, see OnSlotChanged. Broadcast when an item stack's data changes (count, customValue, etc). */
	UPROPERTY(BlueprintAssignable, Category = "Rock|Inventory")
	FOnInventoryItemStackChanged OnItemChanged;

	uint32 GetRevision() const { return Revision; }

	/**
	 * Nested inventories only: FollowsParent makes the viewers of the inventory holding the item this inventory's viewers (weapon attachments);
	 * Separate is a container of its own that must be opened (backpack contents). Copied from URockInventoryConfig::Visibility by Init.
	 */
	UPROPERTY(Replicated)
	ERockNestedVisibility NestedVisibility = ERockNestedVisibility::Separate;

	/** The inventory whose viewers, access and Observed entry apply to this one: itself, or the first ancestor that does not follow its parent. */
	const URockInventory* GetGatingRoot() const;
	URockInventory* GetGatingRoot() { return const_cast<URockInventory*>(static_cast<const URockInventory*>(this)->GetGatingRoot()); }

	/** Calls Func for every object that replicates with this inventory: the inventory itself, then the item instances in its stacks, and for a nested inventory that follows its parent (bFollowsParentViewers) the same again, recursively. Other nested inventories are their own unit and are not visited. */
	void ForEachGatedObject(const TFunctionRef<void(UObject*)>& Func);

	/**
	 * What this machine knows about the inventory's contents (T-76). The server and a standalone game are always Live. A client is Live for
	 * an inventory on its own player, Unknown for one it was never granted, Syncing from the grant until the replicated Revision reaches the
	 * grant's, Live after that, and Stale once the grant ended (the copy stops updating but is not destroyed).
	 */
	ERockInventorySyncState GetSyncState() const;
	/** Recomputes the sync state and broadcasts OnSyncStateChanged when it differs from the last one. Called when the grant list or the replicated state changes. */
	void RefreshSyncState();
	/** RefreshSyncState on this inventory and the nested ones that follow it. */
	void RefreshSyncStateWithFollowers();
	FOnInventorySyncStateChanged OnSyncStateChanged;

	/**
	 * Groups the changes made until the matching EndOperation into one batch. Nests; only the outermost end flushes.
	 * Use FRockInventoryOperationScope. Every URockInventory mutator opens one itself, so a call that is not inside a scope is
	 * its own operation.
	 */
	void BeginOperation() { ++OperationDepth; }
	void EndOperation();

	/** Called by the replicated arrays' callbacks on a client: record the change, flush it in PostNetReceive. */
	void QueueReplicatedSlotDelta(const FRockSlotDelta& SlotDelta);
	void QueueReplicatedItemDelta(const FRockItemStackHandle& ItemStackHandle, ERockItemChangeType ChangeType);
	/** Called by the slot array's callbacks: keeps the item-to-slot index right for a slot that was added or changed. */
	void OnReplicatedSlotEntry(int32 SlotIndex, const FRockItemStackHandle& PreviousItemHandle);
	/** Called when the slot array shrinks or is replaced; the index is rebuilt on the next lookup. */
	void InvalidateItemSlotIndex() { bItemSlotIndexDirty = true; }

	virtual void PostNetReceive() override;
	/** Points the slot and item arrays at this inventory, so a client copy that arrives as a plain replicated subobject routes its array callbacks here. */
	virtual void PostInitProperties() override;

	/* The owner of this inventory, most likely the InventoryComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TObjectPtr<UObject> Owner;

	UObject* GetOwner() const { return Owner; }

	/**
	 * Sets the replication-owner link that GetTopLevelOwner follows to find the actor (not the Outer).
	 * A nested inventory's owner is the inventory holding its item, so the item instance updates it when the item moves.
	 * C++ only: Blueprint already gets a setter for the Owner property.
	 */
	void SetOwner(UObject* InOwner) { Owner = InOwner; }

	/** Walks the ownership chain until it finds an Actor */
	AActor* GetOwningActor();

	/** Iterates slots; return false from Func to break early. */
	void ForEachSlotInSection(const TFunctionRef<bool(const FRockInventorySectionInfo&, const FRockInventorySlotEntry&)>& Func) const;

	/** Iterates item stacks; return false from Func to break early. */
	void ForEachItemStack(const TFunctionRef<bool(const FRockItemStack&)>& Func) const;


	/** Sets up slots and sections from the given config. Must be called before use. */
	void Init(const URockInventoryConfig* config);

	/** Returns section info by SectionTag, or an empty struct if not found. */
	const FRockInventorySectionInfo& GetSectionInfo(const FGameplayTag& SectionTag) const;
	const FRockInventorySectionInfo& GetSectionInfoBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const;

	/** Returns the index of the section with the given SectionTag, or INDEX_NONE if not found. */
	int32 GetSectionIndex(const FGameplayTag& SectionTag) const;


	/** Returns the slot entry for the given handle, or a default entry if the handle is invalid. */
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	FRockInventorySlotEntry GetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle) const;
	const FRockInventorySlotEntry& GetSlotByAbsoluteIndex(int32 AbsoluteIndex) const;

	/** The slot holding the item, through the item-to-slot index: O(1). Empty/null for an invalid or stale handle and for an item that is in no slot. */
	FRockInventorySlotEntry GetSlotByItemHandle(const FRockItemStackHandle& InItemHandle) const;
	const FRockInventorySlotEntry* GetSlotByItemHandlePtr(const FRockItemStackHandle& InItemHandle) const;

	/* Get item stack by handle */
	FRockItemStack GetItemBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const;
	FRockItemStack GetItemByHandle(const FRockItemStackHandle& InItemHandle) const;
	const FRockItemStack* GetItemByHandlePtr(const FRockItemStackHandle& InItemHandle) const;

	/** Overwrites the item stack at the given handle and broadcasts OnItemChanged. Handle must be valid. */
	void SetItemByHandle(const FRockItemStackHandle& InSlotHandle, const FRockItemStack& InItemStack);

	/** Overwrites the slot entry at the given handle and broadcasts OnSlotChanged. Handle must be valid. */
	void SetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle, const FRockInventorySlotEntry& InSlotEntry);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;

#if UE_WITH_IRIS
	/** Register all replication fragments */
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	// IsNameStableForNetworking false?
	void RegisterReplicationWithOwner();
	void UnregisterReplicationWithOwner();

	/** Broadcast the inventory changed event */
	void BroadcastSlotChanged(const FRockSlotDelta& SlotDelta);
	void BroadcastItemChanged(const FRockItemStackHandle& ItemStackHandle, ERockItemChangeType ChangeType);

	//////////////////////////////////////////////////////////////////////////
	/// Slot Status Management
	// Slots can be "pending" to prevent concurrent modifications (e.g. locking a slot mid-drag
	// so another player can't move the item before the operation completes).

	/** Locks the slot to the given status, associating it with the instigating controller. */
	void RegisterSlotStatus(AController* Instigator, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus);

	/** Releases the lock on the slot. No-ops if the instigator doesn't own the lock. */
	void ReleaseSlotStatus(AController* Instigator, const FRockInventorySlotHandle& InSlotHandle);

	/** Returns the current status of the slot (e.g. Empty, Pending). */
	UFUNCTION(BlueprintCallable)
	ERockSlotStatus GetSlotStatus(const FRockInventorySlotHandle& InSlotHandle) const;

	/** Returns the full pending operation state for the slot, if any. */
	UFUNCTION(BlueprintCallable)
	FRockPendingSlotOperation GetPendingSlotState(const FRockInventorySlotHandle& InSlotHandle) const;

	/////////////////////////////////////////////////////////////////

	/** Get a debug string representation of the inventory */
	FString GetDebugString() const;

	// Need to add a 'preference' on where it goes.
	// Does the caller have a preference or does the inventory itself have a preference
	FRockItemStackHandle AddItemToInventory(const FRockItemStack& InItemStack);


	void RemoveItemFromInventory(const FRockItemStackHandle& InItemStackHandle);
	void RemoveItemFromInventory(const FRockItemStack& InItemStack);
	// TODO: Instead of just SetItem, consider a RemoveItem with stackCount option? (and with query option?)

	// Because of some delegates/events and how our core Item works, we can't allow directly modifying it
	// Thus you have to use this function to change the count of an item stack, which will then trigger the appropriate events and delegates.
	void SetItemStackCount(const FRockItemStackHandle& Handle, int32 NewCount, bool bAutoRemoveIfZero = true);
	bool SetItemCustomValueByTag(const FRockItemStackHandle& Handle, FGameplayTag tag, int32 NewCount);
private:
	// Internal use only
	uint32 AcquireAvailableItemIndex();
public:
	/** Total units summed across all stacks. */
	int32 GetTotalItemQuantity() const;
	/** Number of occupied stacks (not units). */
	int32 GetNumItemStacks() const;

	/** Does this handle point to a valid item stack in the inventory */
	bool IsHandleValid(FRockItemStackHandle ItemHandle) const;
	FRockItemReference MakeItemReference(FRockItemStackHandle SlotHandle);
	FRockSlotReference MakeSlotReference(FRockInventorySlotHandle SlotHandle) const;

	// TODO: Should we handle ActivateItem, DeactivateItem, UseItem
	// Or should other classes handle that?

	void ForEachSlot(const FRockInventoryQuery& Query, const TFunctionRef<bool(const FRockInventorySectionInfo*, const FRockInventorySlotEntry*)>& Visitor);

	const FRockInventorySlotEntry* FindFirstSlot(const FRockInventoryQuery& Query);

	/** Note: This function should be considered expensive. O(n) with no early out */
	TArray<FRockInventorySlotEntry> FindAllSlots(const FRockInventoryQuery& Query);
	/** Note: This function should be considered expensive O(n) with no early out */
	TArray<FRockItemStackHandle> FindAllItemHandles(const FRockInventoryQuery& Query);


	///////////////////////////////////
	// Misc
	friend class URockInventoryLibrary;
	friend struct FRockInventoryData;
	friend class URockItemInstanceLibrary;
	friend class URockInventoryComponent;
#if WITH_DEV_AUTOMATION_TESTS
	/** Lets tests stand in for the replication system (apply replicated arrays, call their callbacks). */
	friend struct FRockInventoryTestAccess;
#endif
};

/** Groups everything done to an inventory while it lives into one change batch (and one Revision step). Null-safe. */
struct FRockInventoryOperationScope
{
	explicit FRockInventoryOperationScope(URockInventory* InInventory) : Inventory(InInventory)
	{
		if (Inventory) { Inventory->BeginOperation(); }
	}
	~FRockInventoryOperationScope()
	{
		if (Inventory) { Inventory->EndOperation(); }
	}
	FRockInventoryOperationScope(const FRockInventoryOperationScope&) = delete;
	FRockInventoryOperationScope& operator=(const FRockInventoryOperationScope&) = delete;

private:
	URockInventory* Inventory;
};


///////////////////////////////////////////////////////////////////////////////
/// Inline functions
FORCEINLINE const FRockInventorySlotEntry& URockInventory::GetSlotByAbsoluteIndex(int32 AbsoluteIndex) const
{
	return SlotData[AbsoluteIndex];
}
