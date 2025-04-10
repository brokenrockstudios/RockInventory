// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockInventoryTabInfo.h"
#include "Inventory/RockItemOrientation.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventoryLibrary.generated.h"

enum class ERockItemOrientation : uint8;
struct FRockInventorySlot;
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
	static bool AddItemToInventory(URockInventory* Inventory, FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess);

	//////////////////////////////////////////////////////////////////////////
	/// Inventory Location Manipulation
	UFUNCTION(BlueprintCallable)
	static bool PlaceItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, FRockItemStack& ItemStack, ERockItemOrientation DesiredOrientation);
	UFUNCTION(BlueprintCallable)
	static bool RemoveItemAtLocation(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle);
	UFUNCTION(BlueprintCallable)
	static bool MoveItem(URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
		URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
		ERockItemOrientation DesiredOrientation = ERockItemOrientation::Horizontal);

	//////////////////////////////////////////////////////////////////////////
	// Utility functions
	static void PrecomputeOccupancyGrids(const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid);
	static bool CanItemFitInGridPosition(const TArray<bool>& OccupancyGrid, const FRockInventoryTabInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize);
};
