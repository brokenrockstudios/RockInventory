// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "Transactions/Core/RockInventoryTransaction.h"
#include "RockMoveItemTransaction.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockMoveItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transaction")
	static URockMoveItemTransaction* CreateMoveItemTransaction(
		URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle,
		URockInventory* InTargetInventory, const FRockInventorySlotHandle& InTargetSlotHandle);

	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle SourceSlotHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;

	// User Configurable
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockMoveItemParams MoveParams;

	virtual bool Execute_Implementation() override
	{
		// TODO: We need to save off the existing item's rotation and stack count so we can 'undo' it later.
		
		return URockInventoryLibrary::MoveItem(SourceInventory, SourceSlotHandle, TargetInventory, TargetSlotHandle, MoveParams);
	}

	virtual bool Undo_Implementation() override
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

