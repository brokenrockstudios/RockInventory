// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

/**
 *
 * Dependency: Enum must be ordered TL=0, TR=1, BL=2, BR=3
 */
UENUM(BlueprintType)
enum class ERockInventory_TileQuadrant : uint8
{
	TopLeft = 0,
	TopRight = 1,
	BottomLeft = 2,
	BottomRight = 3,
	None
};
