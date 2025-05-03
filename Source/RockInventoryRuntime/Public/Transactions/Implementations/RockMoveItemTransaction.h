// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "Enums/RockEnums.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Library/RockInventoryLibrary.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "RockMoveItemTransaction.generated.h"

USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockMoveItemUndoTransaction : public FRockItemTransactionBase
{
	GENERATED_BODY()

	FRockMoveItemUndoTransaction() = default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle SourceSlotHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;

	///////////////////////////////////////////////////////////////////////////
	/** Original state of the source item before the move */
	UPROPERTY()
	FRockItemStack OriginalSourceItem;

	/** Original state of the target item before the move */
	UPROPERTY()
	FRockItemStack OriginalTargetItem;

	/** State of the source item after the move */
	UPROPERTY()
	FRockItemStack PostMoveSourceItem;

	/** State of the target item after the move */
	UPROPERTY()
	FRockItemStack PostMoveTargetItem;

	// Undo is always custom move type.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MoveCount = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERockItemOrientation OriginalOrientation = ERockItemOrientation::Horizontal;

	bool CanUndo() const;
	bool Undo() const;
};


// This data is what get's serialized and sent to the server
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockMoveItemTransaction : public FRockItemTransactionBase
{
	GENERATED_BODY()
	FRockMoveItemTransaction();

	FRockMoveItemTransaction(
		URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle,
		URockInventory* InTargetInventory, const FRockInventorySlotHandle& InTargetSlotHandle,
		const FRockMoveItemParams& InMoveParam = FRockMoveItemParams());

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle SourceSlotHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockMoveItemParams MoveParams;


	FRockMoveItemUndoTransaction Execute() const;
	bool CanExecute() const;
	bool AttemptPredict() const;
};
