// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventoryTransaction.h"
#include "Inventory/RockSlotHandle.h"
#include "UObject/Object.h"
#include "RockAddItemTransaction.generated.h"

class URockInventory;
/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockAddItemTransaction : public FRockInventoryTransaction
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStack ItemStack;

	virtual bool Execute() override
	{
		return false; // URockInventoryLibrary::AddItemToInventory(TargetInventory, ItemStack, TargetSlotHandle);
	}

	virtual bool Undo() override
	{
		return false; // URockInventoryLibrary::RemoveItem(TargetInventory, ItemStack, TargetSlotHandle);
	}

	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Add Item"));
	}
};
