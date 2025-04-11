// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemOrientation.h"
#include "RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockInventorySlot.generated.h"

/**
 * Represents a single slot in an inventory grid.
 * 
 * Each slot can contain an item stack and has properties like orientation and lock state.
 * The slot is part of a larger inventory system and maintains its position through a SlotHandle.
 * 
 * This struct is designed for efficient network replication and cache-friendly memory layout.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlot : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	/** The actual item in this slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStack Item;

	/** The orientation of the item in this slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERockItemOrientation Orientation = ERockItemOrientation::Horizontal;

	/** Whether this slot is locked (preventing item movement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsLocked = false;

	/** Handle to identify this slot's position in the inventory */
	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;

	/** Resets the slot to its default state */
	void Reset();
	
	/** Required for FFastArraySerializerItem */
	void PreReplicatedRemove(const struct FRockInventoryData& InArraySerializer);
	void PostReplicatedAdd(const struct FRockInventoryData& InArraySerializer);
	void PostReplicatedChange(const struct FRockInventoryData& InArraySerializer);

	/** Validates the slot's state */
	bool IsValid() const;

	/** Returns true if the slot is empty */
	bool IsEmpty() const;

	/** Returns true if the slot can accept the given item */
	bool CanAcceptItem(const FRockItemStack& NewItem) const;
};
