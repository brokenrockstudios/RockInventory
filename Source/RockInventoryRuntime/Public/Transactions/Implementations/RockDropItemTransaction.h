// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enums/RockItemOrientation.h"
#include "Inventory/RockSlotHandle.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "RockDropItemTransaction.generated.h"

class URockInventory;
class ARockInventoryWorldItemBase;

USTRUCT(BlueprintType)
struct FRockDropItemUndoTransaction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSuccess = false;

	// Place this item
	UPROPERTY()
	TWeakObjectPtr<ARockInventoryWorldItemBase> SpawnedItemStack = nullptr;
	// In this inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	// In this slot
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;
	// with this orientation
	UPROPERTY()
	ERockItemOrientation ExistingOrientation = ERockItemOrientation::Horizontal;

	bool CanUndo();
	bool Undo();
};


USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockDropItemTransaction : public FRockItemTransactionBase
{
	GENERATED_BODY()
	FRockDropItemTransaction() = default;

	FRockDropItemTransaction(
		AController* InInstigator, URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle
		, const FVector& InDropLocationOffset = FVector::ZeroVector, const FVector& InImpulse = FVector::ZeroVector
	)
		: Super(InInstigator), SourceInventory(InSourceInventory), SourceSlotHandle(InSourceSlotHandle),
		  DropLocationOffset(InDropLocationOffset), Impulse(InImpulse)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle SourceSlotHandle;

	// Relative to the instigator's pawn location.
	// NetQuantize10 is used to reduce the size of the vector for replication. 72 bits as opposed to original 96 bits
	// Behaves like a FVector otherwise. 

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize10 DropLocationOffset = FVector::ZeroVector;
	// For a forward 'toss' velocity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize Impulse = FVector::ZeroVector;

	bool CanExecute() const;
	FRockDropItemUndoTransaction Execute() const;
	bool AttemptPredict() const;
};
