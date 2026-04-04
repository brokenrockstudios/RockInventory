// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemChangeType.h"
#include "Item/RockItemStackHandle.h"
#include "UObject/Object.h"
#include "RockItemDelta.generated.h"

class URockInventory;

/**
 * Struct that describes a change to an item in an inventory.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemDelta
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<URockInventory> Inventory = nullptr;

	UPROPERTY()
	FRockItemStackHandle ItemHandle;

	UPROPERTY()
	ERockItemChangeType ChangeType = ERockItemChangeType::None;
	// TODO:
	// Add DirtyFlags to core ItemStack to have 4 bits dedicated to StackCount, CustomValue1, CustomValue2, RuntimeInstance
	// And when replicating modify a last changed in replication then reset it back after replication broadcast
};
