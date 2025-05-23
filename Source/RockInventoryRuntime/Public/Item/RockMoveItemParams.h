// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enums/RockEnums.h"
#include "Enums/RockItemOrientation.h"
#include "RockMoveItemParams.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockMoveItemParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERockItemOrientation DesiredOrientation = ERockItemOrientation::Horizontal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERockItemMoveMode MoveMode = ERockItemMoveMode::FullStack;

	// MoveCount <= 0 means full stack.  Should it be < or <=, or is there any special case where moving 0 items would be valid? 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MoveCount = -1;
};
