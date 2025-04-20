// Copyright 2025 Broken Rock Studios. All rights reserved.

#include "Inventory/RockSlotHandle.h"

// All this just to save 4 bytes per slot :P 
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
	return FString::Printf(TEXT("SlotHandle[Index:%d,Section:%d]"), Index, Section);
}

uint32 FRockInventorySlotHandle::GetHash() const
{
	return GetTypeHash(*this);
}

uint32 GetTypeHash(const FRockInventorySlotHandle& Slot)
{
	return HashCombine(Slot.Index, Slot.Section);
}

bool FRockInventorySlotHandle::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	
	if (Ar.IsSaving())
	{
		// When saving, compress the values into a single 32-bit integer
		uint32 PackedHandle = 0;
		
		// Only pack if the values are within our network bit range
		if (Index >= 0 && Index <= NET_INDEX_MASK && 
			Section >= 0 && Section <= NET_SECTION_MASK)
		{
			PackedHandle = (Index & NET_INDEX_MASK) | ((Section & NET_SECTION_MASK) << NET_SECTION_SHIFT);
		}
		else
		{
			// If values are too large, use a special invalid value
			PackedHandle = INDEX_NONE;
		}
		
		Ar << PackedHandle;
	}
	else if (Ar.IsLoading())
	{
		// When loading, unpack the values from the compressed format
		uint32 PackedHandle = 0;
		Ar << PackedHandle;
		
		if (PackedHandle == INDEX_NONE)
		{
			Index = INDEX_NONE;
			Section = INDEX_NONE;
		}
		else
		{
			Index = PackedHandle & NET_INDEX_MASK;
			Section = (PackedHandle & NET_SECTION_HANDLE_MASK) >> NET_SECTION_SHIFT;
		}
	}
	
	return true;
}
