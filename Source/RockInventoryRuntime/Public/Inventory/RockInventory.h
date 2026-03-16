// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InventoryReferenceHelper.h"
#include "RockInventoryConfig.h"
#include "RockInventoryQuery.h"
#include "RockInventorySlot.h"
#include "RockPendingSlotOperation.h"
#include "RockSlotHandle.h"
#include "Events/RockItemDelta.h"
#include "Events/RockSlotChangeType.h"
#include "Item/RockItemStack.h"
#include "UObject/Object.h"

#include "RockInventory.generated.h"

/**
 * Delegate that is broadcast when the inventory changes.
 * @param Inventory - The inventory that changed
 * @param SlotHandle - The handle of the slot that was modified
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotChanged, const FRockSlotDelta&, SlotDelta);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemStackChanged, const FRockItemDelta&, ItemDelta);

// URockInventory*, Inventory, const FRockItemStackHandle&, ItemHandle);

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

public:
	/** Broadcast when a slot's state changes (item assigned, removed, etc). */
	UPROPERTY(BlueprintAssignable, Category = "Rock|Inventory")
	FOnInventorySlotChanged OnSlotChanged;

	/** Broadcast when an item stack's data changes (count, customValue, etc). */
	UPROPERTY(BlueprintAssignable, Category = "Rock|Inventory")
	FOnInventoryItemStackChanged OnItemChanged;

	/* The owner of this inventory, most likely the InventoryComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TObjectPtr<UObject> Owner;

	UObject* GetOwner() const { return Owner; }

	/** Walks the ownership chain until it finds an Actor */
	AActor* GetOwningActor();

	/** Iterates slots; return true from Func to break early. */
	void ForEachSlotInSection(const TFunctionRef<bool(const FRockInventorySectionInfo&, const FRockInventorySlotEntry&)>& Func) const;

	/** Iterates item stacks; return true from Func to break early. */
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

	// TODO: Maintain reverse mapping for ItemHandle->SlotHandle to make this more efficient
	// TODO: Currently O(N) but could be O(1) with memory cache
	// Alternatively try to not use this as much
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
	void BroadcastSlotChanged(const FRockInventorySlotHandle& SlotHandle, ERockSlotChangeType ChangeType);
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

	FRockItemStackHandle AddItemToInventory(const FRockItemStack& InItemStack);
	void RemoveItemFromInventory(const FRockItemStackHandle& InItemStackHandle);
	void RemoveItemFromInventory(const FRockItemStack& InItemStack);

	// Because of some delegates/events and how our core Item works, we can't allow directly modifying it
	// Thus you have to use this function to change the count of an item stack, which will then trigger the appropriate events and delegates.
	void SetItemStackCount(const FRockItemStackHandle& Handle, int32 NewCount, bool bAutoRemoveIfZero = true);
	bool SetItemCustomValueByTag(const FRockItemStackHandle& Handle, FGameplayTag tag, int32 NewCount);
private:
	// Internal use only
	uint32 AcquireAvailableItemIndex();
public:
	int32 GetItemStackCount();
	int32 GetItemTotalCount();
	
	/** Does this handle point to a valid item stack in the inventory */
	bool IsHandleValid(FRockItemStackHandle ItemHandle) const;
	FRockItemReference MakeItemReference(FRockItemStackHandle SlotHandle);
	FRockSlotReference MakeSlotReference(FRockInventorySlotHandle SlotHandle);

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
	friend class URockItemInstanceLibrary;
	friend class URockInventoryComponent;
};


///////////////////////////////////////////////////////////////////////////////
/// Inline functions
FORCEINLINE const FRockInventorySlotEntry& URockInventory::GetSlotByAbsoluteIndex(int32 AbsoluteIndex) const
{
	return SlotData[AbsoluteIndex];
}
