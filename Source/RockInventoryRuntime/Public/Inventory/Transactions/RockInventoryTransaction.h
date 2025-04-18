// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Object.h"

#include "RockInventoryTransaction.generated.h"

// Maybe we want to support asynchronous transactions?
UENUM(BlueprintType)
enum class ERockTransactionState : uint8
{
	Pending,
	Executing,
	Completed,
	Failed,
	Undoing
};

UENUM(BlueprintType)
enum class ERockTransactionResult : uint8
{
	InProgress UMETA(DisplayName = "In Progress", ToolTip = "Transaction is still executing"),
	Complete UMETA(DisplayName = "Complete", ToolTip = "Transaction has completed successfully"),
	Failed UMETA(DisplayName = "Failed", ToolTip = "Transaction has failed"),
};


UENUM(BlueprintType)
enum class ERockInventoryTransactionType : uint8
{
	None,
	AddItem, // instantiate a wholly new item from nowhere
	LootItem, // Get an item from a source that isn't necessarily another container? 
	MoveItem, // Move an item from any inventory to any other inventory.

	// Destroy or remove or consume item?
	// Modify an item somehow?

	// ItemUpdated,

	// TabAdded,
	// TabRemoved,
	// TabUpdated,

	// SlotAdded,
	// SlotRemoved,
	// SlotUpdated

	Max UMETA(Hidden)
};


// Experimental:
// A transaction Base class
// This is a base class for all transactions. It should be used to create a transaction system.
// It should be used to create a transaction system that can be used to undo/redo transactions.
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockInventoryTransaction : public UObject
{
	GENERATED_BODY()
public:
	// If a NPC or non player controller modified the inventory.
	// We likely will want to invalidate the transaction history
	// e.g. A NPC gave you a quest item, you don't want to be able to undo that!
	UPROPERTY()
	TWeakObjectPtr<APlayerController> Instigator;

	UPROPERTY()
	FDateTime Timestamp;

	UPROPERTY()
	ERockInventoryTransactionType TransactionType = ERockInventoryTransactionType::None;


	URockInventoryTransaction()
		: Instigator(nullptr)
		  , Timestamp(FDateTime::Now())
	{
	}


	UFUNCTION(BlueprintNativeEvent, Category="Transaction")
	bool Execute();
	virtual bool Execute_Implementation() { return false; }
	virtual bool Redo() { return Execute(); }
	
	UFUNCTION(BlueprintNativeEvent, Category="Transaction")
	bool Undo();
	virtual bool Undo_Implementation() { return false; }
	
	/* For commands that can't be undone, override this or conditionally check if CanUndo occur */
	virtual bool CanUndo() const { return true; }
	
	virtual FString GetDescription() const { return TEXT("No Description"); }

	// Can check if the transaction can be applied to the inventory (e.g. is there free space, are you 'close enough' to the item
	// other basic checks can be applied here. However, this is not a replacement for the actual transaction logic.
	// Note: We could leverage the OwnerInventory to indirectly get actor's WorldLocation and other stuff
	// It doesn't necessarily mean we are doing something with this particular inventory, as we could be moving items between 2 independent inventories
	virtual bool CanApply(URockInventoryComponent* OwnerInventory) const { return true; }

	// If we decide to support async transactions, we can use this to check if the transaction is still executing
	// We'd need to update the TransactionManager
	// BeginExecute, IsExecuting, UpdateExecuting, OnExecutionComplete
	// BeginUndo, IsUndoing, UpdateUndoing, OnUndoComplete
};

