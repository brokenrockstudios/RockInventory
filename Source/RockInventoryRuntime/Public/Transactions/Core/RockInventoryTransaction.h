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
	UPROPERTY()
	int32 TransactionID = 0;

	void GenerateNewHandle();
};
