// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockSlotChangeType.h"
#include "Inventory/RockSlotHandle.h"
#include "UObject/Object.h"
#include "RockSlotDelta.generated.h"

class URockInventory;

/**
 * Struct that describes a change to a slot in an inventory.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockSlotDelta
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<URockInventory> Inventory = nullptr;

	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;

	UPROPERTY()
	ERockSlotChangeType ChangeType = ERockSlotChangeType::None;
};


