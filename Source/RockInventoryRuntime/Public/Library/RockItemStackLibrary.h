// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemStack.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockItemStackLibrary.generated.h"

class URockInventory;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockItemStackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the item definition for this item stack */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|ItemStack")
	static URockItemDefinition* GetItemDefinition(const FName& ItemId);

	/** Returns the size of the item stack in slots */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|ItemStack")
	static FVector2D GetItemSize(const FRockItemStack& ItemStack);

	/** Returns true if this stack can be combined with another stack */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|ItemStack")
	static bool CanStackWith(const FRockItemStack& FirstItem, const FRockItemStack& SecondItem);

	/** Returns the maximum stack size for this item stack */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|ItemStack")
	static int32 GetMaxStackSize(const FRockItemStack& ItemStack);

	/** Returns true if the stack is at its maximum size */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|ItemStack")
	static bool IsFull(const FRockItemStack& ItemStack);
};
