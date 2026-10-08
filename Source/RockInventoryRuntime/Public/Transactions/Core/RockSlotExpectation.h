// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enums/RockItemOrientation.h"
#include "Inventory/RockSlotHandle.h"

#include "RockSlotExpectation.generated.h"

class URockInventory;
class URockItemDefinition;
struct FRockInventoryData;
struct FRockItemStack;

/**
 * What a client command expects to find in one slot when it runs (T-78): the precondition that gives optimistic concurrency.
 * A command says what to do and what the client saw; the server refuses it when the slot holds something else (another player
 * took the item, the client's picture was stale) instead of guessing.
 *
 * Content, not handle: a stack a pending command created on the client has a handle only the client knows until the item index
 * allocation is deterministic (T-79), so the definition, count, custom values and the slot's orientation are compared. Item
 * instances are not compared (a moved instanced item is a whole-stack move).
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockSlotExpectation
{
	GENERATED_BODY()

	/** False: nothing was captured and Matches always passes (commands built by code that does not predict). */
	UPROPERTY()
	bool bCheck = false;
	/** The slot held no stack. */
	UPROPERTY()
	bool bEmpty = true;
	UPROPERTY()
	TObjectPtr<URockItemDefinition> Definition = nullptr;
	UPROPERTY()
	int32 Count = 0;
	UPROPERTY()
	int32 CustomValue1 = 0;
	UPROPERTY()
	int32 CustomValue2 = 0;
	UPROPERTY()
	ERockItemOrientation Orientation = ERockItemOrientation::Horizontal;

	/** Captures what the slot holds now. */
	static FRockSlotExpectation Capture(const FRockInventoryData& Data, const FRockInventorySlotHandle& Slot);
	static FRockSlotExpectation Capture(const URockInventory& Inventory, const FRockInventorySlotHandle& Slot);

	/** Does the slot still hold what was captured? Always true when nothing was captured. */
	bool Matches(const FRockInventoryData& Data, const FRockInventorySlotHandle& Slot) const;
	bool Matches(const URockInventory& Inventory, const FRockInventorySlotHandle& Slot) const;

private:
	static FRockSlotExpectation FromStack(const FRockItemStack* Stack, ERockItemOrientation SlotOrientation);
	bool MatchesStack(const FRockItemStack* Stack, ERockItemOrientation SlotOrientation) const;
};
