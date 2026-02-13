// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockSlotHandle.h"
#include "UObject/Object.h"
#include "RockContainerItemView.generated.h"

class URockInventory_Slot_ItemBase;

USTRUCT(BlueprintType)
struct ROCKINVENTORYUI_API FRockContainerItemView
{
	GENERATED_BODY()

public:
	// may be null if virtualized
	TWeakObjectPtr<URockInventory_Slot_ItemBase> Widget;
	// upper-left slot (or your anchor)
	FRockInventorySlotHandle AnchorSlot;
	// last known size in tiles
	FIntPoint Size = FIntPoint(1, 1);

	// cached Tab-local index (optional)
	//int32 LocalIndex = INDEX_NONE;
	// TArray<int32> CoveredLocalIndices; // optional: exact covered tiles for fast clearing
};
