// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockSlotHandle.h"
#include "Item/RockItemStackHandle.h"
#include "RockPendingSlotOperation.generated.h"

UENUM(BlueprintType)
enum class ERockSlotStatus : uint8
{
	Empty,             // Truly empty.
	Pending, 		   // Someone is moving or interacting with this item or slot right now. Slot is visually occupied.

	// Maybe we want to differnetiate this later
	// Occupied,          // Normal: An item is fully placed and usable.
	// PendingEmpty,      // Someone is moving or interacting with this item right now. Slot is visually empty, but logically reserved.
	// PendingIncoming,   // A slot targeted for dropping an incoming item, if you want reservations. (e.g. A NPC is dropping an item at this location)
};

/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockPendingSlotOperation
{
	GENERATED_BODY()

	UPROPERTY()
	ERockSlotStatus SlotStatus = ERockSlotStatus::Empty;
	
	// Which controller is doing the operation
	UPROPERTY()
	TObjectPtr<AController> Controller = nullptr;
	
	// The slot handle that is being moved
	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;

	// The item handle that is being moved
	UPROPERTY()
	FRockItemStackHandle ItemHandle;

	// For timeout purposes. 
	UPROPERTY()
	double TimeStarted = 0.0;

	// TODO Move to UFunction Library, so we can have a UFUNCTION on this.
	static bool CanClaimSlot(const FRockPendingSlotOperation& SlotOperation)
	{
		switch (SlotOperation.SlotStatus)
		{
		case ERockSlotStatus::Empty:
		//case ERockSlotStatus::Occupied:
			return true;
		case ERockSlotStatus::Pending:
		//case ERockSlotStatus::PendingEmpty:
		//case ERockSlotStatus::PendingIncoming:
			return false;
		default:
			return false;
		}
	}

	bool IsClaimedByOther(AController* OtherController) const
	{
		return SlotStatus == ERockSlotStatus::Pending && Controller != OtherController;
	}
};