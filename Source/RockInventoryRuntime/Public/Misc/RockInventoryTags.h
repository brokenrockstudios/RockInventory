// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "UObject/Object.h"


// TODO: Move this into more game specific module.
// WARN: This type of code really shouldn't be here in this plugin. MOVE IT!

#define TAG_EXTERN(Name) ROCKINVENTORYRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Name)

namespace RockInventoryTags
{
	// Declare all the custom native tags that Rock Modular GameplayAbilities will use
	TAG_EXTERN(Item_Rarity_Common);
	TAG_EXTERN(Item_Rarity_Uncommon);
	TAG_EXTERN(Item_Rarity_Rare);
	TAG_EXTERN(Item_Rarity_Epic);
	TAG_EXTERN(Item_Rarity_Legendary);

	// General sections. Game-specific sections (Equipment.*, Process.*, ...) live in the game module.
	TAG_EXTERN(Inventory_Section_Backpack);
	TAG_EXTERN(Inventory_Section_Pockets);
	TAG_EXTERN(Inventory_Section_Storage);
}

#undef TAG_EXTERN
