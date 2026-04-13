// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStack.h"
#include "UObject/Object.h"

#include "RockItemFragment.generated.h"

class URockItemDefinition;

/**
 * Base struct for item fragment data types used by the Rock Inventory system.
 * Derived structs define specific fragment behavior that can be attached to items.
 */
USTRUCT(BlueprintType, meta=(DisplayName="Item Fragment"))
struct ROCKINVENTORYRUNTIME_API FRockItemFragment
{
	GENERATED_BODY()
	FRockItemFragment() = default;

	// ~Begin Lifecycle events for the fragment. These are called by the owning ItemDefinition or ItemInstance at appropriate times.
	virtual void OnPostLoad(const URockItemDefinition* OwnerDef);
	virtual void OnPostEditChangeProperty(const URockItemDefinition* OwnerDef);
	// ~End Lifecycle event

	// Rule of five: Because the presence of a user-defined destructor should declare all five special member functions
	virtual ~FRockItemFragment() = default;
	// Copy
	FRockItemFragment(const FRockItemFragment&) = default;
	FRockItemFragment& operator=(const FRockItemFragment&) = default;
	// Move
	FRockItemFragment(FRockItemFragment&&) = default;
	FRockItemFragment& operator=(FRockItemFragment&&) = default;

	// Fragment configures the item it's on.  This is what sets any modifiers on the item itself.
	virtual void OnItemCreated(FRockItemStack& ItemStack) const;

	// The fragment might have an opinion about combining stacks.
	virtual bool CanCombineItemStack(const FRockItemStack& ItemStack, const FRockItemStack& OtherItemStack) const;

	// Contribute zero or more stable tokens for asset search/filtering.
	//virtual void GetSearchTokens(TSet<FName>& OutTokens) const;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const;

	// TODO: Add if/when actually have a good use case for this. Such as differentiating the duplicate fragments?
	// UPROPERTY(EditAnywhere, Category = "Inventory", meta = (Categories="FragmentTags"))
	// FGameplayTag FragmentTag = FGameplayTag::EmptyTag;

	// Fragments will auto-sort themselves in the ItemDefinition and/or Instance based upon this value.
	// This enables putting more frequently queried fragments earlier in the TArray to offer slight optimization.
	// COLD PATH: _SetStats that only happens once on item initialization should have a very high positive number.
	// HOT PATH: _Resource, or _Usable likely gets queried a lot more, should have a negative number
	virtual int32 GetSortOrder() const { return 0; }

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context, const URockItemDefinition* OwnerDef) const;
#endif // WITH_EDITOR
};
