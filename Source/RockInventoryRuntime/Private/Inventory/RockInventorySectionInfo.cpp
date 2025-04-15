// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventorySectionInfo.h"

int32 FRockInventorySectionInfo::GetNumSlots() const
{
	return Width * Height;
}

int32 FRockInventorySectionInfo::GetWidth() const
{
	return Width;
}

int32 FRockInventorySectionInfo::GetHeight() const
{
	return Height;
}
