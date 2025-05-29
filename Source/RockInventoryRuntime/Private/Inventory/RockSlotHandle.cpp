// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

// All this just to save 4 bytes per slot:P 
// But when you consider hundreds of slots, it can add up.
namespace
{
	/** Bit counts for network serialization */
	constexpr uint32 NET_INDEX_BITS = 22; // 22 bits = 4.1 million unique slots
	constexpr uint32 NET_SECTION_BITS = 10; // 10 bits = 1024 sections

	/** Bit masks and shifts for network serialization */
	constexpr uint32 NET_INDEX_MASK = (1 << NET_INDEX_BITS) - 1;
	constexpr uint32 NET_SECTION_SHIFT = NET_INDEX_BITS;
	constexpr uint32 NET_SECTION_MASK = ((1 << NET_SECTION_BITS) - 1);
	constexpr uint32 NET_SECTION_HANDLE_MASK = NET_SECTION_MASK << NET_SECTION_SHIFT;

	// Compile-time check to ensure we don't exceed 32 bits for network serialization
	static_assert(NET_INDEX_BITS + NET_SECTION_BITS == 32, "Network bit allocation exceeds 32 bits");
}

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
	if (!IsValid())
	{
		return TEXT("Invalid");
	}
	return FString::Printf(TEXT("SlotHandle[Index:%d,Section:%d]"), Index, Section);
}
