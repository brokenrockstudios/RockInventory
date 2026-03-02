// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Item/RockItemFragment.h"
#include "UObject/Object.h"
#include "RockItemFragment_SetStats.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemFragment_SetStats : public FRockItemFragment
{
	GENERATED_BODY()

	virtual void OnItemCreated(FRockItemStack& ItemStack) const override;

	UPROPERTY(EditDefaultsOnly, Category=Equipment, meta=(Tooltip="This is used to set initial stats on an item's instance if there is one when it's created."))
	TMap<FGameplayTag, int32> InitialItemStats;
	
	// Better way to do this? or should we validate that the Inventory has a matching GameplayTag?
	// Or perhaps we could do an 'editor' trick to only show this if it has a CustomValue1 GameplayTag and/or even show it to the user
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	int32 CustomValue1 = 0;
	
	// Better way to do this? or should we validate that the Inventory has a matching GameplayTag?
	// Or perhaps we could do an 'editor' trick to only show this if it has a CustomValue1 GameplayTag and/or even show it to the user
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	int32 CustomValue2 = 0;
};
