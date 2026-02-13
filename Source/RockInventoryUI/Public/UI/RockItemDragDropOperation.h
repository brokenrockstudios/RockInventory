// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/RockDragCarryOperation.h"
#include "Core/RockItemDragCarrySubsystem.h"
#include "Core/Shared/RockCarryContextData.h"
#include "Engine/AssetManager.h"
#include "Inventory/RockSlotHandle.h"
#include "Kismet/GameplayStatics.h"
#include "Library/RockInventoryLibrary.h"
#include "RockItemDragDropOperation.generated.h"

class URockInventory;


inline void LoadAndPlaySFX(const UObject* WorldContext, const TSoftObjectPtr<USoundBase>& SFX)
{
	if (!WorldContext || !SFX.ToSoftObjectPath().IsValid())
	{
		return;
	}

	if (SFX.IsValid())
	{
		UGameplayStatics::PlaySound2D(WorldContext->GetWorld(), SFX.Get());
	}
	else
	{
		UAssetManager::GetStreamableManager().RequestAsyncLoad(SFX.ToSoftObjectPath(),
			[WeakContext = TWeakObjectPtr<const UObject>(WorldContext), SFX]()
			{
				if (WeakContext.IsValid() && SFX.IsValid())
				{
					UGameplayStatics::PlaySound2D(WeakContext->GetWorld(), SFX.Get());
				}
			});
	}
}


// firearm
// weapon mesh
// weapon sides
// attachment of weapon sides

// weapon mesh is set in child blueprint






UCLASS()
class UInventoryCarryContextData : public URockCarryContextData
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY()
	FRockInventorySlotHandle TargetSlotHandle;
};


/**
 * 
 */
// This is the base class for drag and drop operations in the Rock Inventory UI.
UCLASS(BlueprintType, Blueprintable, Abstract)
class ROCKINVENTORYUI_API URockItemDragDropOperation : public URockDragCarryOperation
{
	GENERATED_BODY()

public:
	// From Inventory, From Slot, and Item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	TObjectPtr<AController> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	TObjectPtr<URockInventory> SourceInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	FRockInventorySlotHandle SourceSlotHandle;

	// Customizations
	// Override in BP version
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	FVector DropLocationOffset = FVector(150, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	FVector DropImpulse = FVector(0, 0, 0);
	
	// UPROPERTY()
	// FRockItemStack ItemStack;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	FRockMoveItemParams MoveItemParams;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	ERockItemOrientation Orientation = ERockItemOrientation::Horizontal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	ERockItemMoveMode MoveMode = ERockItemMoveMode::SingleItem;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	int32 MoveCount = 1;

	bool bRunOnce = false;

	virtual void OnBeginCarry_Implementation() override;
	virtual void OnCancelCarry_Implementation() override;
	virtual void OnFinishedCarry_Implementation() override;
	virtual FRockDropOutcome OnUnhandledDrop_Implementation() override;
	virtual void PlayFeedbackForOutcome_Implementation(const FRockDropOutcome& Outcome);
	
	// Set this in the blueprint parent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop")
	TObjectPtr<USoundBase> DefaultDropSound = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop")
	TObjectPtr<USoundBase> DefaultDragSound = nullptr;
};

// Overlap mode potential default colors?
// CanOverlap	| 007209FF | 00B334FF
// CantOverlap	| 3B0604FF | 852D23FF
// CanMerge		| 5D5704FF | A39E21FF
// CanPush		| 532B00FF | 9B7200FF

// Note: CanPush is basically can nest into it's sub inventory




