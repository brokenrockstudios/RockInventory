// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStack.h"
#include "UObject/Object.h"
#include "RockItemInstance.generated.h"

class URockInventory;
/**
 * 
 */
// Base class for all item instances
UCLASS(BlueprintType)
class ROCKINVENTORY_API URockItemInstance : public UObject
{
	GENERATED_BODY()

	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory")
	TWeakObjectPtr<URockInventory> Owner = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory")
	TObjectPtr<URockItemDefinition> CachedDefinition = nullptr;
	///////////////////////////////////////////////////////////////////////////
	//~ Begin UObject interface
	virtual bool IsSupportedForNetworking() const override;
	//~ End UObject interface
	///////////////////////////////////////////////////////////////////////////
public:
	URockInventory* GetOwner() const { return Owner.Get(); }
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
