// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockInventoryTabInfo.generated.h"

/**
 *
 */
// Tab dimension info
USTRUCT(BlueprintType)
struct ROCKINVENTORY_API FRockInventoryTabInfo
{
	GENERATED_BODY()
	
	// Grid dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Width = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Height = 0;

	// Optional name for debugging/UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TabID;

	// First slot index in the AllSlots array
	UPROPERTY()
	int32 FirstSlotIndex = 0;

	// Total number of slots in this tab
	UPROPERTY()
	int32 NumSlots = 0;
};
