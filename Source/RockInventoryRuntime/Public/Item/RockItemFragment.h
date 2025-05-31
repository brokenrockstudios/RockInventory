// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStack.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"

#include "RockItemFragment.generated.h"

struct FRockItemStack;
/**
 * Base struct for item fragment data types used by the Rock Inventory system.
 * Derived structs define specific fragment behavior that can be attached to items.
 */
USTRUCT(BlueprintType, meta=(DisplayName="Item Fragment"))
struct ROCKINVENTORYRUNTIME_API FRockItemFragment
{
	GENERATED_BODY()
	FRockItemFragment() = default;
	virtual ~FRockItemFragment() = default;

	// The fragment might have an opinion about combining stacks.
	virtual bool CanCombineItemStack(const FRockItemStack& ItemStack, const FRockItemStack& OtherItemStack) const
	{
		return true;
	}
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const;
#endif // WITH_EDITOR
};

#if WITH_EDITOR
inline EDataValidationResult FRockItemFragment::IsDataValid(FDataValidationContext& Context) const
{
	return EDataValidationResult::Valid;
}
#endif // WITH_EDITOR


/**
 * Instance of a fragment type that can be used to store and retrieve data.
 */
USTRUCT(BlueprintType, meta=(DisplayName="Item Definition Fragment Instance"))
struct ROCKINVENTORYRUNTIME_API FRockItemFragmentInstance
{
	GENERATED_BODY()
	FRockItemFragmentInstance() = default;

	template <typename T>
		requires std::is_base_of_v<FRockItemFragment, std::decay_t<T>>
	static FRockItemFragmentInstance Make(T&& InStruct)
	{
		using DecayedType = std::decay_t<T>;
		const DecayedType& Payload = InStruct;

		FRockItemFragmentInstance Instance;
		Instance.Fragment.InitializeAsScriptStruct(
			TBaseStructure<DecayedType>::Get(),
			Payload
		);
#if WITH_EDITORONLY_DATA
		Instance.FragmentGuid = FGuid::NewGuid();
#endif
		return Instance;
	}

	/** Get the fragment as a specific type */
	template <typename T>
		requires std::is_base_of_v<FRockItemFragment, std::decay_t<T>>
	const T* GetFragmentData() const
	{
		return Fragment.GetPtr<T>();
	}

	/** Get mutable fragment as a specific type */
	// template <typename T>
	// requires std::is_base_of_v<FRockItemFragment, std::decay_t<T>>
	// T* GetMutableFragment()
	// {
	// 	return Fragment.GetMutablePtr<T>();
	// }

	/** Check if the fragment is of a specific type */
	template <typename T>
		requires std::is_base_of_v<FRockItemFragment, std::decay_t<T>>
	bool IsFragmentType() const
	{
		return Fragment.IsValid() && Fragment.GetScriptStruct() == TBaseStructure<T>::Get();
	}

	/** Check if the fragment data is valid */
	bool IsValid() const { return Fragment.IsValid(); }
	/** Get the type name of the fragment for debugging */
	FString GetFragmentTypeName() const { return Fragment.IsValid() ? Fragment.GetScriptStruct()->GetName() : TEXT("Invalid"); }


	UPROPERTY(EditDefaultsOnly, Category = "Item", NoClear, meta=(ShowOnlyInnerProperties))
	TInstancedStruct<FRockItemFragment> Fragment;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly, Category = "Item", meta=(EditConditionHides,EditCondition=false))
	FGuid FragmentGuid;
#endif // WITH_EDITORONLY_DATA
};
