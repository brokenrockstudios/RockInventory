// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "RockLootWorldItemTransaction.generated.h"

class URockInventory;

USTRUCT(BlueprintType)
struct FRockLootWorldItemUndoTransaction : public FRockItemTransactionBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStack ItemStack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Excess = -1;
	bool bSuccess;

	bool CanUndo();
	bool Undo();
};

USTRUCT(Blueprintable, BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockLootWorldItemTransaction : public FRockItemTransactionBase
{
	GENERATED_BODY()

	// This could be a world item attempting to give some items to the player.
	// Generally this will just be a 'lootable' item on the ground.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> SourceWorldItemActor = nullptr;

	// No specific location
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;

	bool CanExecute() const;
	FRockLootWorldItemUndoTransaction Execute();
	bool AttemptPredict() const;
};
