// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RockInventoryComponent.h"
#include "Inventory/RockInventoryTabInfo.h"
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
	static bool AddItemToInventory(URockInventory* Inventory, const FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess);

	//////////////////////////////////////////////////////////////////////////
	/// Inventory Location Manipulation
	UFUNCTION(BlueprintCallable)
	static bool PlaceItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, const FRockItemStack& ItemStack, ERockItemOrientation DesiredOrientation);
	UFUNCTION(BlueprintCallable)
	static bool GetItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, FRockItemStack& OutItemStack);
	UFUNCTION(BlueprintCallable)
	static bool RemoveItemAtLocation(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle);
	UFUNCTION(BlueprintCallable)
	static bool MoveItem(URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
		URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
		ERockItemOrientation DesiredOrientation = ERockItemOrientation::Horizontal);

	UFUNCTION(BlueprintCallable)
	static int32 GetItemCount(const URockInventory* Inventory, const FName& ItemId);
	
	//////////////////////////////////////////////////////////////////////////
	// Misc Utility functions
	static void PrecomputeOccupancyGrids(const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid);
	static bool CanItemFitInGridPosition(const TArray<bool>& OccupancyGrid, const FRockInventoryTabInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize);
};
