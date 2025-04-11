// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockSlotHandle.generated.h"

/**
 * Handle for an inventory slot position.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlotHandle
{
	GENERATED_BODY()

	// Default constructor is an invalid handle
	FRockInventorySlotHandle();
	FRockInventorySlotHandle(uint8 InTabIndex, uint8 InX, uint8 InY);
	virtual ~FRockInventorySlotHandle() = default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 TabIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Y = 0;

	bool IsValid() const;

	// Helper Utility functions
	friend uint32 GetTypeHash(const FRockInventorySlotHandle& Handle);
	virtual uint32 GetHash() const;
	FString ToString() const;
	bool operator==(const FRockInventorySlotHandle& OtherSlotHandle) const = default;
};
