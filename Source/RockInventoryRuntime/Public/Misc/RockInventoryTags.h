// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "UObject/Object.h"


// TODO: Move this into more game specific module.
// WARN: This type of code really shouldn't be here in this plugin. MOVE IT!

namespace RockInventoryTags
{
// Declare all the custom native tags that Rock Modular GameplayAbilities will use
ROCKINVENTORYRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Rarity_Common);
ROCKINVENTORYRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Rarity_Uncommon);
ROCKINVENTORYRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Rarity_Rare);
ROCKINVENTORYRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Rarity_Epic);
ROCKINVENTORYRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Rarity_Legendary);
}
