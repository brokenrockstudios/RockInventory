// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemOrientation.h"
#include "RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockInventorySlot.generated.h"

USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlot : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	// The actual item in this slot
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStack Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERockItemOrientation Orientation = ERockItemOrientation::Horizontal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsLocked = false;

	//////////////////////////////////////////////////////////////////////////
	// Which slot this is in the inventory
	// Do we also want to have a reference to the inventory?
	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;
	//////////////////////////////////////////////////////////////////////////

	void Reset();
	
	// Required for FFastArraySerializerItem
	void PreReplicatedRemove(const struct FRockInventoryData& InArraySerializer);
	void PostReplicatedAdd(const struct FRockInventoryData& InArraySerializer);
	void PostReplicatedChange(const struct FRockInventoryData& InArraySerializer);
};
