// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once
#include "RockInventoryTransaction.h"

#include "RockMoveItemTransaction.generated.h"

/**
 * 
 */

UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockMoveItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle SourceSlotHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;


	virtual bool Execute_Implementation() override
	{
		return URockInventoryLibrary::MoveItem(SourceInventory, SourceSlotHandle, TargetInventory, TargetSlotHandle);
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
