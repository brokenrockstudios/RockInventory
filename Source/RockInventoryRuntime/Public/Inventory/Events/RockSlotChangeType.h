// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RockSlotChangeType.generated.h"

/**
 *
 */
UENUM(BlueprintType)
enum class ERockSlotChangeType : uint8
{
	// WARNING: We currently aren't using these types correctly. Add could mean a new slot OR a new item.
	// NOTE: There are bugs and other issues with this current design/enum. It needs to be rethought and redesigned.

	// Default/Unknown state
	None UMETA(DisplayName = "None"),
	// We went from an Invalid ItemHandle to a valid one
	Added UMETA(DisplayName = "Added"),
	// We went from a valid ItemHandle to an Invalid one
	Removed UMETA(DisplayName = "Removed"),
	// The slot changed properties (e.g. item, rotation, lock)
	Changed UMETA(DisplayName = "Properties Changed"),
	// The slot was cleared (e.g. item was split out)
	//Cleared UMETA(DisplayName = "Cleared")

	// Something is happening with this Slot, such that it's pending some action. Possibly being moved or locked or something.
	PendingChange UMETA(DisplayName = "Pending Slot Change") // Internal use only
};
// { None=0, ItemRef=1<<0, Orientation=1<<1, Locked=1<<2, Anchor=1<<3 };
ENUM_CLASS_FLAGS(ERockSlotChangeType);
