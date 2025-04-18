// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Inventory/Transactions/RockInventoryTransactionManager.h"

#include "Inventory/Transactions/RockDropItemTransaction.h"
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

bool URockInventoryTransactionManager::BeginTransaction(URockInventoryTransaction* Transaction)
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

	URockInventoryTransaction* Transaction = TransactionHistory[CurrentTransactionIndex].Get();
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

	URockInventoryTransaction* Transaction = TransactionHistory[CurrentTransactionIndex + 1];
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

bool URockInventoryTransactionManager::K2_BeginDrop(URockDropItemTransaction* Transaction)
{
	return BeginTransaction(Transaction);
}

// bool URockInventoryTransactionManager::K2_BeginTransaction(const int32& Transaction)
// {
// 	// This will never be called, the exec version below will be hit instead
// 	checkNoEntry();
// 	return false;
// }
//
// DEFINE_FUNCTION(URockInventoryTransactionManager::execK2_BeginTransaction)
// {
// 	// Stack.MostRecentPropertyAddress = nullptr;
// 	// Get the transaction data struct from the parameter
// 	Stack.StepCompiledIn<FStructProperty>(nullptr);
// 	void* StructPtr = Stack.MostRecentPropertyAddress;
// 	FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
//
// 	// Make sure it's derived from FRockInventoryTransaction
// 	if (!StructProperty || !StructProperty->Struct->IsChildOf(FRockInventoryTransaction::StaticStruct()))
// 	{
// 		const FBlueprintExceptionInfo ExceptionInfo(
// 			EBlueprintExceptionType::AccessViolation,
// 			FText::Format(NSLOCTEXT("RockInventory", "WrongStructType",
// 					"BeginTransaction requires a struct derived from FRockInventoryTransaction, not {0}"),
// 				FText::FromString(StructProperty ? StructProperty->Struct->GetName() : TEXT("None")))
// 		);
// 		FBlueprintCoreDelegates::ThrowScriptException(Context, Stack, ExceptionInfo);
//
// 		P_FINISH;
// 		P_NATIVE_BEGIN;
// 			*static_cast<bool*>(RESULT_PARAM) = false;
// 		P_NATIVE_END;
// 		return;
// 	}
//
// 	// Validate the transaction data
// 	FRockInventoryTransaction* TransactionData = static_cast<FRockInventoryTransaction*>(StructPtr);
// 	if (!TransactionData)
// 	{
// 		const FBlueprintExceptionInfo ExceptionInfo(
// 			EBlueprintExceptionType::AccessViolation,
// 			NSLOCTEXT("RockInventory", "InvalidTransactionData", "Invalid transaction data provided")
// 		);
// 		FBlueprintCoreDelegates::ThrowScriptException(Context, Stack, ExceptionInfo);
//
// 		P_FINISH;
// 		P_NATIVE_BEGIN;
// 			*static_cast<bool*>(RESULT_PARAM) = false;
// 		P_NATIVE_END;
// 		return;
// 	}
//
// 	// Create a shared pointer to the transaction using the existing data
// 	const TSharedPtr<FRockInventoryTransaction> Transaction = MakeShared<FRockInventoryTransaction>(*TransactionData);
//
// 	P_FINISH;
// 	P_NATIVE_BEGIN;
// 		*static_cast<bool*>(RESULT_PARAM) = P_THIS->BeginTransaction(Transaction);
// 	P_NATIVE_END;
// }
