// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemStack.h"
#include "UObject/Interface.h"
#include "RockWorldItemInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class URockWorldItemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ROCKINVENTORYRUNTIME_API IRockWorldItemInterface
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RockInventory")
	void SetItemStack(const FRockItemStack& InItemStack);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RockInventory")
	FRockItemStack GetItemStack() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RockInventory")
	void PickedUp(AActor* Instigator);


	
};
