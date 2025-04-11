// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventoryConfig.h"
#include "RockInventoryData.h"
#include "RockSlotHandle.h"
#include "UObject/Object.h"
#include "RockInventory.generated.h"

/**
 * Delegate that is broadcast when the inventory changes.
 * @param Inventory - The inventory that changed
 * @param SlotHandle - The handle of the slot that was modified
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, URockInventory*, Inventory, const FRockInventorySlotHandle&, SlotHandle);

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
	/** The inventory data containing all slots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	FRockInventoryData InventoryData;

	/** Tab configuration for the inventory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, meta = (AllowPrivateAccess = true))
	TArray<FRockInventoryTabInfo> Tabs;

	/**
	 * Initialize the inventory with a configuration
	 * @param config - The configuration to use for initialization
	 */
	void Init(const URockInventoryConfig* config);

	/**
	 * Get tab info by index (fastest method)
	 * @param TabIndex - The index of the tab to retrieve
	 * @return Pointer to the tab info, or nullptr if index is invalid
	 */
	const FRockInventoryTabInfo* GetTabInfo(int32 TabIndex) const;

	/**
	 * Find tab index by name (slower than GetTabInfo)
	 * @param TabName - The name of the tab to find
	 * @return The index of the tab, or INDEX_NONE if not found
	 */
	int32 FindTabIndex(const FName& TabName) const;

	/**
	 * Blueprint-friendly version of GetSlotByHandle
	 * @param InSlotHandle - The handle of the slot to retrieve
	 * @return The slot, or an empty slot if handle is invalid
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	FRockInventorySlot GetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle) const;

	/**
	 * Set the slot at the given handle
	 * @param InSlotHandle - The handle of the slot to set
	 * @param InSlot - The slot to set
	 */
	void SetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle, const FRockInventorySlot& InSlot);

	/**
	 * Get the slot index in the AllSlots array
	 * @param TabIndex - The tab index
	 * @param X - The X coordinate in the tab
	 * @param Y - The Y coordinate in the tab
	 * @return The slot index, or INDEX_NONE if coordinates are invalid
	 */
	int32 GetSlotIndex(int32 TabIndex, int32 X, int32 Y) const;

	/**
	 * Get slot at specific coordinates in a tab
	 * @param TabIndex - The tab index
	 * @param X - The X coordinate in the tab
	 * @param Y - The Y coordinate in the tab
	 * @return Pointer to the slot, or nullptr if coordinates are invalid
	 */
	FRockInventorySlot GetSlotAt(int32 TabIndex, int32 X, int32 Y) const;
	

	/**
	 * Add a new tab and initialize its slots
	 * @param TabID - The unique identifier for the tab
	 * @param Width - The width of the tab in slots
	 * @param Height - The height of the tab in slots
	 * @return The index of the newly created tab
	 */
	int32 AddTab(FName TabID, int32 Width, int32 Height);

	/**
	 * Get all slots in a specific tab
	 * @param TabIndex - The index of the tab
	 * @return Array view of the slots in the tab
	 */
	TArrayView<FRockInventorySlot> GetTabSlots(int32 TabIndex);

	/**
	 * Find tab index by ID
	 * @param TabID - The ID of the tab to find
	 * @return The index of the tab, or INDEX_NONE if not found
	 */
	int32 GetTabIndexByID(FName TabID) const;

	/**
	 * Move an item from one inventory to another
	 * @param SourceInventory - The source inventory
	 * @param SourceSlotHandle - The handle of the source slot
	 * @param TargetInventory - The target inventory
	 * @param TargetSlotHandle - The handle of the target slot
	 * @return true if the move was successful
	 */
	bool MoveItem(URockInventory* SourceInventory, FRockInventorySlotHandle SourceSlotHandle, URockInventory* TargetInventory, FRockInventorySlotHandle TargetSlotHandle);

	/** Override to specify which properties should be replicated */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Broadcast the inventory changed event */
	void BroadcastInventoryChanged(const FRockInventorySlotHandle& SlotHandle = FRockInventorySlotHandle());

	/** Get a debug string representation of the inventory */
	FString GetDebugString() const;

	/** Called when the inventory changes */
	UPROPERTY(BlueprintAssignable, Category = "Rock|Inventory")
	FOnInventoryChanged OnInventoryChanged;
};


