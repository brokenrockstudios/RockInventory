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
	FRockInventorySlotHandle(int32 InSectionIndex, int32 InAbsoluteSlotIndex);

private:
	UPROPERTY(VisibleAnywhere)
	int32 Index;
	UPROPERTY(VisibleAnywhere)
	int32 Section;

public:
	/** Get the tab index from the handle */
	int32 GetSectionIndex() const { return Section; }
	int32 GetIndex() const { return Index; }
	bool IsValid() const { return Index != INDEX_NONE && Section != INDEX_NONE; }

	// Helper Utility functions
	uint32 GetHash() const { return HashCombine(Index, Section); }
	friend uint32 GetTypeHash(const FRockInventorySlotHandle& Slot) { return Slot.GetHash(); }
	FString ToString() const;

	friend FArchive& operator<<(FArchive& Ar, FRockInventorySlotHandle& SlotHandle)
	{
		Ar << SlotHandle.Index;
		Ar << SlotHandle.Section;
		return Ar;
	}

	bool operator==(const FRockInventorySlotHandle& Other) const { return Index == Other.Index && Section == Other.Section; }
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
