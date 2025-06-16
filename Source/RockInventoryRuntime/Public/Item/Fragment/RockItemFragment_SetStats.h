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


	virtual void OnItemCreated(URockItemInstance* ItemInstance) const override;

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TMap<FGameplayTag, int32> InitialItemStats;
};
