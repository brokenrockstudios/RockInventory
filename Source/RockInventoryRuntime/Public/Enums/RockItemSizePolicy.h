// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

// This allows us to have an 'unrestricted' item size in a 1x1 slot of a section, such as for equipment or hot bar slots.
UENUM(BlueprintType)
enum class ERockItemSizePolicy : uint8
{
	// Standard grid rules
	RespectSize,
	// Ignores the size of the item when checking for placement. Treats items like a 1x1 item.
	IgnoreSize
};
