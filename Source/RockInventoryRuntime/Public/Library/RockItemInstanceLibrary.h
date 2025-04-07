// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockInventorySlot.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockItemInstanceLibrary.generated.h"

class URockItemInstance;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockItemInstanceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Rock|Inventory|ItemInstance")
	static bool FindItemStackForInstance(const URockItemInstance* InstanceToFind, FRockItemStack& OutItemStack);
	
	UFUNCTION(BlueprintCallable, Category = "Rock|Inventory|ItemInstance")
	static bool FindItemSlotForInstance(const URockItemInstance* InstanceToFind, FRockInventorySlot& OutItemSlot);
};
