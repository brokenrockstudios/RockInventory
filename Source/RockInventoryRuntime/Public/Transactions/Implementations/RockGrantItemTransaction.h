// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "RockGrantItemTransaction.generated.h"

/**
 * To spawn an item directly into a player's inventory. Such as from a quest or NPC or admin tools
 * In theory this could/should either fall back to it dropping on the ground or possibly be 'mailed' to the player.
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockGrantItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()
};
