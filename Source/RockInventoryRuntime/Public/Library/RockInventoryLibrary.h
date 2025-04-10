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

	static void PrecomputeOccupancyGrids(const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid);
	static bool AddItemToInventory(URockInventory* Inventory, FRockItemStack& ItemStack, FRockInventorySlotHandle& OutHandle, int32& OutExcess);
	static bool CanItemFitInGridPosition(const TArray<bool>& OccupancyGrid, const FRockInventoryTabInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize);

	static bool PlaceItemAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, FRockItemStack& ItemStack, ERockItemOrientation DesiredOrientation);
	// static bool PlaceItemAtLocation(URockInventory* Inventory, const FRockItemStack& ItemStack, int32 TabIndex, int32 StartX, int32 StartY, FRockInventorySlotHandle& OutHandle);
	static bool RemoveItemAtLocation(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle);


	UFUNCTION(BlueprintCallable)
	static bool MoveItem(URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
		URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
		ERockItemOrientation DesiredOrientation = ERockItemOrientation::Horizontal);

	
	// static bool CanItemFitInTab(const URockInventory* Inventory, const FRockItemStack& ItemStack, int32 TabIndex, int32 StartX, int32 StartY);
};
