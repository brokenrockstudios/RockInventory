// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/RockPendingSlotOperation.h"
#include "StructUtils/InstancedStruct.h"
#include "Transactions/Implementations/RockDropItemTransaction.h"
#include "Transactions/Implementations/RockLootWorldItemTransaction.h"
#include "Transactions/Implementations/RockMoveItemTransaction.h"
#include "RockInventoryManagerComponent.generated.h"

class URockInventory;
class URockInventoryComponent;

USTRUCT()
struct FRockInventoryTransactionRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TInstancedStruct<FRockItemTransactionBase> Command;

	UPROPERTY()
	TInstancedStruct<FRockItemTransactionBase> Undo;

	template<typename CommandT, typename UndoT>
	void Set(const CommandT& Cmd, const UndoT& UndoData)
	{
		Command.InitializeAs<CommandT>(Cmd);
		Undo.InitializeAs<UndoT>(UndoData);
	}

	bool ExecuteUndo();
};

// Should put this on the PlayerController?
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYRUNTIME_API URockInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URockInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
private:
	// TODO: Spin into a custom CircularBuffer later...
	UPROPERTY()
	TArray<FRockInventoryTransactionRecord> TransactionHistoryData;
	
	// Current position in the transaction history
	int32 CurrentTransactionIndex = -1;
	// Maximum history length
	int32 MaxHistoryLength = 25;
	
	bool bAwaitingServerSync = false;
	bool bHasPendingPredictiveMove = false;
	bool bEnablePredictiveExecution = false;
	// This was used to track pending transactions form the client that were sent to the server. Waiting on 'reconcilation'
	// UPROPERTY()
	// TMap<int32, FRockInventoryPendingTransaction> PendingServerTransactions;
	
public:
	/**
	 * Send transaction result to client
	 *
	 * @param ClientTransactionID - The transaction ID to send the result for
	 * @param bSuccess - Whether the transaction was successful or not
	 */
	UFUNCTION(Client, Reliable)
	void Client_TransactionResult(int32 ClientTransactionID, bool bSuccess);
	void Client_TransactionResult_Implementation(int32 ClientTransactionID, bool bSuccess);

	// Basic Inventory CRUD functions
	UFUNCTION(BlueprintCallable)
	void LootWorldItem(const FRockLootWorldItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_LootWorldItem(FRockLootWorldItemTransaction ItemTransaction);
	void Server_LootWorldItem_Implementation(FRockLootWorldItemTransaction ItemTransaction);
	
	UFUNCTION(BlueprintCallable)
	bool MoveItem(const FRockMoveItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_MoveItem(FRockMoveItemTransaction ItemTransaction);
	void Server_MoveItem_Implementation(FRockMoveItemTransaction ItemTransaction);

	UFUNCTION(BlueprintCallable)
	void DropItem(const FRockDropItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_DropItem(FRockDropItemTransaction ItemTransaction);
	void Server_DropItem_Implementation(FRockDropItemTransaction ItemTransaction);


	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RegisterSlotStatus(URockInventory* Inventory, AController* Instigator, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus);
	void Server_RegisterSlotStatus_Implementation(URockInventory* Inventory, AController* Instigator, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_ReleaseSlotStatus(URockInventory* Inventory, AController* Instigator, const FRockInventorySlotHandle& InSlotHandle);
	void Server_ReleaseSlotStatus_Implementation(URockInventory* Inventory, AController* Instigator, const FRockInventorySlotHandle& InSlotHandle);

	
	// Clear transaction history
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	void ClearHistory();

	
	// Deprioritizing undo and redo for now.
	//
	// // Undo the last transaction
	// UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	// bool UndoLastTransaction();
	//
	// // Redo the last undone transaction
	// UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	// bool RedoTransaction();
	//
	//
	// // Check if can undo
	// UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	// bool CanUndo() const;
	//
	// // Check if can redo
	// UFUNCTION(BlueprintCallable, Category = "Inventory|Transactions")
	// bool CanRedo() const;
};


