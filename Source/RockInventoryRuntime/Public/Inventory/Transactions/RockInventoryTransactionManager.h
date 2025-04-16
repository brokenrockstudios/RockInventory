// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RockInventoryComponent.h"
#include "UObject/Object.h"
#include "RockInventoryTransactionManager.generated.h"

struct FRockInventoryTransaction;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryTransactionManager : public UObject
{
	GENERATED_BODY()
private:
	// Transaction history
	TArray<TSharedPtr<FRockInventoryTransaction>> TransactionHistory;
	// Current position in the transaction history
	int32 CurrentTransactionIndex = -1;
	// Maximum history length
	int32 MaxHistoryLength;
	// Owner inventory
	UPROPERTY()
	URockInventoryComponent* OwnerInventory;
public:
	URockInventoryTransactionManager();
	void Initialize(URockInventoryComponent* InOwnerInventory, int32 InMaxHistoryLength = 25);

	// Execute and record a transaction
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	bool ExecuteTransaction(TSharedPtr<FRockInventoryTransaction> Transaction);
    
	// Undo the last transaction
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	bool UndoLastTransaction();
    
	// Redo the last undone transaction
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	bool RedoTransaction();
    
	// Clear transaction history
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	void ClearHistory();
    
	// Get transaction history as an array
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	TArray<FString> GetTransactionDescriptions() const;
    
	// Check if can undo
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	bool CanUndo() const;
    
	// Check if can redo
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	bool CanRedo() const;
	
};
