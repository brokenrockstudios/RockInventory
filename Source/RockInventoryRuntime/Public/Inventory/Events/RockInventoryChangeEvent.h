// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "UObject/Object.h"

#include "RockInventoryChangeEvent.generated.h"

class URockInventory;

UENUM(BlueprintType)
enum class ERockInventoryChangeType : uint8
{
	None,
	ItemAdded,
	ItemRemoved,
	ItemMoved,
	ItemUpdated,
	
	TabAdded,
	TabRemoved,
	TabUpdated,
	
	SlotAdded,
	SlotRemoved,
	SlotUpdated
};

/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryChangeEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ERockInventoryChangeType ChangeType = ERockInventoryChangeType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRockItemStack PreviousItem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRockItemStack CurrentItem;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<URockInventory> SourceInventory;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRockInventorySlotHandle SourceSlotHandle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<URockInventory> TargetInventory;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRockInventorySlotHandle TargetSlotHandle;
};
