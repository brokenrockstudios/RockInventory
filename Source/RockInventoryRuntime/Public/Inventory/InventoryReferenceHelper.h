// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventorySlot.h"
#include "RockSlotHandle.h"
#include "Item/RockItemStackHandle.h"
#include "UObject/Object.h"
#include "InventoryReferenceHelper.generated.h"

/**
 * 
 */
USTRUCT()
struct FRockItemReference
{
	GENERATED_BODY()
	
	// inventory and ItemHandle
	UPROPERTY()
	TWeakObjectPtr<class URockInventory> Inventory;
	UPROPERTY()
	FRockItemStackHandle ItemHandle;
	
	// getter function
	FRockItemStack GetCopyOfItem() const;
};

USTRUCT(BlueprintType, Blueprintable)
struct ROCKINVENTORYRUNTIME_API FRockSlotReference
{
	GENERATED_BODY()
	
	// inventory and SlotHandle
	UPROPERTY()
	TWeakObjectPtr<class URockInventory> Inventory = nullptr;
	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;
	
	// getter function
	FRockInventorySlotEntry GetSlotEntry() const;
};
