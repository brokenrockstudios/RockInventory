// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockSlotHandle.h"
#include "Item/RockItemStackHandle.h"
#include "RockPendingSlotOperation.generated.h"

UENUM(BlueprintType)
enum class ERockSlotStatus : uint8
{
	Empty, // Truly empty.
	Pending, // Someone is moving or interacting with this item or slot right now. Slot is visually occupied.

	// Maybe we want to differentiate this later
	// Occupied,          // Normal: An item is fully placed and usable.
	// PendingEmpty,      // Someone is moving or interacting with this item right now. Slot is visually empty, but logically reserved.
	// PendingIncoming,   // A slot targeted for dropping an incoming item, if you want reservations. (e.g. A NPC is dropping an item at this location)
};

/** Represents an in-flight slot operation (e.g. a drag) used to prevent concurrent modifications.
 *  Locked slots remain visually occupied until the operation completes or times out. */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockPendingSlotOperation
{
	GENERATED_BODY()

	UPROPERTY()
	ERockSlotStatus SlotStatus = ERockSlotStatus::Empty;

	/** Controller that owns this operation. */
	UPROPERTY()
	TObjectPtr<AController> Controller = nullptr;

	/** The slot being locked. */
	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;

	/** The item involved in the operation. */
	UPROPERTY()
	FRockItemStackHandle ItemHandle;

	/** World time when the operation started; used for timeout detection. */
	UPROPERTY()
	double TimeStarted = 0.0;

	// TODO Move to UFunction Library, so we can have a UFUNCTION on this.
	/** Returns true if the slot is unclaimed and can be locked. */
	static bool CanClaimSlot(const FRockPendingSlotOperation& SlotOperation);
	
	/** Returns true if this slot is locked by a different controller. */
	bool IsClaimedByOther(AController* OtherController) const;
};

