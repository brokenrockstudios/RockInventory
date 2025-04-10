// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/RockSlotHandle.h"
#include "RockInventory_Slot_ItemBase.generated.h"

class UImage;
class UTextBlock;
class URockInventory;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventory_Slot_ItemBase : public UUserWidget
{
	GENERATED_BODY()
public:

	// The main widget for an item that will exist in the canvas panel of a Container
	// This needs to adjust it's size based upon the size of the item
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FRockInventorySlotHandle SlotHandle;

	// We might not need this, but it is here for now. This is the inventory that this slot belongs to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	TObjectPtr<URockInventory> Inventory;

	// Bind Image Widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;
	
	// Text widget for displaying item count
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCount;
	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	// Updates the ItemCount text based on current stack size
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateItemCount();
	
	// Callback for when the inventory changes
	UFUNCTION()
	void OnInventoryChanged(URockInventory* ChangedInventory, const FRockInventorySlotHandle& ChangedSlotHandle);
};
