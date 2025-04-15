// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RockInventoryConfig.generated.h"

struct FRockInventorySectionInfo;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// Define the height/width and tabID for each tab.
	// A character might have 8 tabs. 7 tabs of 1x1 for character slots (head, armor, gun, etc.) and 1 large of 8x5 for general inventory
	// For example, a Backpack might have a single tab of 4x5
	// A chest rig, might have 4 tabs of 1x2
	// Note: that the TabIndex will equal the order they are defined here.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FRockInventorySectionInfo> InventoryTabs;

};
