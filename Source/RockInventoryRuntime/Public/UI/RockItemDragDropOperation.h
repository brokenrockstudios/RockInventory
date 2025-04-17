// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
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
	TObjectPtr<URockInventory> SourceInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = true))
	FRockInventorySlotHandle SourceSlot;

	// We can get the itemstack if we need it from the sourceslot's item handle
	// UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "DragDrop")
	// FRockItemStack SourceStack;
};
