// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RockInventory_SpaceQueryResult.generated.h"

/**
 *
 */
USTRUCT()
struct FRockInventory_SpaceQueryResult
{
	GENERATED_BODY()

	// Upper left index of the space queried
	int32 UpperLeftIndex = INDEX_NONE;

	// True if the space queried has no items in it
	bool bHasSpace = false;

	// I don't know if I want to support item swapping
	// Valid if there's a single item we can swap with
	// TWeakObjectPtr<> ValidItem = nullptr;
};