// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

FRockInventorySlotHandle::FRockInventorySlotHandle()
	: Handle(INDEX_NONE)
{
}

FRockInventorySlotHandle::FRockInventorySlotHandle(uint8 InTabIndex, uint8 InX, uint8 InY)
{
	Set(InTabIndex, InX, InY);
}

FString FRockInventorySlotHandle::ToString() const
{
	return FString::Printf(TEXT("SlotHandle(Tab=%d, X=%d, Y=%d, Valid=%s)"), 
		GetTabIndex(), GetX(), GetY(), IsValid() ? TEXT("true") : TEXT("false"));
}

uint32 GetTypeHash(const FRockInventorySlotHandle& Handle)
{
	return GetTypeHash(Handle.Handle);
}

uint32 FRockInventorySlotHandle::GetHash() const
{
	return GetTypeHash(*this);
}
