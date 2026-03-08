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
USTRUCT(BlueprintType, Blueprintable)
struct ROCKINVENTORYRUNTIME_API FRockItemReference
{
	GENERATED_BODY()
public:
	FRockItemReference() = default;
	URockInventory * GetInventory() const { return Inventory.Get(); }
	FRockItemStackHandle GetItemHandle() const { return ItemHandle; }
	
	FRockItemStack GetCopyOfItem() const;
	
private:
	// inventory and ItemHandle
	UPROPERTY()
	TWeakObjectPtr<class URockInventory> Inventory = nullptr;
	UPROPERTY()
	FRockItemStackHandle ItemHandle;
	
	bool IsValid() const;
private:
	// Only allow an inventory to be able to create these references
	FRockItemReference(URockInventory* InInventory, FRockItemStackHandle InSlotHandle);
	friend class URockInventory;
};

USTRUCT(BlueprintType, Blueprintable)
struct ROCKINVENTORYRUNTIME_API FRockSlotReference
{
	GENERATED_BODY()
public:
	FRockSlotReference() = default;
	URockInventory* GetInventory() const { return Inventory.Get(); }
	FRockInventorySlotHandle GetSlotHandle() const { return SlotHandle; }
	// getter function
	FRockInventorySlotEntry GetSlotEntry() const;
private:
	
	// We don't want anyone to modify these. They should be initialized only by RockInventory
	UPROPERTY()
	TWeakObjectPtr<class URockInventory> Inventory = nullptr;
	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;
	
	// Only allow an inventory to be able to create these references
	FRockSlotReference(URockInventory* InInventory, FRockInventorySlotHandle InSlotHandle);
	friend class URockInventory;
};
