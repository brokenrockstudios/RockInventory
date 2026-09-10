// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemFragment.h"
#include "RockItemFragment_NestedInventory.generated.h"

class URockInventoryConfig;

/**
 * Grants the item instance a nested inventory (e.g. a Backpack's contents, a Weapon's attachment slots).
 */
USTRUCT(BlueprintType, meta=(DisplayName="Nested Inventory"))
struct ROCKINVENTORYRUNTIME_API FRockItemFragment_NestedInventory : public FRockItemFragment
{
	GENERATED_BODY()

	// The layout used to initialize this item's nested inventory.
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSoftObjectPtr<URockInventoryConfig> InventoryConfig = nullptr;

	virtual void OnInstanceCreated(URockItemInstance* ItemInstance) const override;
};
