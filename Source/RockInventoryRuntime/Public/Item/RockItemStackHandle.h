// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStackHandle.generated.h"

/**
 * Unique handle to a stack of items in the inventory system.
 * Provides a stable reference that can be used to locate and manipulate item stacks.
 * The handle combines an index (lower 24 bits) (16 million slots) and a generation ID (upper 8 bits) (255 slots)
 * to allow for stable references even when slots are reused.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemStackHandle
{
	GENERATED_BODY()

private:
	/** The unique identifier for this item stack (combines index and generation) */
	UPROPERTY()
	int32 Handle = INDEX_NONE;

public:
	/** Bit counts for index and generation components */
	static constexpr uint32 INDEX_BITS = 24; // 24 bits = 16 million unique items
	static constexpr uint32 GENERATION_BITS = 8; // 8 bits = 256 generations

	/** Bit masks and shifts based on bit counts */
	static constexpr uint32 INDEX_MASK = (1 << INDEX_BITS) - 1; // Masks the lower 24 bits
	static constexpr uint32 GENERATION_SHIFT = INDEX_BITS; // Bits to shift for generation
	static constexpr uint32 GENERATION_MASK = ((1 << GENERATION_BITS) - 1); // Maximum generation count (8 bits, 0-255)
	static constexpr uint32 GENERATION_HANDLE_MASK = GENERATION_MASK << GENERATION_SHIFT; // Masks the upper 8 bits in complete handle

	// Compile-time check to ensure we don't exceed 32 bits
	static_assert(INDEX_BITS + GENERATION_BITS == 32, "Bit allocation exceeds 32 bits");

	FRockItemStackHandle();
	void Reset();

	/**
	 * Creates a handle with specific index and generation values
	 * @param InIndex - The index component. 24 bits (0-16777215)
	 * @param InGeneration - The generation component. 8 bits (0-255)
	 */
	static FRockItemStackHandle Create(uint32 InIndex, uint32 InGeneration);

	/**
	 * Creates an invalid handle
	 * @return An invalid handle instance
	 */
	static FRockItemStackHandle Invalid();

	/** Returns true if this handle refers to a valid item stack */
	bool IsValid() const;

	/** Gets the index portion of the handle (lower 24 bits) */
	int32 GetIndex() const;

	/** Gets the generation portion of the handle (upper 8 bits) */
	int32 GetGeneration() const;

	/** Converts the handle to a human-readable string representation */
	FString ToString() const;

	///////////////////////////////////////////////////////////////////////////
	// Helper Utility functions

	/** Virtual hash function for derived classes */
	uint32 GetHash() const;
	friend uint32 GetTypeHash(const FRockItemStackHandle& ItemStackHandle) { return ItemStackHandle.GetHash(); }
	/** Serialization operator */
	friend FArchive& operator<<(FArchive& Ar, FRockItemStackHandle& ItemStackHandle)
	{
		Ar << ItemStackHandle.Handle;
		return Ar;
	}

	/** Equality comparison operator */
	bool operator==(const FRockItemStackHandle& OtherSlotHandle) const;
	bool operator!=(const FRockItemStackHandle& OtherSlotHandle) const;

	/** Explicit cast to bool for conditional expressions */
	explicit operator bool() const;
};

template <>
struct TStructOpsTypeTraits<FRockItemStackHandle> : public TStructOpsTypeTraitsBase2<FRockItemStackHandle>
{
	enum
	{
		WithIdenticalViaEquality = true
	};
};
