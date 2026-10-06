// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Events/RockInventoryChangeBatch.h"
#include "Inventory/Events/RockItemDelta.h"
#include "Inventory/Events/RockSlotDelta.h"
#include "UObject/Object.h"

#include "RockInventoryTestListener.generated.h"

/**
 * Records every batch and delta an inventory broadcasts. The inventory delegates are dynamic, so they need a UObject with UFUNCTIONs.
 * CallOrder has one letter per callback in arrival order: B batch, S slot delta, I item delta.
 * ProbeFunction, when set, runs at the start of every callback (to look at what state the inventory is in while it is being told).
 */
UCLASS()
class URockInventoryTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnChangeBatch(const FRockInventoryChangeBatch& Batch) { Probe(); CallOrder.AppendChar('B'); Batches.Add(Batch); }

	UFUNCTION()
	void OnSlotChanged(const FRockSlotDelta& SlotDelta) { Probe(); CallOrder.AppendChar('S'); SlotDeltas.Add(SlotDelta); }

	UFUNCTION()
	void OnItemChanged(const FRockItemDelta& ItemDelta) { Probe(); CallOrder.AppendChar('I'); ItemDeltas.Add(ItemDelta); }

	TArray<FRockInventoryChangeBatch> Batches;
	TArray<FRockSlotDelta> SlotDeltas;
	TArray<FRockItemDelta> ItemDeltas;
	FString CallOrder;
	TFunction<void()> ProbeFunction;

private:
	void Probe() { if (ProbeFunction) { ProbeFunction(); } }
};
