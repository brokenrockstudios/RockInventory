// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockInventoryTransaction.generated.h"


USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemTransactionBase
{
	GENERATED_BODY()
	FRockItemTransactionBase();
	explicit FRockItemTransactionBase(AController* controller);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AController> Instigator = nullptr;
	/**
	 * The per-player sequence number the manager component gives a command when it sends it (0 = unsequenced, built by code that does not
	 * go through the manager). The server acks it and refuses a number it has already seen; it is not unique across players.
	 */
	UPROPERTY()
	int32 TransactionID = 0;
};
