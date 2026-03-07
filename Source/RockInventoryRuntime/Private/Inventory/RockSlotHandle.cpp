// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

FRockInventorySlotHandle::FRockInventorySlotHandle()
	: Index(INDEX_NONE)
{
}

FRockInventorySlotHandle::FRockInventorySlotHandle(int32 InAbsoluteSlotIndex)
	: Index(InAbsoluteSlotIndex)
{
}

FRockInventorySlotHandle FRockInventorySlotHandle::Invalid()
{
	return FRockInventorySlotHandle();
}

FString FRockInventorySlotHandle::ToString() const
{
	if (!IsValid())
	{
		return TEXT("SlotHandle[Invalid]");
	}
	return FString::Printf(TEXT("SlotHandle[Index:%d]"), Index);
}
