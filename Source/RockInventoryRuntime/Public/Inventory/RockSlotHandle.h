// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockSlotHandle.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlotHandle
{
	GENERATED_BODY()

	FRockInventorySlotHandle() = default;
	FRockInventorySlotHandle(uint8 InTabIndex, uint8 InX, uint8 InY);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 TabIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Y = 0;

	UPROPERTY()
	bool bInitialized = false;
	bool operator==(const FRockInventorySlotHandle& Other) const;
	friend uint32 GetTypeHash(const FRockInventorySlotHandle& Handle);
	bool IsValid() const;
	FString GetDebugString() const;
		
	// Generate Invalid Entry
	static FRockInventorySlotHandle Invalid();
};
