// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStack.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventorySlot.h"
#include "Inventory/RockSlotHandle.h"
#include "UObject/Object.h"
#include "RockItemInstance.generated.h"

class URockInventory;
/**
 * 
 */
// Base class for all item instances
UCLASS(BlueprintType)
class ROCKINVENTORYRUNTIME_API URockItemInstance : public UObject
{
	GENERATED_BODY()

	
protected:
	// Make sure this gets updated if the item moves inventory! 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory")
	TWeakObjectPtr<URockInventory> OwningInventory = nullptr;
	
	// Slot of the corresponding item in the inventory
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory")
	FRockSlotHandle SlotHandle;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory")
	TObjectPtr<URockItemDefinition> CachedDefinition = nullptr;

	///////////////////////////////////////////////////////////////////////////
	//~ Begin UObject interface
	virtual bool IsSupportedForNetworking() const override;
	//~ End UObject interface
	///////////////////////////////////////////////////////////////////////////
public:
	void SetOwningInventory(URockInventory* InOwningInventory) { OwningInventory = InOwningInventory; }
	URockInventory* GetOwningInventory() const { return OwningInventory.Get(); }
	void SetSlotHandle(FRockSlotHandle SlotHandle);
	FRockSlotHandle GetSlotHandle() const { return SlotHandle; }
	FRockInventorySlot* GetItemSlot() const { return GetOwningInventory()->GetSlotByHandle(GetSlotHandle()); }

	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	const URockItemDefinition* GetItemDefinition() const;

	UFUNCTION(BlueprintCallable)
	bool FindItemStackForThisInstance(FRockItemStack& OutItemStack) const;
	
	UFUNCTION(BlueprintCallable)
	bool FindItemSlotForThisInstance(FRockInventorySlot& OutItemStack) const;
	

	// TODO: FGameplayTagStackContainer StatTags;
	// TODO: Should all ItemInstances have nested inventories?
	// TODO: Should all ItemInstances have Fragments?
};
