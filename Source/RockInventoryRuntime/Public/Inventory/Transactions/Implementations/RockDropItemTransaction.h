// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Transactions/Core/RockInventoryTransaction.h"
#include "RockDropItemTransaction.generated.h"

class ARockInventoryWorldItem;
/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockDropItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()

public:
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
	void Initialize(AController* InInstigator, URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle);
	virtual bool Execute_Implementation() override;
	virtual bool Undo_Implementation() override;
	virtual bool CanUndo() const override;
	virtual bool CanApply(URockInventoryComponent* OwnerInventory) const override;
	virtual FString GetDescription() const override;
};

