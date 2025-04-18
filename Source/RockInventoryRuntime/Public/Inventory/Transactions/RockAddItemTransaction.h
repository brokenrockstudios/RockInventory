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
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockAddItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStack ItemStack;

	virtual bool Execute_Implementation() override
	{
		return false; // URockInventoryLibrary::AddItemToInventory(TargetInventory, ItemStack, TargetSlotHandle);
	}

	virtual bool Undo_Implementation()
	{
		return false; // URockInventoryLibrary::RemoveItem(TargetInventory, ItemStack, TargetSlotHandle);
	}

	virtual FString GetDescription_Implementation() const
	{
		return FString::Printf(TEXT("Add Item"));
	}
};
