// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/RockItemStack.h"
#include "RockInventoryWorldItem.generated.h"

UCLASS()
class ROCKINVENTORYRUNTIME_API ARockInventoryWorldItem : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARockInventoryWorldItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	void SetItemStack(const FRockItemStack& InItemStack);
	
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	FRockItemStack& GetItemStack();

	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStack ItemStack;
};

