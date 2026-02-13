// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_TileQuadrant.h"
#include "Inventory/RockSlotHandle.h"
#include "UObject/Object.h"
#include "RockGridItemEventData.generated.h"

class URockInventory;
class URockInventory_Slot_ItemBase;


/**
 *
 */
USTRUCT(BlueprintType)
struct FRockGridItemEventData
{
	GENERATED_BODY()

	FRockGridItemEventData() : SlotWidget(nullptr), Inventory(nullptr)
	{
	}

	FRockGridItemEventData(
		URockInventory_Slot_ItemBase* InSlotWidget, const FGeometry& InGeometry, const FPointerEvent& InPointerEvent,
		URockInventory* InInventory, const FRockInventorySlotHandle& InSlotHandle, ERockInventory_TileQuadrant InTileQuadrant)
		: SlotWidget(InSlotWidget)
		  , Geometry(InGeometry)
		  , PointerEvent(InPointerEvent)
		  , Inventory(InInventory)
		  , SlotHandle(InSlotHandle)
		  , TileQuadrant(InTileQuadrant)
	{
	}

	UPROPERTY()
	TObjectPtr<URockInventory_Slot_ItemBase> SlotWidget;

	UPROPERTY()
	FGeometry Geometry;

	UPROPERTY()
	FPointerEvent PointerEvent;

	UPROPERTY()
	TObjectPtr<URockInventory> Inventory;

	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;

	// If 'Clicked' or 'Hovered' where in the tile was it. 
	UPROPERTY()
	ERockInventory_TileQuadrant TileQuadrant = ERockInventory_TileQuadrant::None;
};
