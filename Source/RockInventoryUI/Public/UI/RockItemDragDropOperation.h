// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Inventory/RockSlotHandle.h"
#include "Library/RockInventoryLibrary.h"
#include "RockItemDragDropOperation.generated.h"

class URockInventory;
/**
 * 
 */
// This is the base class for drag and drop operations in the Rock Inventory UI.
UCLASS(BlueprintType, Blueprintable, Abstract)
class ROCKINVENTORYUI_API URockItemDragDropOperation : public UDragDropOperation
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

	virtual void Dragged_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void Drop_Implementation(const FPointerEvent& PointerEvent) override;

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




