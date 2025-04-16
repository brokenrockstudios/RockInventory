// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/Transactions/RockInventoryTransactionManager.h"

#include "Inventory/Transactions/RockInventoryTransaction.h"

URockInventoryTransactionManager::URockInventoryTransactionManager() : Super(),
                                                                       CurrentTransactionIndex(-1),
                                                                       MaxHistoryLength(0),
                                                                       OwnerInventory(nullptr)
{
}

void URockInventoryTransactionManager::Initialize(URockInventoryComponent* InOwnerInventory, int32 InMaxHistoryLength)
{
	OwnerInventory = InOwnerInventory;
	MaxHistoryLength = InMaxHistoryLength;
	ClearHistory();
}

bool URockInventoryTransactionManager::ExecuteTransaction(TSharedPtr<FRockInventoryTransaction> Transaction)
{
	if (!Transaction || !OwnerInventory)
	{
		return false;
	}

	if (!Transaction->CanApply(OwnerInventory))
	{
		return false;
	}

	// Remove any redoable transactions
	if (TransactionHistory.Num() > 0 && CurrentTransactionIndex < TransactionHistory.Num() - 1)
	{
		TransactionHistory.RemoveAt(CurrentTransactionIndex + 1, TransactionHistory.Num() - CurrentTransactionIndex - 1);
	}

	// Execute the transaction
	if (Transaction->Execute())
	{
		// Add to history
		TransactionHistory.Add(Transaction);
		CurrentTransactionIndex = TransactionHistory.Num() - 1;

		// Trim history if needed
		if (TransactionHistory.Num() > MaxHistoryLength)
		{
			TransactionHistory.RemoveAt(0);
			CurrentTransactionIndex--;
		}

		return true;
	}

	return false;
}

bool URockInventoryTransactionManager::UndoLastTransaction()
{
	if (!CanUndo())
	{
		return false;
	}

	const TSharedPtr<FRockInventoryTransaction> Transaction = TransactionHistory[CurrentTransactionIndex];
	if (Transaction->Undo())
	{
		CurrentTransactionIndex--;
		return true;
	}

	return false;
}

bool URockInventoryTransactionManager::RedoTransaction()
{
	if (!CanRedo())
	{
		return false;
	}

	const TSharedPtr<FRockInventoryTransaction> Transaction = TransactionHistory[CurrentTransactionIndex + 1];
	if (Transaction->Redo())
	{
		CurrentTransactionIndex++;
		return true;
	}

	return false;
}

void URockInventoryTransactionManager::ClearHistory()
{
	TransactionHistory.Empty();
	CurrentTransactionIndex = -1;
}

TArray<FString> URockInventoryTransactionManager::GetTransactionDescriptions() const
{
	TArray<FString> Descriptions;
	for (const auto& Transaction : TransactionHistory)
	{
		Descriptions.Add(Transaction->GetDescription());
	}
	return Descriptions;
}

bool URockInventoryTransactionManager::CanUndo() const
{
	return TransactionHistory.Num() > 0 && CurrentTransactionIndex >= 0 &&
		TransactionHistory[CurrentTransactionIndex]->CanUndo();
}

bool URockInventoryTransactionManager::CanRedo() const
{
	return TransactionHistory.Num() > 0 && CurrentTransactionIndex < TransactionHistory.Num() - 1;
}
