// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

FRockInventorySlotHandle::FRockInventorySlotHandle()
	: Index(INDEX_NONE)
	  , Section(INDEX_NONE)
{
}

FRockInventorySlotHandle::FRockInventorySlotHandle(int32 InSectionIndex, int32 InAbsoluteSlotIndex)
	: Index(InAbsoluteSlotIndex)
	  , Section(InSectionIndex)
{
}

FString FRockInventorySlotHandle::ToString() const
{
	return FString::Printf(TEXT("SlotHandle(%d, Valid=%s)"), GetIndex(), IsValid() ? TEXT("true") : TEXT("false"));
}

uint32 FRockInventorySlotHandle::GetHash() const
{
	return GetTypeHash(*this);
}

uint32 GetTypeHash(const FRockInventorySlotHandle& Slot)
{
	return HashCombine(Slot.Index, Slot.Section);
}
