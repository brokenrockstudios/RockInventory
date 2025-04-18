// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/RockInventoryComponent.h"
#include "Inventory/RockItemOrientation.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventoryLibrary.generated.h"

enum class ERockItemOrientation : uint8;

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


	/** Place an item at a specific location
	 * @param Inventory - The inventory to place the item in
	 * @param SlotHandle - The handle of the slot to place the item in
	 * @param ItemStackHandle - The handle of the item stack to place
	 * @param DesiredOrientation - The desired orientation of the item
	 * @return true if the item was placed successfully
	 */
	UFUNCTION(BlueprintCallable)
	static bool PlaceItemAtSlot(
		URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, const FRockItemStackHandle& ItemStackHandle,
		ERockItemOrientation DesiredOrientation);

	// Used internally to place an item at a specific location
	static bool PlaceItemAtSlot_Internal(
		URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, const FRockItemStack& ItemStack,
		ERockItemOrientation DesiredOrientation);

	/** Remove an item at a specific location
	 * @param Inventory - The inventory to remove the item from
	 * @param SlotHandle - The handle of the slot to remove the item from
	 * @return The item stack that was removed
	 */
	UFUNCTION(BlueprintCallable)
	static FRockItemStack RemoveItemAtLocation(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle);

	/**
	 * Move an item from one inventory to another
	 * @param SourceInventory - The source inventory
	 * @param SourceSlotHandle - The handle of the source slot
	 * @param TargetInventory - The target inventory
	 * @param TargetSlotHandle - The handle of the target slot
	 * @param DesiredOrientation - The desired orientation of the item
	 * @return true if the move was successful
	 */
	UFUNCTION(BlueprintCallable)
	static bool MoveItem(
		URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
		URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
		ERockItemOrientation DesiredOrientation = ERockItemOrientation::Horizontal);


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

	//////////////////////////////////////////////////////////////////////////
	// Misc Utility functions
	static void PrecomputeOccupancyGrids(
		const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid, FRockItemStackHandle IgnoreItemHandle = FRockItemStackHandle());
	static bool CanItemFitInGridPosition(
		const TArray<bool>& OccupancyGrid, const FRockInventorySectionInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize);
};
