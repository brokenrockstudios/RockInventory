// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "RockItem.h"

const URockItemDefinition* FRockItemStack::GetDefinition() const
{
	if (!CachedDefinition)
	{
		//ItemDefinition = UMyItemRegistry::Get()->FindDefinition(ItemID);
	}
	return CachedDefinition;
}
