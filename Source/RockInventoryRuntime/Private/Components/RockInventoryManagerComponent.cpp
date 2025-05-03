// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Components/RockInventoryManagerComponent.h"

#include "RockInventoryLogging.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "Transactions/Implementations/RockMoveItemTransaction.h"


URockInventoryManagerComponent::URockInventoryManagerComponent(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer),
	CurrentTransactionIndex(-1),
	MaxHistoryLength(25)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	ClearHistory();
}

void URockInventoryManagerComponent::Client_TransactionResult_Implementation(int32 ClientTransactionID, bool bSuccess)
{
	// If we ever receive a bSuccess == false
	// we need to clear history and refresh everything!
	// Since our prediction is wrong and the state could be out of sync
	bHasPendingPredictiveMove = false;

	if (bSuccess)
	{
		// At this time, we shouldn't ever have more than 1 predictive move. As that complicates unwinding a lot of things.
		// PendingServerTransactions.Remove(ClientTransactionID);
	}
	else
	{
		// UH OH. Clear history, prevent any further transactions until we've resynced
		bAwaitingServerSync = true;
		// ensureMsgf(false, TEXT("ClientTransactionResult_Implementation - Not yet implemented"));
		// Request a redownload of the inventory of the relevant inventory and slots.
	}
}

void URockInventoryManagerComponent::LootWorldItem(const FRockLootWorldItemTransaction& ItemTransaction)
{
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	FRockLootWorldItemUndoTransaction Undo;
	Undo.bSuccess = true;

	if (bEnablePredictiveExecution && ItemTransaction.AttemptPredict())
	{
		Undo = ItemTransaction.Execute();
	}

	// If we predicted locally, and it succeeded, we need to send the transaction to the server and add to history
	if (Undo.bSuccess)
	{
		if (GetOwnerRole() != ROLE_Authority)
		{
			// TODO: If undo/redo isn't predicted, do we even need to add it to local client history?

			// Remove any redoable transactions
			// We are about to add to the history, so we need to remove any redoable transactions ahead of 'this one'
			if (TransactionHistoryData.Num() > 0 && CurrentTransactionIndex < TransactionHistoryData.Num() - 1)
			{
				TransactionHistoryData.RemoveAt(CurrentTransactionIndex + 1, TransactionHistoryData.Num() - CurrentTransactionIndex - 1);
			}

			FRockInventoryTransactionRecord TransactionRecord;
			TransactionRecord.Set<FRockLootWorldItemTransaction, FRockLootWorldItemUndoTransaction>(ItemTransaction, Undo);
			TransactionHistoryData.Add(TransactionRecord);

			CurrentTransactionIndex = TransactionHistoryData.Num() - 1;
			// Trim history if needed
			if (TransactionHistoryData.Num() > MaxHistoryLength)
			{
				TransactionHistoryData.RemoveAt(0);
				CurrentTransactionIndex--;
			}
		}
		else
		{
			// We are the authority, so we don't need to add the transaction to history.
			// The server will add it to history when it executes the transaction.
		}

		// If we predicted locally, and it succeeded, we need to send the transaction to the server
		Server_LootWorldItem(ItemTransaction);
	}
}


void URockInventoryManagerComponent::Server_LootWorldItem_Implementation(const FRockLootWorldItemTransaction& AddItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_AddItem - Not authority!"));
		return;
	}
	// If we can't execute, don't execute and don't add to history
	if (!AddItemTransaction.CanExecute())
	{
		return;
	}
	const FRockLootWorldItemUndoTransaction& Undo = AddItemTransaction.Execute();

	FRockInventoryTransactionRecord TransactionRecord;
	TransactionRecord.Set<FRockLootWorldItemTransaction, FRockLootWorldItemUndoTransaction>(AddItemTransaction, Undo);
	TransactionHistoryData.Add(TransactionRecord);

	Client_TransactionResult(AddItemTransaction.TransactionID, Undo.bSuccess);
}


bool URockInventoryManagerComponent::MoveItem(const FRockMoveItemTransaction& ItemTransaction)
{
	if (!ItemTransaction.CanExecute())
	{
		return false;
	}
	FRockMoveItemUndoTransaction Undo;

	Undo.bSuccess = true;

	if (bEnablePredictiveExecution && ItemTransaction.AttemptPredict())
	{
		Undo = ItemTransaction.Execute();
	}

	if (Undo.bSuccess)
	{
		if (GetOwnerRole() != ROLE_Authority)
		{
			// TODO: If undo/redo isn't predicted, do we even need to add it to local client history?

			// Remove any redoable transactions
			// We are about to add to the history, so we need to remove any redoable transactions ahead of 'this one'
			if (TransactionHistoryData.Num() > 0 && CurrentTransactionIndex < TransactionHistoryData.Num() - 1)
			{
				TransactionHistoryData.RemoveAt(CurrentTransactionIndex + 1, TransactionHistoryData.Num() - CurrentTransactionIndex - 1);
			}

			FRockInventoryTransactionRecord TransactionRecord;
			TransactionRecord.Set<FRockMoveItemTransaction, FRockMoveItemUndoTransaction>(ItemTransaction, Undo);
			TransactionHistoryData.Add(TransactionRecord);

			CurrentTransactionIndex = TransactionHistoryData.Num() - 1;
			// Trim history if needed
			if (TransactionHistoryData.Num() > MaxHistoryLength)
			{
				TransactionHistoryData.RemoveAt(0);
				CurrentTransactionIndex--;
			}
		}
		else
		{
			// We are the authority, so we don't need to add the transaction to history.
			// The server will add it to history when it executes the transaction.
		}
		// If we predicted locally, and it succeeded, we need to send the transaction to the server
		Server_MoveItem(ItemTransaction);
	}

	return Undo.bSuccess;
}

void URockInventoryManagerComponent::Server_MoveItem_Implementation(const FRockMoveItemTransaction& ItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_DropItem - Not authority!"));
		return;
	}
	// If we can't execute, don't execute and don't add to history
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	const FRockMoveItemUndoTransaction& Undo = ItemTransaction.Execute();
	FRockInventoryTransactionRecord TransactionRecord;

	TransactionRecord.Set<FRockMoveItemTransaction, FRockMoveItemUndoTransaction>(ItemTransaction, Undo);
	TransactionHistoryData.Add(TransactionRecord);

	Client_TransactionResult(ItemTransaction.TransactionID, Undo.bSuccess);
}


void URockInventoryManagerComponent::DropItem(const FRockDropItemTransaction& ItemTransaction)
{
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	FRockDropItemUndoTransaction Undo;
	Undo.bSuccess = true;

	if (bEnablePredictiveExecution && ItemTransaction.AttemptPredict())
	{
		Undo = ItemTransaction.Execute();
	}
	// Drop item can't add an undo without prediction because it relies on knowing the dropped item
	// Which we don't know until we spawn it on the server and replicate it back to the client:(

	// The client might predict the drop, but it lacks the information to be able to predictively undo it.
	// All undos are non-predictive.
	if (Undo.bSuccess)
	{
		if (GetOwnerRole() != ROLE_Authority)
		{
			// TODO: If undo/redo isn't predicted, do we even need to add it to local client history?

			// Remove any redoable transactions
			// We are about to add to the history, so we need to remove any redoable transactions ahead of 'this one'
			if (TransactionHistoryData.Num() > 0 && CurrentTransactionIndex < TransactionHistoryData.Num() - 1)
			{
				TransactionHistoryData.RemoveAt(CurrentTransactionIndex + 1, TransactionHistoryData.Num() - CurrentTransactionIndex - 1);
			}

			FRockInventoryTransactionRecord TransactionRecord;
			TransactionRecord.Set<FRockDropItemTransaction, FRockDropItemUndoTransaction>(ItemTransaction, Undo);
			TransactionHistoryData.Add(TransactionRecord);

			CurrentTransactionIndex = TransactionHistoryData.Num() - 1;
			// Trim history if needed
			if (TransactionHistoryData.Num() > MaxHistoryLength)
			{
				TransactionHistoryData.RemoveAt(0);
				CurrentTransactionIndex--;
			}
		}
		else
		{
			// We are the authority, so we don't need to add the transaction to history.
			// The server will add it to history when it executes the transaction.
		}

		
		// If we predicted locally, and it succeeded, we need to send the transaction to the server
		Server_DropItem(ItemTransaction);
	}
}

void URockInventoryManagerComponent::Server_DropItem_Implementation(const FRockDropItemTransaction& ItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_DropItem - Not authority!"));
		return;
	}
	// If we can't execute, don't execute and don't add to history
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	const FRockDropItemUndoTransaction& Undo = ItemTransaction.Execute();
	FRockInventoryTransactionRecord TransactionRecord;

	TransactionRecord.Set<FRockDropItemTransaction, FRockDropItemUndoTransaction>(ItemTransaction, Undo);
	TransactionHistoryData.Add(TransactionRecord);

	Client_TransactionResult(ItemTransaction.TransactionID, Undo.bSuccess);
}

void URockInventoryManagerComponent::ClearHistory()
{
	TransactionHistoryData.Empty();
	CurrentTransactionIndex = -1;
}

bool FRockInventoryTransactionRecord::ExecuteUndo()
{
	if (Undo.GetScriptStruct() == FRockMoveItemUndoTransaction::StaticStruct())
	{
		FRockMoveItemUndoTransaction UndoData = Undo.Get<FRockMoveItemUndoTransaction>();
		return UndoData.Undo();
	}
	return false;
	// can't undo anything other than Move at this time. So don't even try.
}

// bool URockInventoryManagerComponent::UndoLastTransaction()
// {
// 	if (!CanUndo())
// 	{
// 		return false;
// 	}
//
// 	//URockInventoryTransaction* Transaction = TransactionHistory[CurrentTransactionIndex].Get();
// 	// if (Transaction->Undo())
// 	// {
// 	// 	CurrentTransactionIndex--;
// 	// 	return true;
// 	// }
//
// 	FRockInventoryTransactionRecord Transaction = TransactionHistoryData[CurrentTransactionIndex];
// 	Transaction.ExecuteUndo();
//
// 	return false;
// }

// bool URockInventoryManagerComponent::RedoTransaction()
// {
// 	if (!CanRedo())
// 	{
// 		return false;
// 	}
// 	return false;
// }

// bool URockInventoryManagerComponent::CanUndo() const
// {
// 	if (TransactionHistoryData.Num() > 0 && CurrentTransactionIndex >= 0)
// 	{
// 		return false;
// 		
// 	}
// 	return false;
// }
//
// bool URockInventoryManagerComponent::CanRedo() const
// {
// 	return TransactionHistoryData.Num() > 0 && CurrentTransactionIndex < TransactionHistoryData.Num() - 1;
// }
//
