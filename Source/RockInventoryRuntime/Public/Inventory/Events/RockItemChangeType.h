// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RockItemChangeType.generated.h"

// WARNING: We currently aren't using these types correctly. Add could mean a new slot OR a new item.
// NOTE: There are bugs and other issues with this current design/enum. It needs to be rethought and redesigned.
/**
 * Describes what kind of change occurred in the inventory.
 * Designed as flags so multiple change types can be combined.
 */
UENUM(BlueprintType)
enum class ERockItemChangeType : uint8
{
	None UMETA(DisplayName = "None"),
	
	// A new item was placed into a previously empty slot
	Added UMETA(DisplayName = "Added"),
	
	// An item was removed from a slot, leaving it empty
	Removed UMETA(DisplayName = "Removed"),
	
	// It might not be possible to determine what exactly changed
	Changed UMETA(DisplayName = "Changed"),
};

ENUM_CLASS_FLAGS(ERockItemChangeType);
