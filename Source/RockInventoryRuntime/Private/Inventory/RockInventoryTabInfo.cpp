// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventoryTabInfo.h"

int32 FRockInventoryTabInfo::GetNumSlots() const
{
	return Width * Height;
}

int32 FRockInventoryTabInfo::GetWidth() const
{
	return Width;
}

int32 FRockInventoryTabInfo::GetHeight() const
{
	return Height;
}
