// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Transactions/Implementations/RockMoveItemTransaction.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Library/RockInventoryLibrary.h"

FRockMoveItemTransaction::FRockMoveItemTransaction()
{
}

FRockMoveItemTransaction::FRockMoveItemTransaction(AController* Instigator, 
	URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle, URockInventory* InTargetInventory,
	const FRockInventorySlotHandle& InTargetSlotHandle, const FRockMoveItemParams& InMoveParam)
	: Super(Instigator), SourceInventory(InSourceInventory), SourceSlotHandle(InSourceSlotHandle),
	  TargetInventory(InTargetInventory), TargetSlotHandle(InTargetSlotHandle),
	  MoveParams(InMoveParam)
{
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

	// Not an error: someone else changed the slot, or the client's picture was stale. The refusal is the point of the check, and it
	// comes first so a raced command (an undo whose item another player took) does not warn about an empty source.
	if (!ExpectedSource.Matches(*SourceInventory, SourceSlotHandle) || !ExpectedTarget.Matches(*TargetInventory, TargetSlotHandle))
	{
		UE_LOG(LogRockInventory, Log, TEXT("MoveItemTransaction::CanApply - a slot does not hold what the command expected"));
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
	
	if (!Instigator.IsValid() || !IsValid(SourceInventory) || !IsValid(TargetInventory))
	{
		UE_LOG(LogRockInventory, Error, TEXT("Invalid data in FRockLootWorldItemTransaction. Instigator valid: %s, SourceInventory valid: %s, TargetInventory valid: %s"), 
			Instigator.IsValid() ? TEXT("true") : TEXT("false"), 
			IsValid(SourceInventory) ? TEXT("true") : TEXT("false"), 
			IsValid(TargetInventory) ? TEXT("true") : TEXT("false"));
		return UndoTransaction;
	}
		
	

	const FRockInventorySlotEntry& OriginalSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
	UndoTransaction.OriginalOrientation = OriginalSlot.Orientation;

	// Store the original states before the move
	UndoTransaction.OriginalSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
	UndoTransaction.OriginalTargetItem = TargetInventory->GetItemBySlotHandle(TargetSlotHandle);

	// Execute the move operation
	UndoTransaction.bSuccess = URockInventoryLibrary::MoveItem(SourceInventory, SourceSlotHandle, TargetInventory, TargetSlotHandle, MoveParams);

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
			UndoTransaction.MoveCount = UndoTransaction.PostMoveTargetItem.GetStackCount() - UndoTransaction.OriginalTargetItem.GetStackCount();
		}
		else
		{
			UndoTransaction.MoveCount = UndoTransaction.PostMoveTargetItem.GetStackCount();
		}
	}

	return UndoTransaction;
}
