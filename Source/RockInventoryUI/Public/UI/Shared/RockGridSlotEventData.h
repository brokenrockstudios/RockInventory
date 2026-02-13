// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_TileQuadrant.h"
#include "Inventory/RockSlotHandle.h"
#include "UObject/Object.h"
#include "RockGridSlotEventData.generated.h"

class URockInventory_Slot_BackgroundBase;
class URockInventory_Slot_ItemBase;
class URockInventory;

USTRUCT(BlueprintType)
struct FRockGridSlotEventData
{
	GENERATED_BODY()

	FRockGridSlotEventData() : SlotWidget(nullptr), Inventory(nullptr)
	{
	}

	FRockGridSlotEventData(
		URockInventory_Slot_BackgroundBase* InSlotWidget,
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		URockInventory* InInventory,
		const FRockInventorySlotHandle& InSlotHandle,
		const FRockInventorySlotHandle& InAnchorSlotHandle,
		ERockInventory_TileQuadrant InTileQuadrant
	)
		: SlotWidget(InSlotWidget)
		  , Geometry(InGeometry)
		  , MouseEvent(InMouseEvent)
		  , Inventory(InInventory)
		  , SlotHandle(InSlotHandle)
		  , AnchorSlotHandle(InAnchorSlotHandle)
		  , TileQuadrant(InTileQuadrant)
	{
	}

	UPROPERTY()
	TObjectPtr<URockInventory_Slot_BackgroundBase> SlotWidget;

	UPROPERTY()
	FGeometry Geometry;

	UPROPERTY()
	FPointerEvent MouseEvent;

	UPROPERTY()
	TObjectPtr<URockInventory> Inventory;

	// Never Changing index of this slot in the inventory
	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;

	// The index of this slot in the local section
	UPROPERTY()
	FRockInventorySlotHandle AnchorSlotHandle;

	UPROPERTY()
	ERockInventory_TileQuadrant TileQuadrant = ERockInventory_TileQuadrant::None;
};
