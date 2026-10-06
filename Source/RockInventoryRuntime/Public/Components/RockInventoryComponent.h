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
	URockInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Called when the game starts */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Configuration for the inventory size, layout, and properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RockInventory")
	TObjectPtr<URockInventoryConfig> InventoryConfig;

	// TODO: Consider adding a tarray/variable of controllers currently interacting with this top level Inventory?
	// But to what end? conditional replication?  
	/** The underlying inventory data */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RockInventory", ReplicatedUsing=OnRep_Inventory)
	TObjectPtr<URockInventory> Inventory;
	UFUNCTION()
	void OnRep_Inventory(URockInventory* OldInventory);
	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnInventoryChanged();

	/**
	 * Adds an item to the inventory
	 * @param InItemStack The item and amount to add
	 * @param Params What the call may do (intent, exclusions); the defaults add like a plain pickup
	 * @param OutResult Every placement made and the quantity that couldn't be added due to space limitations
	 * @return True if the whole stack was added
	 */
	UFUNCTION(BlueprintCallable, Category="RockInventory|Items", Meta=(DisplayName="Add Item"))
	bool K2_AddItem(const FRockItemStack& InItemStack, const FRockLootParams& Params, FRockLootResult& OutResult);

	UFUNCTION(BlueprintCallable, Category="RockInventory|Items", Meta=(DisplayName="Loot Item"))
	bool K2_LootItem(const FRockItemStack& InItemStack, const FRockLootParams& Params, FRockLootResult& OutResult);
	// After calling this, the item will cease to exist in this inventory, do something with it!
	UFUNCTION(BlueprintCallable, Category="RockInventory|Items", Meta=(DisplayName="Drop Item"))
	FRockItemStack K2_DropItem(const FRockInventorySlotHandle& SlotHandle);

	/**
	 * Removes an item from the inventory
	 * @param InHandle The handle to the slot where the item is located
	 * @return The item stack that was removed
	 */
	UFUNCTION(BlueprintCallable, Category="RockInventory|Items", Meta=(DisplayName="Remove Item"))
	FRockItemStack K2_RemoveItem(const FRockInventorySlotHandle& InHandle);

	// Misc
	UFUNCTION(BlueprintPure, Category="RockInventory|Items", Meta=(DisplayName="Has Item"))
	bool K2_HasItem(FName ItemId, int32 MinQuantity);
	UFUNCTION(BlueprintPure, Category="RockInventory|Items", Meta=(DisplayName="Get Item Count"))
	int32 K2_GetItemCount(FName ItemId);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
#if WITH_EDITOR
	// Validation
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
