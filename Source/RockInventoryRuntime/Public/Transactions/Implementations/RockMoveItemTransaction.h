// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "RockInventoryLogging.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "RockMoveItemTransaction.generated.h"

/**
 * Transaction for moving items between inventory slots
 * Supports full undo/redo with proper state tracking
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockMoveItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transaction")
	static URockMoveItemTransaction* CreateMoveItemTransaction(
		URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle,
		URockInventory* InTargetInventory, const FRockInventorySlotHandle& InTargetSlotHandle);

	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle SourceSlotHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;

	// User Configurable
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockMoveItemParams MoveParams;

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
	
protected:
	bool moveItemSuccess;

	virtual bool Execute_Implementation() override
	{
		// Validate both inventories
		if (!SourceInventory || !TargetInventory)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::Execute - Invalid Source or Target Inventory"));
			return false;
		}

		// Store the original states before the move
		OriginalSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
		OriginalTargetItem = TargetInventory->GetItemBySlotHandle(TargetSlotHandle);
		
		// Execute the move operation
		moveItemSuccess = URockInventoryLibrary::MoveItem(SourceInventory, SourceSlotHandle, TargetInventory, TargetSlotHandle, MoveParams);

		// Store the post-move states for future validation
		PostMoveSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
		PostMoveTargetItem = TargetInventory->GetItemBySlotHandle(TargetSlotHandle);
		
		// Calculate the actual amount moved for proper undoing
		if (moveItemSuccess)
		{
			// For a new stack, this is the final target stack size
			// For a merged stack, this is the difference from original
			MoveParams.MoveAmount = ERockItemMoveAmount::Custom;
			if (OriginalTargetItem.IsValid())
			{
				MoveParams.MoveCount = PostMoveTargetItem.GetStackSize() - OriginalTargetItem.GetStackSize();
			}
			else
			{
				MoveParams.MoveCount = PostMoveTargetItem.GetStackSize();
			}
		}
		
		UE_LOG(LogRockInventory, Log, TEXT("MoveItem: Original Source=%d, Original Target=%d -> PostMove Source=%d, PostMove Target=%d, Actual Moved=%d"), 
			OriginalSourceItem.GetStackSize(), 
			OriginalTargetItem.GetStackSize(), 
			PostMoveSourceItem.GetStackSize(), 
			PostMoveTargetItem.GetStackSize(),
			MoveParams.MoveCount);
		
		return moveItemSuccess;
	}

	virtual bool Undo_Implementation() override
	{
		// Validate both inventories are still available
		if (!SourceInventory || !TargetInventory)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::Undo - Invalid Source or Target Inventory"));
			return false;
		}
		
		// Skip undo if execute didn't succeed
		if (!moveItemSuccess)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::Undo - Original move failed, nothing to undo"));
			return false;
		}
		
		// Only try to undo if we can verify the state is still valid
		if (!CanUndo())
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::Undo - CanUndo returned false, state may have changed"));
			return false;
		}

		// Perform the reverse move operation
		bool undoSucceeded = URockInventoryLibrary::MoveItem(
			TargetInventory, TargetSlotHandle, 
			SourceInventory, SourceSlotHandle, 
			MoveParams);
			
		if (!undoSucceeded)
		{
			UE_LOG(LogRockInventory, Error, TEXT("MoveItemTransaction::Undo - Failed to undo move operation"));
		}
		
		return undoSucceeded;
	}

	// Can undo
	virtual bool CanUndo() const override
	{
		// If move failed, we definitely can't undo it
		if (!moveItemSuccess)
		{
			return false;
		}
		
		// Verify inventories are still valid
		if (!SourceInventory || !TargetInventory)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::CanUndo - Source or Target inventory is no longer valid"));
			return false;
		}
		
		// Check that current states match what we expect after the move
		const FRockItemStack CurrentSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
		const FRockItemStack CurrentTargetItem = TargetInventory->GetItemBySlotHandle(TargetSlotHandle);
		
		// If the items have been modified since our operation, we can't safely undo
		if (CurrentSourceItem != PostMoveSourceItem || CurrentTargetItem != PostMoveTargetItem)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::CanUndo - Item states have changed since the move was performed"));
			return false;
		}
		
		return true;
	}

	// Can apply/execute
	virtual bool CanApply(URockInventoryComponent* OwnerInventory) const override
	{
		// Validate inventories exist
		if (!SourceInventory || !TargetInventory)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::CanApply - Invalid Source or Target Inventory"));
			return false;
		}
		
		// Validate slot handles
		if (!SourceSlotHandle.IsValid() || !TargetSlotHandle.IsValid())
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::CanApply - Invalid Source or Target Slot Handle"));
			return false;
		}
		
		// Check source has an item
		const FRockItemStack SourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
		if (!SourceItem.IsValid())
		{
			UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::CanApply - Source slot has no valid item"));
			return false;
		}
		
		return true;
	}

	virtual FString GetDescription() const override
	{
		if (SourceInventory && TargetInventory)
		{
			return FString::Printf(TEXT("Move Item from %s to %s"), 
				*SourceInventory->GetName(), 
				*TargetInventory->GetName());
		}
		return FString::Printf(TEXT("Move Item"));
	}
};

