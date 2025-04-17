// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/RockInventory.h"
#include "RockInventoryComponent.generated.h"


class URockInventoryConfig;

/**
 * RockInventoryComponent provides inventory management functionality for actors
 * Manages item storage, retrieval, and organization based on a configurable inventory layout
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYRUNTIME_API URockInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Sets default values for this component's properties */
	URockInventoryComponent( const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get() );

	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Configuration for the inventory size, layout, and properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RockInventory")
	TObjectPtr<URockInventoryConfig> InventoryConfig;

	/** The underlying inventory data */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RockInventory")
	TObjectPtr<URockInventory> Inventory;

	/**
	 * Adds an item to the inventory
	 * @param InItemStack The item and amount to add
	 * @param outHandle Output parameter with handle to the slot where item was placed
	 * @param OutExcess Output parameter with the quantity that couldn't be added due to space limitations
	 * @return True if at least some of the item was added
	 */
	UFUNCTION(BlueprintCallable, Category="RockInventory|Items", Meta=(DisplayName="Add Item"))
	bool K2_AddItem(const FRockItemStack& InItemStack, FRockInventorySlotHandle& outHandle, int32& OutExcess);
	
	UFUNCTION(BlueprintCallable, Category="RockInventory|Items", Meta=(DisplayName="Loot Item"))
	bool K2_LootItem(const FRockItemStack& InItemStack, FRockInventorySlotHandle& outHandle, int32& OutExcess);

	// After calling this, the item will cease to exist in this inventory, do something with it! 
	UFUNCTION(BlueprintCallable, Category="RockInventory|Items", Meta=(DisplayName="Loot Item"))
	FRockItemStack K2_DropItem(const FRockInventorySlotHandle& SlotHandle);
	
	/**
	 * Removes an item from the inventory
	 * @param InHandle The handle to the slot where the item is located
	 * @return The item stack that was removed
	 */
	UFUNCTION(BlueprintCallable, Category="RockInventory|Items", Meta=(DisplayName="Remove Item"))
	FRockItemStack K2_RemoveItem(const FRockInventorySlotHandle& InHandle);


	// Misc
	bool K2_HasItem(FName ItemId, int32 MinQuantity);
	int32 K2_GetItemCount(FName ItemId);
	
#if WITH_EDITOR
	// Validation
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
