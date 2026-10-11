// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "Enums/RockEnums.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Library/RockInventoryLibrary.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "Transactions/Core/RockSlotExpectation.h"
#include "RockMoveItemTransaction.generated.h"

/** What a move did on the server (FRockMoveItemTransaction::Execute). The server keeps no history: undo is the client's (T-80, URockInventoryManagerComponent::Undo). */
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
};


// This data is what get's serialized and sent to the server
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockMoveItemTransaction : public FRockItemTransactionBase
{
	GENERATED_BODY()
	FRockMoveItemTransaction();

	FRockMoveItemTransaction(AController* Instigator,
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

	/**
	 * What the client saw in the two slots when it made the move (T-78). The manager component fills them when it sends the command;
	 * the server refuses the move if a slot holds something else now. Unset (bCheck false) means no check.
	 */
	UPROPERTY()
	FRockSlotExpectation ExpectedSource;
	UPROPERTY()
	FRockSlotExpectation ExpectedTarget;

	FRockMoveItemUndoTransaction Execute() const;
	/** Valid inventories and slots, a source item, no slot claimed by someone else, and the preconditions still hold. */
	bool CanExecute() const;
};
