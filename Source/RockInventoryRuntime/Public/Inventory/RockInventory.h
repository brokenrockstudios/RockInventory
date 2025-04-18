// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventoryConfig.h"
#include "RockInventorySlot.h"
#include "RockSlotHandle.h"
#include "Item/RockItemStack.h"
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
	URockInventory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

private:
	friend class URockInventoryLibrary;
	friend class URockItemInstanceLibrary;
	/** The item data */
	UPROPERTY(VisibleAnywhere, Replicated)
	FRockInventoryItemContainer ItemData;

	/** Stack of available slot indices for reuse */
	UPROPERTY()
	TArray<uint32> FreeIndices;

	/** The grid slot data */
	UPROPERTY(VisibleAnywhere, Replicated)
	FRockInventorySlotContainer SlotData;
	
	// If you are holding an item, this could be the last slot you picked up, that might be 'dropped' or something soon?
	// Should this be on the inventory or on the player or component/manager/or something?
	// UPROPERTY(VisibleAnywhere, Transient)
	// FRockInventorySlotHandle PendingItemHandle;

	/** Tab configuration for the inventory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, meta = (AllowPrivateAccess = true))
	TArray<FRockInventorySectionInfo> SlotSections;

public:
	// Owning Actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TObjectPtr<AActor> OwningActor;


	/**
	 * Initialize the inventory with a configuration
	 * @param config - The configuration to use for initialization
	 */
	void Init(const URockInventoryConfig* config);

	/**
	 * Get section info by name
	 * @param SectionName - The name of the section to retrieve
	 * @return Pointer to the tab info, or nullptr if index is invalid
	 */
	FRockInventorySectionInfo GetSectionInfo(const FName& SectionName) const;

	/**
	 * Find section index by name
	 * @param SectionName - The ID of the tab to find
	 * @return The index of the tab, or INDEX_NONE if not found
	 */
	int32 GetSectionIndexById(const FName& SectionName) const;

	/**
	 * Get slot by handle
	 * @param InSlotHandle - The handle of the slot to retrieve
	 * @return The slot, or an empty slot if handle is invalid
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	FRockInventorySlotEntry GetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle) const;

	const FRockInventorySlotEntry& GetSlotByAbsoluteIndex(int32 AbsoluteIndex) const;

	FRockItemStack GetItemBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const;
	FRockItemStack GetItemByHandle(const FRockItemStackHandle& InSlotHandle) const;
	void SetItemByHandle(const FRockItemStackHandle& InSlotHandle, const FRockItemStack& InItemStack);

	/**
	 * Set the slot at the given handle
	 * @param InSlotHandle - The handle of the slot to set
	 * @param InSlotEntry - The slot to set
	 */
	void SetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle, const FRockInventorySlotEntry& InSlotEntry);

	/**
	 * Add a new tab and initialize its slots
	 * @param SectionName - The unique identifier for the tab
	 * @param Width - The width of the tab in slots
	 * @param Height - The height of the tab in slots
	 * @return The index of the newly created tab
	 */
	int32 AddSection(const FName& SectionName, int32 Width, int32 Height);

	/** Override to specify which properties should be replicated */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Broadcast the inventory changed event */
	void BroadcastSlotChanged(const FRockInventorySlotHandle& SlotHandle = FRockInventorySlotHandle());
	void BroadcastItemChanged(const FRockItemStackHandle& RockInventorySlotHandle = FRockItemStackHandle());


	/** Get a debug string representation of the inventory */
	FString GetDebugString() const;

	FRockItemStackHandle AddItemToInventory(const FRockItemStack& InItemStack);

	/** Called when the inventory changes */
	UPROPERTY(BlueprintAssignable, Category = "Rock|Inventory")
	FOnInventoryChanged OnInventoryChanged;
};


///////////////////////////////////////////////////////////////////////////////
/// Inline functions
FORCEINLINE const FRockInventorySlotEntry& URockInventory::GetSlotByAbsoluteIndex(int32 AbsoluteIndex) const
{
	return SlotData[AbsoluteIndex];
}
