// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemDelta.h"
#include "RockSlotDelta.h"

#include "RockInventoryChangeBatch.generated.h"

class URockInventory;

/**
 * Everything one operation changed in an inventory, delivered once the operation is complete.
 *
 * On the server a batch is one operation (a move, a loot, a split, one setter call); the inventory state a listener reads is the
 * finished state. On a client a batch is one replication update, flushed from PostNetReceive after every replicated array has
 * been applied. Revision is the inventory's Revision at that point: it rises by one per server operation that changed anything,
 * and is replicated, so a client can compare it with the revision an ack names.
 *
 * Deltas are listed in the order the changes were made. ReplayOrder interleaves the two lists in that order (a value >= 0 is an
 * index into SlotDeltas, a negative value v is index ~v into ItemDeltas); the legacy per-delta delegates are replayed from it.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryChangeBatch
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rock|Inventory")
	TObjectPtr<URockInventory> Inventory = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Rock|Inventory")
	int32 Revision = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rock|Inventory")
	TArray<FRockSlotDelta> SlotDeltas;

	UPROPERTY(BlueprintReadOnly, Category = "Rock|Inventory")
	TArray<FRockItemDelta> ItemDeltas;

	UPROPERTY()
	TArray<int32> ReplayOrder;

	bool IsEmpty() const { return ReplayOrder.IsEmpty(); }

	void Reset()
	{
		SlotDeltas.Reset();
		ItemDeltas.Reset();
		ReplayOrder.Reset();
	}

	void AddSlotDelta(const FRockSlotDelta& Delta)
	{
		ReplayOrder.Add(SlotDeltas.Add(Delta));
	}

	void AddItemDelta(const FRockItemDelta& Delta)
	{
		ReplayOrder.Add(~ItemDeltas.Add(Delta));
	}
};
