// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockSlotHandle.generated.h"

USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlotHandle
{
	GENERATED_BODY()
	// Default constructor is an invalid handle
	FRockInventorySlotHandle()
		: Index(INDEX_NONE)
		  , Section(INDEX_NONE)
	{
	}

	// Constructor with SectionIndex and absolute index into Inventory's SlotData
	FRockInventorySlotHandle(int32 InSectionIndex, int32 InAbsoluteSlotIndex)
		: Index(InAbsoluteSlotIndex)
		  , Section(InSectionIndex)
	{
	}

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
	friend uint32 GetTypeHash(const FRockInventorySlotHandle& Slot)
	{
		return HashCombine(Slot.Index, Slot.Section);
	}

	uint32 GetHash() const;
	FString ToString() const;
	bool operator==(const FRockInventorySlotHandle& OtherSlotHandle) const = default;
};
