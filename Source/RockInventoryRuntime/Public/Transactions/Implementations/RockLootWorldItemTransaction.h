// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockInventoryQuery.h"
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
	/** What the loot placed and the excess, as returned by URockInventoryLibrary::LootItemToInventory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockLootResult Result;
	bool bSuccess;

	bool CanUndo();
	bool Undo();
};

USTRUCT(BlueprintType)
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

	// What the looting may do. Sent by the client like the rest of the command, so it can only narrow what the sections allow.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockLootParams LootParams;

	bool CanExecute() const;
	FRockLootWorldItemUndoTransaction Execute();
	bool AttemptPredict() const;
};
