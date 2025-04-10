// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemStack.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockItemStackLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockItemStackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "RockInventory|ItemStack")
	static URockItemDefinition* GetItemDefinition(const UObject* WorldContextObject, const FName& ItemId);


	static FVector2D GetItemSize(const FRockItemStack& ItemStack);

	static FRockItemStack CreateItemStack(const FRockItemStack& InItemStack);
};
