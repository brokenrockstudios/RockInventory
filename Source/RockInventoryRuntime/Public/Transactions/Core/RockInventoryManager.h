// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RockInventoryComponent.h"
#include "UObject/Object.h"
#include "RockInventoryManager.generated.h"

class URockInventoryTransaction;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class ROCKINVENTORYRUNTIME_API URockInventoryManager : public UObject
{
	GENERATED_BODY()

private:
	// Transaction history
	UPROPERTY()
	TArray<TObjectPtr<URockInventoryTransaction>> TransactionHistory;
	// Current position in the transaction history
	int32 CurrentTransactionIndex = -1;
	// Maximum history length
	int32 MaxHistoryLength;
	// Owner inventory
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventoryComponent> OwnerInventory;
	URockInventoryManager();
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	void Initialize(URockInventoryComponent* InOwnerInventory, int32 InMaxHistoryLength = 25);


	/**
	 * Enqueue and execute a transaction
	 * This likely will fully execute in the same frame as call. Though this be subject to change later.
	 *
	 * @param Transaction The transaction to execute (must be derived from URockInventoryTransaction)
	 */
	// Execute and record a transaction
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	bool EnqueueTransaction(URockInventoryTransaction* Transaction);

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
private:
};
