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
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemReference
{
	GENERATED_BODY()

public:
	FRockItemReference() = default;

	// Identity
	bool IsValid() const;
	URockInventory* GetInventory() const { return Inventory.Get(); }
	FRockItemStackHandle GetItemHandle() const { return ItemHandle; }

	// Data Access
	URockItemDefinition* GetDefinition() const;
	int32 GetStackCount() const;
	URockItemInstance* GetRuntimeInstanceOrNull() const;
	TOptional<int32> GetCustomValueByTag(FGameplayTag Tag) const;
	
	// It's generally recommended to not use this, as a copy of the struct can lead to outdated values. Don't hold onto long term references to these.
	FRockItemStack GetCopyOfItem() const;
	
private:
	// inventory and ItemHandle
	UPROPERTY()
	TWeakObjectPtr<class URockInventory> Inventory = nullptr;
	UPROPERTY()
	FRockItemStackHandle ItemHandle;

private:
	// Only allow an inventory to be able to create these references. As they generally should be considered valid (until they become stale)
	FRockItemReference(URockInventory* InInventory, FRockItemStackHandle InSlotHandle);
	friend class URockInventory;
};

USTRUCT(BlueprintType)
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
