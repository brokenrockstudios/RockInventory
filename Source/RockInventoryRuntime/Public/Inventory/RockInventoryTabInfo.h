// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"

#include "RockInventoryTabInfo.generated.h"

/**
 * Tab dimension info
 * This struct is used to define the dimensions of a tab or collection in a single inventory system.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryTabInfo
{
    GENERATED_BODY()

    /** Grid width in slots */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    uint8 Width = 0;

    /** Grid height in slots */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    uint8 Height = 0;

    /** Unique identifier for this tab */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName TabID;

    /** First slot index in the AllSlots array */
    UPROPERTY()
    int32 FirstSlotIndex = 0;

    /** Total number of slots in this tab */
    UPROPERTY()
    int32 NumSlots = 0;

    /** Optional tags to filter items in this tab.
     * e.g. a Head Slot only accepts hat items, or weapons only accept weapons, Keychain only accepts keys. 
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagQuery TabFilter;

    /** Returns the width of the tab */
    int32 GetWidth() const;
    
    /** Returns the height of the tab */
    int32 GetHeight() const;
};
