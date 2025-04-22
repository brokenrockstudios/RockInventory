// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "RockLoadItemTransaction.generated.h"

/**
 * If loading from some 'saved' state, this might 'bypass' some restrictions that GrantItem, MoveItem, or PickupItem might have
 * TBD: Though ultimately we might not want to use the Transaction system for this behavior. 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockLoadItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()
};
