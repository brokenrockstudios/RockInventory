// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ERockItemOrientation : uint8
{
	Horizontal = 0,
	Vertical = 1,
};

// an enum to help determine if the stack can be fully merged, partially, or not at all
UENUM(BlueprintType)
enum class ERockItemStackMergeCondition : uint8
{
	// Will only return if it can be fully merged  
	Full = 0,
	// Will only return true if it can only be partially merged
	Partial = 1,
	// will return false if it cannot be merged at all
	None = 2,
};
