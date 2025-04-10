// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventorySlot.h"


void FRockInventorySlot::Reset()
{
	Item.Reset();
	Orientation = ERockItemOrientation::Horizontal;
	// Do we want to reset isLocked?
	bIsLocked = false;
}

void FRockInventorySlot::PreReplicatedRemove(const struct FRockInventoryData& InArraySerializer)
{
}

void FRockInventorySlot::PostReplicatedAdd(const struct FRockInventoryData& InArraySerializer)
{
}

void FRockInventorySlot::PostReplicatedChange(const struct FRockInventoryData& InArraySerializer)
{
}
