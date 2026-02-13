// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventorySectionInfoLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventorySectionInfoLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	// Generate getters for SectionInfo
public:
	UFUNCTION(BlueprintPure, Category = "RockInventory|SectionInfo")
	static FName GetSectionName(const FRockInventorySectionInfo& SectionInfo);
};
