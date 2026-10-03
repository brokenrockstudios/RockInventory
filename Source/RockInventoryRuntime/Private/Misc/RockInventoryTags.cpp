// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Misc/RockInventoryTags.h"

namespace RockInventoryTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Common, "Item.Rarity.Common", "Common item rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Uncommon, "Item.Rarity.Uncommon", "Uncommon item rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Rare, "Item.Rarity.Rare", "Rare item rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Epic, "Item.Rarity.Epic", "Epic item rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Legendary, "Item.Rarity.Legendary", "Legendary item rarity");

	// "Pockets" is plural on purpose: one section holding several pockets. Later variants would be subtags (Pockets.Left, ...).
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory_Section_Backpack, "Inventory.Section.Backpack", "Main carried storage section");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory_Section_Pockets, "Inventory.Section.Pockets", "Small quick-access pockets section");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory_Section_Storage, "Inventory.Section.Storage", "Stationary storage (chests, lockers)");
}
