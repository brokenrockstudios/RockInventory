// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/RockItemStack.h"
#include "RockInventoryWorldItemSpawner.generated.h"

UCLASS()
class ROCKINVENTORYRUNTIME_API ARockInventoryWorldItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARockInventoryWorldItemSpawner();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStack ItemStack;

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SpawnWorldItem();

};
