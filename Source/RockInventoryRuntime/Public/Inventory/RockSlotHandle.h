// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockSlotHandle.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockSlotHandle
{
	GENERATED_BODY()

	FRockSlotHandle() = default;

	FRockSlotHandle(uint8 InTabIndex, uint8 InX, uint8 InY)
		: TabIndex(InTabIndex)
		  , X(InX)
		  , Y(InY)
	{
		bInitialized = true;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 TabIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Y = 0;

	UPROPERTY()
	bool bInitialized = false;
	bool operator==(const FRockSlotHandle& Other) const;
	bool IsValid() const;
};

