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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetDefinition(const TObjectPtr<URockItemDefinition>& object);
	
	///////////////////////////////////////////////////////////////////////////
	// Core Properties~

	/** The inventory that currently owns this item instance */
	// Note: This might be null, in the case of a 'world item' that is not currently in an inventory.
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	TObjectPtr<URockInventory> OwningInventory = nullptr;

	/** Handle to the slot in the inventory where this item instance is located */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	FRockInventorySlotHandle SlotHandle;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	FRockItemStackHandle ItemHandle;

	/** Cached reference to the item definition for quick access */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	TObjectPtr<URockItemDefinition> CachedDefinition = nullptr;

	/** Gameplay tags associated with this item instance */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	FGameplayTagContainer StatTags;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	TObjectPtr<URockInventory> NestedInventory = nullptr;


#ifdef BRS_WITH_GAMEPLAYABILITIES
	// TODO: Add support for gameplay stack containers
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	// FGameplayStackContainer StatStackContainer;
#endif // BRS_WITH_GAMEPLAYABILITIES
	
	// TODO: Add mutable fragments. 
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	// TArray<FRockItemMutableFragments> Fragments;

	///////////////////////////////////////////////////////////////////////////

	/** Sets the owning inventory for this item instance */
	void SetOwningInventory(URockInventory* InOwningInventory);
	void RegisterReplicationWithOwner();
	void UnregisterReplicationWithOwner();

	/** Gets the owning inventory for this item instance */
	URockInventory* GetOwningInventory() const;

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
	
#if UE_WITH_IRIS
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS
};
