// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Transactions/Implementations/RockMoveItemTransaction.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Library/RockInventoryLibrary.h"

bool FRockMoveItemUndoTransaction::CanUndo() const
{
	// If move failed, we definitely can't undo it
	if (!bSuccess)
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
	const FRockItemStack& CurrentSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
	const FRockItemStack& CurrentTargetItem = TargetInventory->GetItemBySlotHandle(TargetSlotHandle);

	// If the items have been modified since our operation, we can't safely undo
	if (CurrentSourceItem != PostMoveSourceItem || CurrentTargetItem != PostMoveTargetItem)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::CanUndo - Item states have changed since the move was performed"));
		return false;
	}

	return true;
}

bool FRockMoveItemUndoTransaction::Undo() const
{
	// Skip undo if execute didn't succeed

	// CanUndo should have been called first. Don't need to check again.
	checkf(!bSuccess, TEXT("MoveItemTransaction::Undo - Original move failed, nothing to undo"));
	checkf(SourceInventory && TargetInventory, TEXT("MoveItemTransaction::Undo - Source or Target inventory is null"));

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
		{
			OriginalOrientation,
			ERockItemMoveMode::CustomAmount,
			MoveCount,
		});

	if (!undoSucceeded)
	{
		UE_LOG(LogRockInventory, Error, TEXT("MoveItemTransaction::Undo - Failed to undo move operation"));
	}

	return undoSucceeded;
}

FRockMoveItemTransaction::FRockMoveItemTransaction()
{
	GenerateNewHandle();
}

FRockMoveItemTransaction::FRockMoveItemTransaction(AController* Instigator, 
	URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle, URockInventory* InTargetInventory,
	const FRockInventorySlotHandle& InTargetSlotHandle, const FRockMoveItemParams& InMoveParam)
	: Super(Instigator), SourceInventory(InSourceInventory), SourceSlotHandle(InSourceSlotHandle),
	  TargetInventory(InTargetInventory), TargetSlotHandle(InTargetSlotHandle),
	  MoveParams(InMoveParam)
{
	GenerateNewHandle();
}


bool FRockMoveItemTransaction::CanExecute() const
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
	const FRockPendingSlotOperation SourcePendingSlot = SourceInventory->GetPendingSlotState(SourceSlotHandle);
	if (SourcePendingSlot.IsClaimedByOther(Instigator.Get()))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::CanApply - Source slot is locked by other %p %p"), Instigator.Get(), SourcePendingSlot.Controller.Get());
		return false;
	}
	const FRockPendingSlotOperation TargetPendingSlot = TargetInventory->GetPendingSlotState(TargetSlotHandle);
	if (TargetPendingSlot.IsClaimedByOther(Instigator.Get()))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("MoveItemTransaction::CanApply - Target slot is locked by other"));
		return false;
	}

	return true;
}

FRockMoveItemUndoTransaction FRockMoveItemTransaction::Execute() const
{
	FRockMoveItemUndoTransaction UndoTransaction;
	UndoTransaction.bSuccess = false;

	// CanExecute should have been called first. Don't need to check again.
	checkf(SourceInventory && TargetInventory, TEXT("MoveItemTransaction::Execute - Source or Target inventory is null"));


	const FRockInventorySlotEntry& OriginalSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
	UndoTransaction.OriginalOrientation = OriginalSlot.Orientation;

	// Store the original states before the move
	UndoTransaction.OriginalSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
	UndoTransaction.OriginalTargetItem = TargetInventory->GetItemBySlotHandle(TargetSlotHandle);

	// Execute the move operation
	UndoTransaction.bSuccess = URockInventoryLibrary::MoveItem(SourceInventory, SourceSlotHandle, TargetInventory, TargetSlotHandle);

	// Store the post-move states for future validation
	UndoTransaction.PostMoveSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
	UndoTransaction.PostMoveTargetItem = TargetInventory->GetItemBySlotHandle(TargetSlotHandle);

	// Calculate the actual amount moved for proper undoing
	if (UndoTransaction.bSuccess)
	{
		// For a new stack, this is the final target stack size
		// For a merged stack, this is the difference from original
		if (UndoTransaction.OriginalTargetItem.IsValid())
		{
			UndoTransaction.MoveCount = UndoTransaction.PostMoveTargetItem.GetStackSize() - UndoTransaction.OriginalTargetItem.GetStackSize();
		}
		else
		{
			UndoTransaction.MoveCount = UndoTransaction.PostMoveTargetItem.GetStackSize();
		}
	}

	return UndoTransaction;
}

bool FRockMoveItemTransaction::AttemptPredict() const
{
	return true;
}
