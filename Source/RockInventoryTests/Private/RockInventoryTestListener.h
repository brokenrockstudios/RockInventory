// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Events/RockItemDelta.h"
#include "Inventory/Events/RockSlotDelta.h"
#include "UObject/Object.h"

#include "RockInventoryTestListener.generated.h"

/** Records every delta an inventory broadcasts. The inventory delegates are dynamic, so they need a UObject with UFUNCTIONs. */
UCLASS()
class URockInventoryTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnSlotChanged(const FRockSlotDelta& SlotDelta) { SlotDeltas.Add(SlotDelta); }

	UFUNCTION()
	void OnItemChanged(const FRockItemDelta& ItemDelta) { ItemDeltas.Add(ItemDelta); }

	TArray<FRockSlotDelta> SlotDeltas;
	TArray<FRockItemDelta> ItemDeltas;
};
