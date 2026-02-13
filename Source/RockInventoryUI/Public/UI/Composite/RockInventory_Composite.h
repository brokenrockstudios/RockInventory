// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_CompositeBase.h"
#include "RockInventory_Composite.generated.h"

/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_Composite : public URockInventory_CompositeBase
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void ApplyFunction(FuncType Function) override;
	virtual void Collapse() override;
	
private:
	UPROPERTY()
	TArray<TObjectPtr<URockInventory_CompositeBase>> Children;
};

