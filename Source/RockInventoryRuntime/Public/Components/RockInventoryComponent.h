// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/RockInventory.h"
#include "RockInventoryComponent.generated.h"


class URockInventoryConfig;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYRUNTIME_API URockInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URockInventoryComponent( const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get() );

	virtual void BeginPlay() override;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RockInventory")
	TObjectPtr<URockInventoryConfig> InventoryConfig;

	// Does blueprints ever need to access this directly?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RockInventory")
	TObjectPtr<URockInventory> Inventory;

	// Set up a config for how to define an inventory
	// e.g. how many slots, what are they (e.g. special slots like chest/armor/backpack/etc..)
	
	UFUNCTION(BlueprintCallable, Category="RockInventory", Meta=(DisplayName="Loot Item"))
	bool K2_AddItem(const FRockItemStack& InItemStack, FRockInventorySlotHandle& outHandle, int32& OutExcess);

	UFUNCTION(BlueprintCallable, Category="RockInventory")
	bool K2_RemoveItem(const FRockItemStack& ItemStack, FRockInventorySlotHandle& outHandle, int32& OutExcess);
	//bool CanItemFitInTab(const FRockItemStack& ItemStack, int32 TabIndex, int32 StartX, int32 StartY);

#if WITH_EDITOR
	// Validation
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
