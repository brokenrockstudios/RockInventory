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


// Deprecated: This function is not used in the current implementation.
// It was originally intended to generate a new handle for the item stack.
// But instead it should be based upon the index and generation of the item stack from the global pool.
//
// void FRockItemStackHandle::GenerateNewHandle()
// {
// 	static uint32 GIndex(1);
// 	static uint8 GGeneration(1);
// 	static FCriticalSection HandleLock;
// 	FScopeLock Lock(&HandleLock);
// 	
// 	// Calculate the new handle value: combine index and generation
// 	Handle = (GIndex & INDEX_MASK) | ((uint32)GGeneration << GENERATION_SHIFT);
// 	
// 	// Increment the index, and if it would exceed our mask, wrap around
// 	GIndex++;
// 	if ((GIndex & INDEX_MASK) == 0)
// 	{
// 		GIndex = 1; // Reset to 1 (0 is reserved as part of INVALID_HANDLE)
// 		// Increment generation when we wrap around
// 		GGeneration++;
// 		// If generation overflows, it'll automatically wrap to 0 (uint8)
// 		if (GGeneration == 0)
// 		{
// 			GGeneration = 1; // Skip 0 generation as it's used in INVALID_HANDLE
// 		}
// 	}
// }
