// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_CompositeBase.h"
#include "RockInventory_Leaf.generated.h"

/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_Leaf : public URockInventory_CompositeBase
{
	GENERATED_BODY()
public:
	virtual void ApplyFunction(FuncType Function) override;
};
