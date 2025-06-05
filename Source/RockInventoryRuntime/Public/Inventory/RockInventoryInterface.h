// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RockInventoryInterface.generated.h"

class URockInventory;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URockInventoryInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ROCKINVENTORYRUNTIME_API IRockInventoryInterface
{
	GENERATED_BODY()

public:
	virtual URockInventory* GetInventory() const = 0;
	
};


