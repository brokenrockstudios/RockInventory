// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Enums/RockEnums.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Item/RockMoveItemParams.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventoryLibrary.generated.h"

class URockInventory;

/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Core Item
	// Add it from anywhere. This will attempt to merge into existing stacks.
	// In case of multiple stacks being merged, the last one will be assigned the OutHandle
	// We also will 'fully initialized' any items not initialized (e.g. Create their runtime instances)
	static bool LootItemToInventory(
		URockInventory* Inventory, const FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess);

	//////////////////////////////////////////////////////////////////////////
	/// Inventory Location Manipulation

	/** Remove an item at a specific location
	 * @param Inventory - The inventory to remove the item from
	 * @param SlotHandle - The handle of the slot to remove the item from
	 * @param Quantity - The quantity of the item to remove. If -1, remove the entire stack
	 * @return The item stack that was removed
	 */
	UFUNCTION(BlueprintCallable)
	static FRockItemStack SplitItemStackAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, int32 Quantity = -1);

	/**
	 * Move an item from one inventory to another
	 * @param SourceInventory - The source inventory
	 * @param SourceSlotHandle - The handle of the source slot
	 * @param TargetInventory - The target inventory
	 * @param TargetSlotHandle - The handle of the target slot
	 * @param InMoveParams - The move parameters
	 * @return true if the move was successful
	 */
	UFUNCTION(BlueprintCallable)
	static bool MoveItem(
		URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
		URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
		const FRockMoveItemParams& InMoveParams = FRockMoveItemParams());

	// Misc helpers

	static bool CanMergeItemAtGridPosition(
		const URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack,
		ERockItemStackMergeCondition MergeCondition = ERockItemStackMergeCondition::Full);
	static int32 MergeItemAtGridPosition(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack);

	/** Get the item at a specific location
	 * @param Inventory - The inventory to get the item from
	 * @param SlotHandle - The handle of the slot to get the item from
	 * @return The item stack at the specified location
	 */
	UFUNCTION(BlueprintCallable)
	static FRockItemStack GetItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle);

	/** Get the item count in an inventory
	 * @param Inventory - The inventory to get the item from
	 * @param ItemId - The ItemID for which you want to count
	 * @return The item stack at the specified location
	 */
	UFUNCTION(BlueprintCallable)
	static int32 GetItemCount(const URockInventory* Inventory, const FName& ItemId);

	/**
	 * Checks if an item can be placed in a section based on its type restrictions
	 * @param ItemStack - The item stack to check
	 * @param SectionInfo - The section info to check against
	 * @return True if the item can be placed in the section
	 */
	static bool CanItemBePlacedInSection(
		const FRockItemStack& ItemStack,
		const FRockInventorySectionInfo& SectionInfo);

	//////////////////////////////////////////////////////////////////////////
	// Misc Utility functions
	static void PrecomputeOccupancyGrids(
		const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid, FRockItemStackHandle IgnoreItemHandle = FRockItemStackHandle());
	static bool CanItemFitInGridPosition(
		const TArray<bool>& OccupancyGrid, const FRockInventorySectionInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	static TArray<FString> GetInventoryContentsDebug(const URockInventory* Inventory);

	// This needs to point to where the inventory is registered for replication
	static UObject* GetTopLevelOwner(UObject* Instance);
	
};
