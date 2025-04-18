// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Inventory/RockSlotHandle.h"
#include "RockItemDragDropOperation.generated.h"

class URockInventory;
/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// From Inventory, From Slot, and Item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	TObjectPtr<AController> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	TObjectPtr<URockInventory> SourceInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	FRockInventorySlotHandle SourceSlot;

	// Customizations
	// Override in BP version
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	FVector DropLocationOffset = FVector(150, 0, 0);


	// We can get the itemstack if we need it from the sourceslot's item handle
	// UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "DragDrop")
	// FRockItemStack SourceStack;

	virtual void Dragged_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void Drop_Implementation(const FPointerEvent& PointerEvent) override;

	// virtual void Drop_Implementation(const FPointerEvent& PointerEvent) override;
};


