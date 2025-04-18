// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/RockSlotHandle.h"
#include "RockInventory_Slot_BackgroundBase.generated.h"

class URockInventory;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventory_Slot_BackgroundBase : public UUserWidget
{
	GENERATED_BODY()

public:
	URockInventory_Slot_BackgroundBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Information about this slot, the TabIndex, X, and Y position. So that drag/drop and other things can know what we are hovering over
	// or dropping on.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FRockInventorySlotHandle SlotHandle;

	// We might not need this, but it is here for now. This is the inventory that this slot belongs to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	TObjectPtr<URockInventory> Inventory;

	// In the event we had some information like if this slot is 'locked' or 'expecting a specific item
	// we might bind that here
	// We might bind some 'color highlight' and other functionality here.
	// The item itself will exist on the canvas panel in front of this background widget.
};
