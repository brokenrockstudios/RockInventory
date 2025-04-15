// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

FString FRockInventorySlotHandle::ToString() const
{
	return FString::Printf(TEXT("SlotHandle(%d, Valid=%s)"), GetIndex(), IsValid() ? TEXT("true") : TEXT("false"));
}

uint32 FRockInventorySlotHandle::GetHash() const
{
	return GetTypeHash(*this);
}
