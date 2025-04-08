// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

bool FRockInventorySlotHandle::operator==(const FRockInventorySlotHandle& Other) const
{
	return TabIndex == Other.TabIndex && X == Other.X && Y == Other.Y && bInitialized == Other.bInitialized;
}

bool FRockInventorySlotHandle::IsValid() const
{
	return bInitialized;
}
