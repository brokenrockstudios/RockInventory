// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockSlotHandle.generated.h"

USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlotHandle
{
	GENERATED_BODY()

	FRockInventorySlotHandle();
	/** Creates a slot handle with the given absolute slot index. */
	explicit FRockInventorySlotHandle(int32 InAbsoluteSlotIndex);

private:
	UPROPERTY(VisibleAnywhere)
	int32 Index;

public:
	// Get the absolute slot index from the handle
	int32 GetAbsoluteIndex() const { return Index; }
	/**	Creates an invalid handle */
	static FRockInventorySlotHandle Invalid();
	bool IsValid() const { return Index != INDEX_NONE; }

	// Helper Utility functions
	uint32 GetHash() const { return GetTypeHash(Index); }
	friend uint32 GetTypeHash(const FRockInventorySlotHandle& Slot) { return Slot.GetHash(); }
	FString ToString() const;

	friend FArchive& operator<<(FArchive& Ar, FRockInventorySlotHandle& SlotHandle)
	{
		Ar << SlotHandle.Index;
		return Ar;
	}

	bool operator==(const FRockInventorySlotHandle& Other) const { return Index == Other.Index; }
	bool operator!=(const FRockInventorySlotHandle& Other) const { return !(*this == Other); }

	/** Network serialization that compresses the values */
};

template <>
struct TStructOpsTypeTraits<FRockInventorySlotHandle> : public TStructOpsTypeTraitsBase2<FRockInventorySlotHandle>
{
	enum
	{
		WithIdenticalViaEquality = true
	};
};
