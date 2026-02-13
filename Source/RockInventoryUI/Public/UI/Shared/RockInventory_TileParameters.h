// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_TileQuadrant.h"
#include "RockInventory_TileParameters.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct FRockInventory_TileParameters
{
	GENERATED_BODY()

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	//FIntPoint TileCoordinates = FIntPoint::NoneValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FRockInventorySlotHandle SlotHandle;
	//int32 TileIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	ERockInventory_TileQuadrant TileQuadrant = ERockInventory_TileQuadrant::None;
};
