// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

FRockInventorySlotHandle::FRockInventorySlotHandle()
	: TabIndex(INDEX_NONE)
	  , X(INDEX_NONE)
	  , Y(INDEX_NONE)
{
}

FRockInventorySlotHandle::FRockInventorySlotHandle(uint8 InTabIndex, uint8 InX, uint8 InY)
	: TabIndex(InTabIndex)
	  , X(InX)
	  , Y(InY)
{
}

bool FRockInventorySlotHandle::IsValid() const
{
	return TabIndex != INDEX_NONE || X != INDEX_NONE || Y != INDEX_NONE;
}

FString FRockInventorySlotHandle::ToString() const
{
	return FString::Printf(TEXT("SlotHandle(Tab=%d, X=%d, Y=%d, Valid=%s)"), 
		TabIndex, X, Y, IsValid() ? TEXT("true") : TEXT("false"));
}

uint32 GetTypeHash(const FRockInventorySlotHandle& Handle)
{
	uint32 Hash = 0;
	Hash = HashCombine(Hash, GetTypeHash(Handle.TabIndex));
	Hash = HashCombine(Hash, GetTypeHash(Handle.X));
	Hash = HashCombine(Hash, GetTypeHash(Handle.Y));
	return Hash;
}

uint32 FRockInventorySlotHandle::GetHash() const
{
	return GetTypeHash(*this);
}
