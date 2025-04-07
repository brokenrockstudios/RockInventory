// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStack.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"

#include "RockItemDefinitionFragment.generated.h"

/**
 *
 */

// These are the fragments that are used to define the item.
// Use a different fragment time for anything 'instanced'.
USTRUCT(BlueprintType, Blueprintable, meta=(DisplayName="Item Definition Fragment"))
struct ROCKINVENTORYRUNTIME_API FRockItemDefinitionFragment
{
	GENERATED_BODY()

public:
	virtual ~FRockItemDefinitionFragment() = default;

	virtual bool CanCombineItemStack(const FRockItemStack& ItemStack, const FRockItemStack& OtherItemStack) const
	{
		return true;
	}

	// Do we want a function to handle 'how' they are combined?

	// What other functions to run on itemdefinition?
};

USTRUCT(BlueprintType, Blueprintable, meta=(DisplayName="Item Definition Fragment Instance"))
struct ROCKINVENTORYRUNTIME_API FRockItemDefinitionFragmentInstance
{
	GENERATED_BODY()

public:
	FRockItemDefinitionFragmentInstance() = default;

	template <typename T>
		requires std::is_base_of_v<FRockItemDefinitionFragment, std::decay_t<T>>
	static FRockItemDefinitionFragmentInstance Make(T&& InStruct)
	{
		using DecayedType = std::decay_t<T>;
		const DecayedType& TypedStruct = static_cast<const DecayedType&>(InStruct);

		FRockItemDefinitionFragmentInstance Instance;
		Instance.Fragment.InitializeAsScriptStruct(
			TBaseStructure<DecayedType>::Get(),
			static_cast<const uint8*>(static_cast<const void*>(&TypedStruct))
		);
#if WITH_EDITORONLY_DATA
#endif
		return Instance;
	}

	UPROPERTY(EditDefaultsOnly, meta=(ShowOnlyInnerProperties))
	TInstancedStruct<FRockItemDefinitionFragment> Fragment;
};
