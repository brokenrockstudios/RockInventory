// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockSlotHandle.generated.h"

/**
 * Handle for an inventory slot position.
 * Uses a packed 32-bit format:
 * - Bits 0-7: X coordinate (0-255)
 * - Bits 8-15: Y coordinate (0-255)
 * - Bits 16-23: Tab index (0-255)
 * - Bits 24-31: Reserved for future use
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlotHandle
{
	GENERATED_BODY()

	// Bit masks and shifts
	static constexpr uint32 X_BITS = 8;
	static constexpr uint32 Y_BITS = 8;
	static constexpr uint32 TAB_BITS = 8;
	
	static constexpr uint32 X_MASK = (1 << X_BITS) - 1;
	static constexpr uint32 Y_MASK = (1 << Y_BITS) - 1;
	static constexpr uint32 TAB_MASK = (1 << TAB_BITS) - 1;
	
	static constexpr uint32 Y_SHIFT = X_BITS;
	static constexpr uint32 TAB_SHIFT = Y_SHIFT + Y_BITS;

	// Default constructor is an invalid handle
	FRockInventorySlotHandle();
	FRockInventorySlotHandle(uint8 InTabIndex, uint8 InX, uint8 InY);
	virtual ~FRockInventorySlotHandle() = default;

	/** The packed handle value */
	UPROPERTY()
	uint32 Handle = INDEX_NONE;

	/** Get the tab index from the handle */
	uint8 GetTabIndex() const { return (Handle >> TAB_SHIFT) & TAB_MASK; }
	
	/** Get the X coordinate from the handle */
	uint8 GetX() const { return Handle & X_MASK; }
	
	/** Get the Y coordinate from the handle */
	uint8 GetY() const { return (Handle >> Y_SHIFT) & Y_MASK; }

	uint32 GetIndex() const { return Handle; }
	
	/** Get the relative index within the tab (Y * Width + X) */
	uint32 GetRelativeIndex(uint32 TabWidth) const 
	{ 
		return GetY() * TabWidth + GetX(); 
	}

	/** Get the absolute index in the slot array given the tab's starting index */
	uint32 GetAbsoluteIndex(uint32 TabStartIndex, uint32 TabWidth) const
	{
		return TabStartIndex + GetRelativeIndex(TabWidth);
	}

	/** Set the handle values */
	void Set(uint8 InTabIndex, uint8 InX, uint8 InY)
	{
		Handle = (InTabIndex & TAB_MASK) << TAB_SHIFT |
				(InY & Y_MASK) << Y_SHIFT |
				(InX & X_MASK);
	}

	bool IsValid() const { return Handle != INDEX_NONE; }

	// Helper Utility functions
	friend uint32 GetTypeHash(const FRockInventorySlotHandle& Handle);
	virtual uint32 GetHash() const;
	FString ToString() const;
	bool operator==(const FRockInventorySlotHandle& OtherSlotHandle) const = default;
};
