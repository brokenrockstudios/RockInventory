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

	UFUNCTION(BlueprintCallable, Category = "RockInventory|ItemStack")
	static URockItemDefinition* GetItemDefinition(const FName& ItemId);
	static FVector2D GetItemSize(const FRockItemStack& ItemStack);
	static FRockItemStack CreateItemStack(URockInventory* OwningInventory, const FRockItemStack& InItemStack);
	
	/** Returns true if this stack can be combined with another stack */
	static bool CanStackWith(const FRockItemStack& FirstItem, const FRockItemStack& SecondItem);

	static int32 GetMaxStackSize(const FRockItemStack& ItemStack);

	/** Returns true if the stack is at its maximum size */
	static bool IsFull(const FRockItemStack& ItemStack);
	

};
