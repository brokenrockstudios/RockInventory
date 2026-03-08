// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Inventory/RockSlotHandle.h"
#include "Inventory/Events/RockItemDelta.h"
#include "Inventory/Events/RockSlotDelta.h"
#include "Item/RockItemStackHandle.h"
#include "RockInventorySlotViewModel.generated.h"

class URockItemDefinition;
class URockInventory;
/**
 * Represents the view model for a single inventory slot which may or may not have an item in it
 * 
 * We always have a BackgroundBase for every 'slot'. 
 * The Widget Slot_ItemBase only exists when there is a matching item.
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventorySlotViewModel: public UMVVMViewModelBase
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable)
	void Initialize(URockInventory* NewInventory, const FRockInventorySlotHandle& NewSlot); //, const FGameplayTag& NewGroupTag)
	
	
	UFUNCTION(BlueprintPure, FieldNotify)
	int32 GetStackCount() const;
	
	UFUNCTION(BlueprintPure, FieldNotify)
	int32 GetMaxStackCount() const;
	
	UFUNCTION(BlueprintPure, FieldNotify)
	URockItemDefinition* GetItemDefinition() const;

	
	URockInventory* GetInventory() const { return Inventory; }
	FRockInventorySlotHandle GetSlotHandle() const { return SlotHandle; }
	FRockItemStackHandle GetItemHandle() const { return ItemHandle; }
	
protected:
	
	UFUNCTION()
	void OnItemChanged(const FRockItemDelta& ItemDelta);
	UFUNCTION()
	void OnSlotChanged(const FRockSlotDelta& SlotDelta);
	
	
protected:
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify)
	TObjectPtr<URockInventory> Inventory;
	
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify)
	FRockInventorySlotHandle SlotHandle;
	
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify)
	FRockItemStackHandle ItemHandle;
	
	// GroupTag and underlying ItemID?
};
