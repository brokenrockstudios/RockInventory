// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemStackHandle.h"

FRockItemStackHandle::FRockItemStackHandle() :
	Handle(INDEX_NONE)
{
}

void FRockItemStackHandle::Reset()
{
	Handle = INDEX_NONE;
}

FRockItemStackHandle FRockItemStackHandle::Create(uint32 InIndex, uint32 InGeneration)
{
	// An index that does not fit would alias another slot, so it yields an invalid handle instead.
	if (InIndex > INDEX_MASK)
	{
		return Invalid();
	}

	FRockItemStackHandle Result;

	// Generation is expected to wrap, so extra bits are dropped. Ensure generation is within valid range
	InGeneration = InGeneration & GENERATION_MASK;

	// Combine the index and generation
	Result.Handle = InIndex | (InGeneration << GENERATION_SHIFT);

	return Result;
}

uint32 FRockItemStackHandle::NextGeneration(uint32 InGeneration)
{
	const uint32 Next = (InGeneration + 1) & GENERATION_MASK;
	return Next == GENERATION_MASK ? 0 : Next;
}

FRockItemStackHandle FRockItemStackHandle::Invalid()
{
	return FRockItemStackHandle();
}

bool FRockItemStackHandle::IsValid() const
{
	return Handle != INDEX_NONE;
}

int32 FRockItemStackHandle::GetIndex() const
{
	return Handle & INDEX_MASK;
}

int32 FRockItemStackHandle::GetGeneration() const
{
	return (Handle & GENERATION_HANDLE_MASK) >> GENERATION_SHIFT;
}

FString FRockItemStackHandle::ToString() const
{
	if (!IsValid())
	{
		return TEXT("Invalid");
	}
	return FString::Printf(TEXT("ItemHandle[Index:%u,Gen:%u]"), GetIndex(), GetGeneration());
}

uint32 FRockItemStackHandle::GetHash() const
{
	return GetTypeHash(Handle);
}

bool FRockItemStackHandle::operator==(const FRockItemStackHandle& OtherSlotHandle) const
{
	return Handle == OtherSlotHandle.Handle;
}

bool FRockItemStackHandle::operator!=(const FRockItemStackHandle& OtherSlotHandle) const
{
	return !(*this == OtherSlotHandle);
}

FRockItemStackHandle::operator bool() const
{
	return IsValid();
}
