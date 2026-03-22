// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemFragment.h"

#include "RockItemFragment_Sound.generated.h"

/** 
 * This fragment allows you to override specify various sounds related to the item, such as pickup and drop sounds in the world, 
 * and sounds for inventory interactions like grabbing and placing the item during drag-and-drop operations.
 * Most standard item's will likely leverage default inventory sounds. But there are special cases this could be useful
 * e.g., a captured living insect as an 'item' or a magical orb
*/
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemFragment_Sound : public FRockItemFragment
{
	GENERATED_BODY()

public:
	//////////////////////////////////////////////////////////////////////////
	/// Sound

	/** A sound to place when picking up this item from the world */
	UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (AssetBundles= "Gameplay"))
	TSoftObjectPtr<USoundBase> WorldPickup = nullptr;

	/** A sound to play when dropping the item into the world */
	UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (AssetBundles= "Gameplay"))
	TSoftObjectPtr<USoundBase> WorldDrop = nullptr;

	/** When picking up an item onto the mouse during a dragdrop operation*/
	UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (AssetBundles= "UI"))
	TSoftObjectPtr<USoundBase> InventoryPickup = nullptr;

	/** A potentially looping type sound to play while active in a dragdrop operation. (e.g. an animal or bug or orb might emit sounds) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (AssetBundles= "UI"))
	TSoftObjectPtr<USoundBase> InventoryMove = nullptr;

	/** At the conclusion of a dragdrop operation when placing it down, perhaps a deeper thud sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound", meta = (AssetBundles= "UI"))
	TSoftObjectPtr<USoundBase> InventoryDrop = nullptr;
};
