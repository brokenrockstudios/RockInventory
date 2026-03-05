// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "Item/RockItemFragment.h"
#include "RockItemFragment_Actor.generated.h"

/**
 *  Generic fragment for spawning an Actor from an item. 
 *  This is meant to be used as a base for more specific Actor fragments that may need additional data such as spawn transforms, spawn effects, etc
 */
USTRUCT()
struct ROCKINVENTORYRUNTIME_API FRockItemFragment_Actor : public FRockItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<AActor> ActorClass;
};


/** Generic fragment for spawning an Actor from an item using a soft reference. */
USTRUCT()
struct ROCKINVENTORYRUNTIME_API FRockItemFragment_SoftActor : public FRockItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSoftClassPtr<AActor> SoftActorClass;
};
