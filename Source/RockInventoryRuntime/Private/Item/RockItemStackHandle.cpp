#include "Item/RockItemStackHandle.h"

FRockItemStackHandle::FRockItemStackHandle() :
	Handle(INDEX_NONE)
{
}

FRockItemStackHandle FRockItemStackHandle::Create(uint32 InIndex, uint32 InGeneration)
{
	FRockItemStackHandle Result;

	// Ensure the index is within valid range
	InIndex = InIndex & INDEX_MASK;

	// Ensure generation is within valid range
	InGeneration = InGeneration & GENERATION_MASK;

	// Combine the index and generation
	Result.Handle = InIndex | (InGeneration << GENERATION_SHIFT);

	return Result;
}

bool FRockItemStackHandle::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	Ar << Handle;
	
	return true;
}
