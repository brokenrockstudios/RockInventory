// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventorySlot.h"
#include "RockInventoryTabInfo.h"
#include "GameFramework/Actor.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "RockContainerExperiment.generated.h"

USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryData : public FFastArraySerializer
{
	GENERATED_BODY()

	// Replicated list of inventory slots
	UPROPERTY()
	TArray<FRockInventorySlot> AllSlots;
	

	// FFastArraySerializer implementation
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRockInventorySlot, FRockInventoryData>(AllSlots, DeltaParms, *this);
	}

	// ~Begin of TArray interface
	auto& operator[](int32 Index) { return AllSlots[Index]; }
	const auto& operator[](int32 Index) const { return AllSlots[Index]; }
	int32 Num() const { return AllSlots.Num(); }
	/**
	 * DO NOT USE DIRECTLY
	 * STL-like iterators to enable range-based for loop support.
	 */
	auto begin() { return AllSlots.begin(); }
	auto begin() const { return AllSlots.begin(); }
	auto end() { return AllSlots.end(); }
	auto end() const { return AllSlots.end(); }
	auto rbegin() { return AllSlots.rbegin(); }
	auto rbegin() const { return AllSlots.rbegin(); }
	auto rend() { return AllSlots.rend(); }
	auto rend() const { return AllSlots.rend(); }
	// ~End of TArray interface
	
};

template <>
struct TStructOpsTypeTraits<FRockInventoryData> : public TStructOpsTypeTraitsBase2<FRockInventoryData>
{
	enum
	{
		WithNetDeltaSerializer = true,

		// Other possible options?
		// WithIdenticalViaEquality = true,
		// WithNoInitConstructor = true,
		// WithZeroConstructor = true,
		// WithNetSerializer = true,
		// WithNetSharedSerialization = true,
		// WithSerializer = true,
		// WithSerializeFromMismatchedTag = true,
	};
};

