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
	// We went from this slot having an Invalid ItemHandle to a valid one.
	ItemAdded UMETA(DisplayName = "ItemAdded"),
	// We went from a valid ItemHandle to an Invalid one
	ItemRemoved UMETA(DisplayName = "ItemRemoved"),
	// We went from one valid ItemHandle to another valid ItemHandle
	ItemChanged UMETA(DisplayName = "ItemSwapped"),
	
	// The slot changed state (e.g. orientation, lock, item swapped, or out of scope something 'changed' about this slot)
	PropertiesChanged UMETA(DisplayName = "Properties Changed"),
};
// { None=0, ItemRef=1<<0, Orientation=1<<1, Locked=1<<2, Anchor=1<<3 };
ENUM_CLASS_FLAGS(ERockSlotChangeType);
