// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Transactions/Core/RockInventoryTransaction.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventoryManagerLibrary.generated.h"

class URockInventoryManager;
class URockInventoryManagerComponent;

/**
 * Blueprint function library for managing inventory transactions.
 * Provides utility functions for enqueueing and managing inventory transactions.
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryManagerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Enqueue a transaction to be processed by the inventory manager.
	 * @param Object The object that is the inventory manager, or can get the inventory manager.
	 *					If null, the function will still execute but will not record the transaction.
	 * @param Transaction The transaction to be processed.
	 * @return true if the transaction was successfully enqueued, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Enqueue Transaction"))
	static bool EnqueueTransaction(UObject* Object, URockInventoryTransaction* Transaction);

	/**
	 * Get the inventory manager from an object.
	 * @param Object The object that is the inventory manager, or can get the inventory manager.
	 * @return The inventory manager if found, nullptr otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Get Inventory Manager"))
	static URockInventoryManager* GetInventoryManager(UObject* Object);

	/**
	 * Check if an object has an inventory manager.
	 * @param Object The object to check
	 * @return true if the object has an inventory manager, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Has Inventory Manager"))
	static bool HasInventoryManager(UObject* Object);

	/**
	 * Get the transaction history from an object's inventory manager.
	 * @param Object The object that has an inventory manager
	 * @return Array of transaction descriptions
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Get Transaction History"))
	static TArray<FString> GetTransactionHistory(UObject* Object);

	/**
	 * Undo the last transaction from an object's inventory manager.
	 * @param Object The object that has an inventory manager
	 * @return true if the transaction was successfully undone, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Undo Last Transaction"))
	static bool UndoLastTransaction(UObject* Object);

	/**
	 * Redo the last undone transaction from an object's inventory manager.
	 * @param Object The object that has an inventory manager
	 * @return true if the transaction was successfully redone, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Redo Transaction"))
	static bool RedoTransaction(UObject* Object);

	/**
	 * Clear the transaction history from an object's inventory manager.
	 * @param Object The object that has an inventory manager
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Clear Transaction History"))
	static void ClearTransactionHistory(UObject* Object);
};
