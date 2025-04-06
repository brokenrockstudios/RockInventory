// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemOrientation.h"
#include "Item/RockItemStack.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockInventorySlot.generated.h"

USTRUCT(BlueprintType)
struct ROCKINVENTORY_API FRockInventorySlot : public FFastArraySerializerItem
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
	// NOTE: Consider removing. This is somewhat redundant?
	// Which tab this slot belongs to. 0-255
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 TabIndex = 0;
	// Position within the tab grid, 0-255
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 GridX = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 GridY = 0;
	//////////////////////////////////////////////////////////////////////////

	// Required for FFastArraySerializerItem
	void PreReplicatedRemove(const struct FRockInventoryData& InArraySerializer);
	void PostReplicatedAdd(const struct FRockInventoryData& InArraySerializer);
	void PostReplicatedChange(const struct FRockInventoryData& InArraySerializer);
};

