// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "RockLootIntent.generated.h"

/**
 * What a loot call may do with an item, and what a section accepts. Flags: a section is a candidate for a call when the two share a bit.
 * Store puts the item away (pockets, backpack); Equip puts it where it is worn or wielded.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ERockLootIntent : uint8
{
	None = 0 UMETA(Hidden),
	Store = 1 << 0,
	Equip = 1 << 1,
};
ENUM_CLASS_FLAGS(ERockLootIntent);
