// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

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
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryTransaction
{
	GENERATED_BODY()
	// If a NPC or non player controller modified the inventory.
	// We likely will want to invalidate the transaction history
	// e.g. A NPC gave you a quest item, you don't want to be able to undo that!
	UPROPERTY()
	TWeakObjectPtr<APlayerController> Instigator;

	UPROPERTY()
	FDateTime Timestamp;

	UPROPERTY()
	ERockInventoryTransactionType TransactionType = ERockInventoryTransactionType::None;


	FRockInventoryTransaction()
		: Instigator(nullptr)
		  , Timestamp(FDateTime::Now())
	{
	}

	virtual ~FRockInventoryTransaction() = default;
	virtual bool Execute() { return false; }
	virtual bool Redo() { return Execute(); }
	virtual bool Undo() { return false; }
	/* For commands that can't be undone, override this */
	virtual bool CanUndo() const { return true; }
	virtual FString GetDescription() const { return TEXT("No Description"); }

	// Can check if the transaction can be applied to the inventory (e.g. is there free space, are you 'close enough' to the item
	// other basic checks can be applied here. However, this is not a replacement for the actual transaction logic.
	// Note: We could leverage the OwnerInventory to indirectly get actor's WorldLocation and other stuff
	// It doesn't necessarily mean we are doing something with this particular inventory, as we could be moving items between 2 independent inventories
	virtual bool CanApply(URockInventoryComponent* OwnerInventory) const { return true; }
};


USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockMoveItemTransaction : public FRockInventoryTransaction
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY()
	FRockInventorySlotHandle SourceSlotHandle;
	UPROPERTY()
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY()
	FRockInventorySlotHandle TargetSlotHandle;


	virtual bool Execute() override
	{
		return URockInventoryLibrary::MoveItem(SourceInventory, SourceSlotHandle, TargetInventory, TargetSlotHandle);
	}

	virtual bool Undo() override
	{
		return URockInventoryLibrary::MoveItem(TargetInventory, TargetSlotHandle, SourceInventory, SourceSlotHandle);
	}

	virtual bool CanUndo() const override
	{
		return true;
	}

	virtual bool CanApply(URockInventoryComponent* OwnerInventory) const override
	{
		//URockInventoryLibrary::CanMoveItem(SourceInventory, SourceSlotHandle, TargetInventory, TargetSlotHandle);
		return true;
	}

	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Move Item"));
		//" from %s to %s"), *SourceInventory->GetDebugString(), *TargetInventory->GetDebugString());
	}
};

USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockAddItemTransaction : public FRockInventoryTransaction
{
	GENERATED_BODY()
	UPROPERTY()
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY()
	FRockInventorySlotHandle TargetSlotHandle;
	UPROPERTY()
	FRockItemStack ItemStack;

	virtual bool Execute() override
	{
		return false; // URockInventoryLibrary::AddItemToInventory(TargetInventory, ItemStack, TargetSlotHandle);
	}

	virtual bool Undo() override
	{
		return false; // URockInventoryLibrary::RemoveItem(TargetInventory, ItemStack, TargetSlotHandle);
	}

	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Add Item"));
	}
};
