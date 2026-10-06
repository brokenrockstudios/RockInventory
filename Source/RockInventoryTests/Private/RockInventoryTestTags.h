// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/** Tags registered by the RockInventoryTests module at startup. Valid inside any test body, not in static initializers. */
namespace RockInventoryTestTags
{
	extern FGameplayTag Weapon;
	extern FGameplayTag Food;
	extern FGameplayTag MetaA;
	extern FGameplayTag MetaB;
	/** Item tags for the loot placement tests. */
	extern FGameplayTag Sidearm;
	extern FGameplayTag Wieldable;
	extern FGameplayTag Headgear;
	/** Section tags for equipment-style layouts (Head, Primary, Secondary). */
	extern FGameplayTag SectionHead;
	extern FGameplayTag SectionPrimary;
	extern FGameplayTag SectionSecondary;
	/** Twelve distinct section tags, for layouts with many sections. */
	extern TArray<FGameplayTag> BulkSections;
}
