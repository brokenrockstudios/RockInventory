// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

bool FRockSlotHandle::operator==(const FRockSlotHandle& Other) const
{
	return TabIndex == Other.TabIndex && X == Other.X && Y == Other.Y && bInitialized == Other.bInitialized;
}

bool FRockSlotHandle::IsValid() const
{
	return bInitialized;
}
