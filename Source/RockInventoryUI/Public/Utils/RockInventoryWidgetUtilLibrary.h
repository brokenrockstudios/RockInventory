// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventoryWidgetUtilLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventoryWidgetUtilLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	static int32 GetIndexFromPosition(const FIntPoint& Position, const int32 Columns);
	static FIntPoint GetPositionFromIndex(const int32 Index, const int32 Columns);
};
