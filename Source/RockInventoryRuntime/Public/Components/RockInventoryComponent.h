// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/RockInventory.h"
#include "RockInventoryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYRUNTIME_API URockInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URockInventoryComponent( const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get() );


	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TObjectPtr<URockInventory> Inventory;


	// Set up a config for how to define an inventory
	// e.g. how many slots, what are they (e.g. special slots like chest/armor/backpack/etc..)
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	// TObjectPtr<URockInventoryConfig> InventoryConfig;

	
};
