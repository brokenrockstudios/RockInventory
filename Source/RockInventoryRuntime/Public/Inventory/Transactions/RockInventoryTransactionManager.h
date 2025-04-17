// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockDropItemTransaction.h"
#include "Components/RockInventoryComponent.h"
#include "UObject/Object.h"
#include "RockInventoryTransactionManager.generated.h"

struct FRockInventoryTransaction;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
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
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventoryComponent> OwnerInventory;
	URockInventoryTransactionManager();
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	void Initialize(URockInventoryComponent* InOwnerInventory, int32 InMaxHistoryLength = 25);

	// Execute and record a transaction
	bool BeginTransaction(const TSharedPtr<FRockInventoryTransaction>& Transaction);
    
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


	// TArray<ActiveTransaction>, support only sequential for now.
	// UpdateTransaction
	// Tick | Parent actor or component would need to call this. Otherwise we could potentially leverage the WorldTimer to enable/disable ticks?
	




	UFUNCTION(BlueprintCallable,Category="Inventory|Transactions")
	bool K2_BeginDrop(const FRockDropItemTransaction & Transaction);
	

	
	/**
	 * Execute a transaction 
	 *
	 * @param Transaction			The transaction to execute (must be derived from FRockInventoryTransaction)
	 */
	// UFUNCTION(BlueprintCallable, CustomThunk, Category="Inventory|Transactions", meta=(CustomStructureParam="Transaction", AllowAbstract="false", DisplayName="Begin Transaction"))
	// bool K2_BeginTransaction(const int32& Transaction);
	// DECLARE_FUNCTION(execK2_BeginTransaction);

private:

};
