// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemStack.h"
#include "Library/RockInventoryHelpers.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockInventoryItemContainer.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryItemContainer: public FFastArraySerializer
{
	GENERATED_BODY()
private:
	// Force usage of the helpers, and not this array directly. 
	// Replicated list of inventory slots
	UPROPERTY()
	TArray<FRockItemStack> AllSlots;
public:
	ROCKINVENTORY_FASTARRAYSERIALIZER_TARRAY_ACCESSORS(AllSlots);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRockItemStack, FRockInventoryItemContainer>(AllSlots, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FRockInventoryItemContainer> : public TStructOpsTypeTraitsBase2<FRockInventoryItemContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
