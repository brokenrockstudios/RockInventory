// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemFragment.h"

#include "RockInventoryTestFragments.generated.h"

/** Refuses to combine two stacks when either one has CustomValue1 above Limit (so two equal stacks above it still refuse). */
USTRUCT()
struct FRockTestFragment_CombineLimit : public FRockItemFragment
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Limit = 0;

	virtual bool CanCombineItemStack(const FRockItemStack& ItemStack, const FRockItemStack& OtherItemStack) const override
	{
		return ItemStack.GetCustomValue1() <= Limit && OtherItemStack.GetCustomValue1() <= Limit;
	}
};

/** Always refuses to combine. */
USTRUCT()
struct FRockTestFragment_NeverCombine : public FRockItemFragment
{
	GENERATED_BODY()

	virtual bool CanCombineItemStack(const FRockItemStack& ItemStack, const FRockItemStack& OtherItemStack) const override
	{
		return false;
	}
};
