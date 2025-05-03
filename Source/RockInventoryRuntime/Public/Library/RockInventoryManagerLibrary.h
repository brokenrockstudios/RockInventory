// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
	 * Get the inventory manager from an object.
	 * @param Object The object that is the inventory manager, or can get the inventory manager.
	 * @return The inventory manager if found, nullptr otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Get Inventory Manager"))
	static URockInventoryManagerComponent* GetInventoryManager(UObject* Object);

	/**
	 * Check if an object has an inventory manager.
	 * @param Object The object to check
	 * @return true if the object has an inventory manager, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Has Inventory Manager"))
	static bool HasInventoryManager(UObject* Object);

	/**
	 * Clear the transaction history from an object's inventory manager.
	 * @param Object The object that has an inventory manager
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transaction", meta = (DisplayName = "Clear Transaction History"))
	static void ClearTransactionHistory(UObject* Object);
};
