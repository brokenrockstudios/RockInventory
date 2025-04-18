// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enums/RockEnums.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Object.h"

#include "RockInventoryTransaction.generated.h"


// Experimental:
// A transaction Base class
// This is a base class for all transactions. It should be used to create a transaction system.
// It should be used to create a transaction system that can be used to undo/redo transactions.
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockInventoryTransaction : public UObject
{
	GENERATED_BODY()

public:
	// This should be the Controller if it's a player. Otherwise, it can be the actor of a NPC
	UPROPERTY()
	TWeakObjectPtr<AController> Instigator;

	UPROPERTY()
	FDateTime Timestamp;

	UPROPERTY()
	ERockInventoryTransactionType TransactionType = ERockInventoryTransactionType::None;

	URockInventoryTransaction()
		: Instigator(nullptr)
		  , Timestamp(FDateTime::Now())
	{
	}


	// Normal players shouldn't be calling this, but if you want to exeute the command without a transaction manager, just call this
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Transaction")
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
