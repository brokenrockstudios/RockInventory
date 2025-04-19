// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "RockDropItemTransaction.generated.h"

class ARockInventoryWorldItem;
/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockDropItemTransaction : public URockInventoryTransaction
{
public:
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "RockInventory|Transactions")
	static URockDropItemTransaction* CreateDropItemTransaction(
		AController* InInstigator, URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle);
	
	////////////////////////////////////////////////////////////////////////////////////
	/// Core Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle SourceSlotHandle;

	///////////////////////////////////////////////////////////////////////////////////
	/// Optional Properties
	// Consider optional parameters like where the player is looking or 'hit tracing' or something?
	// Or should that be a different Transaction like Place instead of Drop.

	// Relative to the instigator's pawn location.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector DropLocationOffset = FVector::ZeroVector;
	// For a forward 'toss' velocity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Impulse = FVector::ZeroVector;

	///////////////////////////////////////////////////////////////////////////
	// Required for Undo State
	UPROPERTY()
	TWeakObjectPtr<ARockInventoryWorldItem> SpawnedItemStack = nullptr;
	UPROPERTY()
	ERockItemOrientation ExistingOrientation;

public:
	virtual bool Execute_Implementation() override;
	virtual bool Undo_Implementation() override;
	virtual bool CanUndo() const override;
	virtual bool CanApply(URockInventoryComponent* OwnerInventory) const override;
	virtual FString GetDescription() const override;
};
