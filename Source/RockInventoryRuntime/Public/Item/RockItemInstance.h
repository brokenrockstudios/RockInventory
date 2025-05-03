// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStack.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventorySlot.h"
#include "Inventory/RockSlotHandle.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "RockItemInstance.generated.h"

class URockInventory;

/**
 * Base class for all item instances in the Rock Inventory system.
 * Item instances represent runtime objects that can have state and behavior associated with them.
 * They are created when an item definition requires runtime instantiation (bRequiresRuntimeInstance = true).
 * 
 * Item instances maintain a reference to their owning inventory and slot, allowing them to access
 * and modify their associated item stack data.
 */
UCLASS(BlueprintType)
class ROCKINVENTORYRUNTIME_API URockItemInstance : public UObject
{
	GENERATED_BODY()

public:
	void Init(URockInventory* InOwningInventory, URockItemDefinition* InDefinition, const FRockInventorySlotHandle& InSlotHandle);

	///////////////////////////////////////////////////////////////////////////
	// Core Properties~

	/** The inventory that currently owns this item instance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	TObjectPtr<URockInventory> OwningInventory = nullptr;

	/** Handle to the slot in the inventory where this item instance is located */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	FRockInventorySlotHandle SlotHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	FRockItemStackHandle ItemHandle;

	/** Cached reference to the item definition for quick access */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	TObjectPtr<URockItemDefinition> CachedDefinition = nullptr;

	/** Gameplay tags associated with this item instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	FGameplayTagContainer StatTags;

	// TODO: Add support for gameplay stack containers
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	// FGameplayStackContainer StatStackContainer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	TObjectPtr<URockInventory> NestedInventory = nullptr;

	// TODO: Add mutable fragments. 
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	// TArray<FRockItemMutableFragments> Fragments;

	///////////////////////////////////////////////////////////////////////////

	/** Sets the owning inventory for this item instance */
	void SetOwningInventory(URockInventory* InOwningInventory);

	/** Gets the owning inventory for this item instance */
	URockInventory* GetOwningInventory() const { return OwningInventory.Get(); }

	/** Sets the slot handle for this item instance */
	void SetSlotHandle(FRockInventorySlotHandle InSlotHandle);

	/** Gets the slot handle for this item instance */
	FRockInventorySlotHandle GetSlotHandle() const { return SlotHandle; }

	/** Gets the inventory slot associated with this item instance */
	FRockInventorySlotEntry GetItemSlot() const;

	/** Gets the item stack associated with this item instance */
	FRockItemStack GetItemStack() const;

	/** Gets the item definition for this item instance */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Core")
	const URockItemDefinition* GetItemDefinition() const;

protected:
	///////////////////////////////////////////////////////////////////////////
	// UObject Interface
	virtual bool IsSupportedForNetworking() const override;
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
};
