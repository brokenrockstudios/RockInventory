// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemState.h"
#include "RockItemState_NestedInventory.generated.h"

class URockInventory;

/**
 * Runtime state for item instances that own a nested inventory (e.g. backpacks, containers).
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemState_NestedInventory : public FRockItemState
{
	GENERATED_BODY()

	virtual void OnStateAdded(URockItemInstance* OwnerInstance) override;
	virtual void OnStateRemoved(URockItemInstance* OwnerInstance) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	TObjectPtr<URockInventory> NestedInventory = nullptr;
};
