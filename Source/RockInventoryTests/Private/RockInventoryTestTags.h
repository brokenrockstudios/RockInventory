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
}
