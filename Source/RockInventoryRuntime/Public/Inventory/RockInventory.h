// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockContainerExperiment.h"
#include "UObject/Object.h"
#include "RockInventory.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, URockInventory*, Inventory, const FRockInventorySlotHandle&, SlotHandle);


/*
 * This class is the root class for the Rock Inventory System
 * It manages the inventory data and tab configuration
 * Assume each Tab is a collection of slots. 
 * So a chest rig might have 4 tabs, each with 2x1 slots. 
 */

/**
 * 
 */

// The root class for the RockInventory system
// This class manages the inventory data and tab configuration
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventory : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	FRockInventoryData InventoryData;

	// Tab configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TArray<FRockInventoryTabInfo> Tabs;

	// Get tab info by index
	const FRockInventoryTabInfo* GetTabInfo(int32 TabIndex) const;
	FRockInventorySlot* GetSlotByHandle(FRockInventorySlotHandle SlotHandle);
	// Get slot index in the AllSlots array
	int32 GetSlotIndex(int32 TabIndex, int32 X, int32 Y) const;
	// Get slot at specific coordinates in a tab
	FRockInventorySlot* GetSlotAt(int32 TabIndex, int32 X, int32 Y);
	// Add a new tab and initialize its slots
	int32 AddTab(FName TabID, int32 Width, int32 Height);
	// Get all slots in a specific tab - returns array view for efficiency
	TArrayView<FRockInventorySlot> GetTabSlots(int32 TabIndex);
	// Find tab index by name (for UI/scripting)
	int32 GetTabIndexByID(FName TabID) const;

	bool MoveItem(URockInventory* SourceInventory, FRockInventorySlotHandle SourceSlotHandle, URockInventory* TargetInventory, FRockInventorySlotHandle TargetSlotHandle);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    
	/** Called when the inventory changes */
	UPROPERTY(BlueprintAssignable, Category = "Rock|Inventory")
	FOnInventoryChanged OnInventoryChanged;
};
