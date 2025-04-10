// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockSlotHandle.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Object.h"

#include "RockInventoryTransaction.generated.h"

UENUM(BlueprintType)
enum class ERockInventoryTransactionType : uint8
{
	None,
	// ItemAdded,
	// ItemRemoved,
	ItemMoved
	// ItemUpdated,

	// TabAdded,
	// TabRemoved,
	// TabUpdated,

	// SlotAdded,
	// SlotRemoved,
	// SlotUpdated
};


// Experimental:
// A transaction Base class
// This is a base class for all transactions. It should be used to create a transaction system.
// It should be used to create a transaction system that can be used to undo/redo transactions.
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryTransaction
{
	GENERATED_BODY()

	// TransactionID?
	// timestamp?
	ERockInventoryTransactionType TransactionType = ERockInventoryTransactionType::None;
	// If a NPC or non player controller modified the inventory.
	// We likely will want to invalidate the transaction history
	// e.g. A NPC gave you a quest item, you don't want to be able to undo that!
	TWeakObjectPtr<APlayerController> Instigator;

	virtual ~FRockInventoryTransaction() = default;
	virtual bool Redo() { return false; }
	virtual bool Undo() { return false; }
	virtual FString GetDescription() const { return TEXT("No Description"); }
};

struct FRockItemMovedTransaction : public FRockInventoryTransaction
{
	URockInventory* SourceInventory = nullptr;
	FRockInventorySlotHandle SourceSlotHandle;
	URockInventory* TargetInventory = nullptr;
	FRockInventorySlotHandle TargetSlotHandle;


	virtual bool Redo() override
	{
		return URockInventoryLibrary::MoveItem(SourceInventory, SourceSlotHandle, TargetInventory, TargetSlotHandle);
	}

	virtual bool Undo() override
	{
		return URockInventoryLibrary::MoveItem(TargetInventory, TargetSlotHandle, SourceInventory, SourceSlotHandle);
	}

	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Move Item"));
		//" from %s to %s"), *SourceInventory->GetDebugString(), *TargetInventory->GetDebugString());
	}
};
