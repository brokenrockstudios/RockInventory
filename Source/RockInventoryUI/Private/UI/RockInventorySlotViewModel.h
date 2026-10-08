// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Client/RockInventoryClientModel.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStackHandle.h"
#include "RockInventorySlotViewModel.generated.h"

class URockItemDefinition;
class URockInventory;
class URockInventoryClientModel;
struct FRockInventoryPresentationDiff;
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
	
	void OnModelChanged(URockInventoryClientModel& Changed, const FRockInventoryPresentationDiff& Diff);

protected:
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify)
	TObjectPtr<URockInventory> Inventory;

	/** The client model this view model reads and listens to (T-77). */
	UPROPERTY(Transient)
	TObjectPtr<URockInventoryClientModel> Model;
	
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify)
	FRockInventorySlotHandle SlotHandle;
	
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify)
	FRockItemStackHandle ItemHandle;
	
	// GroupTag and underlying ItemID?
};
