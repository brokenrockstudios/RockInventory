// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

FRockInventorySlotHandle FRockInventorySlotHandle::Invalid()
{
	static FRockInventorySlotHandle InvalidHandle;
	// Force Initialized as false for an invalid handle.
	InvalidHandle.bInitialized = false;
	return InvalidHandle;
}

FRockInventorySlotHandle::FRockInventorySlotHandle(uint8 InTabIndex, uint8 InX, uint8 InY)
	: TabIndex(InTabIndex)
	  , X(InX)
	  , Y(InY)
{
	bInitialized = true;
}

bool FRockInventorySlotHandle::operator==(const FRockInventorySlotHandle& Other) const
{
	return TabIndex == Other.TabIndex && X == Other.X && Y == Other.Y && bInitialized == Other.bInitialized;
}

bool FRockInventorySlotHandle::IsValid() const
{
	return bInitialized;
}

uint32 GetTypeHash(const FRockInventorySlotHandle& Handle)
{
	uint32 Hash = 0;
	Hash = HashCombine(Hash, GetTypeHash(Handle.TabIndex));
	Hash = HashCombine(Hash, GetTypeHash(Handle.X));
	Hash = HashCombine(Hash, GetTypeHash(Handle.Y));
	return Hash;
}
