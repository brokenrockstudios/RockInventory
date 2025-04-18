// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Transactions/Core/RockInventoryTransaction.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventoryManagerLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryManagerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction")
	static void EnqueueTransaction(UObject* Object, URockInventoryTransaction* transaction);
};
