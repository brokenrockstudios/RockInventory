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
	// Add it anywhere we can. This will attempt to merge into existing stacks.
	// In case of multiple stacks being merged, the last one will be assigned the OutHandle
	static bool LootItemToInventory(URockInventory* Inventory, const FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess);

	//////////////////////////////////////////////////////////////////////////
	/// Inventory Location Manipulation
	// Add item at a specific location a specific location and orientation.
	UFUNCTION(BlueprintCallable)
	static bool PlaceItemAtSlot(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, const FRockItemStackHandle& ItemStackHandle, ERockItemOrientation DesiredOrientation);
	UFUNCTION(BlueprintCallable)
	static bool GetItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, FRockItemStack& OutItemStack);
	UFUNCTION(BlueprintCallable)
	static bool RemoveItemAtLocation(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle);

	/**
	 * Move an item from one inventory to another
	 * @param SourceInventory - The source inventory
	 * @param SourceSlotHandle - The handle of the source slot
	 * @param TargetInventory - The target inventory
	 * @param TargetSlotHandle - The handle of the target slot
	 * @return true if the move was successful
	 */
	UFUNCTION(BlueprintCallable)
	static bool MoveItem(URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
		URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
		ERockItemOrientation DesiredOrientation = ERockItemOrientation::Horizontal);

	UFUNCTION(BlueprintCallable)
	static int32 GetItemCount(const URockInventory* Inventory, const FName& ItemId);
	
	//////////////////////////////////////////////////////////////////////////
	// Misc Utility functions
	static void PrecomputeOccupancyGrids(const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid);
	static bool CanItemFitInGridPosition(const TArray<bool>& OccupancyGrid, const FRockInventorySectionInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize);
};
