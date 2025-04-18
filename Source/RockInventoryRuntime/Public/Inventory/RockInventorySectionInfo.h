// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"

#include "RockInventorySectionInfo.generated.h"

/**
 * Tab dimension info
 * This struct is used to define the dimensions of a tab or collection in a single inventory system.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySectionInfo
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return Width > 0 && Height > 0 && FirstSlotIndex >= 0 && SectionName != NAME_None;
	}

	/** Unique identifier for this tab */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SectionName;

	/** First slot index in the AllSlots array */
	UPROPERTY()
	int32 FirstSlotIndex = 0;

	/** Grid width in slots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Width = 0;

	/** Grid height in slots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Height = 0;

	/** Optional tags to filter items in this tab.
	 * e.g. a Head Slot only accepts hat items, or weapons only accept weapons, Keychain only accepts keys. 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagQuery TabFilter;

	/** Total number of slots in this tab */
	int32 GetNumSlots() const;

	/** Returns the width of the tab */
	int32 GetWidth() const;

	/** Returns the height of the tab */
	int32 GetHeight() const;

	static FRockInventorySectionInfo Invalid();
};
