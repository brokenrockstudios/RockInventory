// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventorySlot.h"
#include "RockInventoryTabInfo.h"
#include "GameFramework/Actor.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "RockContainerExperiment.generated.h"

USTRUCT(BlueprintType)
struct ROCKINVENTORY_API FRockInventoryData : public FFastArraySerializer
{
	GENERATED_BODY()

	// Single flat array of all slots
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRockInventorySlot> AllSlots;

	// FFastArraySerializer implementation
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRockInventorySlot, FRockInventoryData>(AllSlots, DeltaParms, *this);
	}
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


UCLASS(BlueprintType)
class ROCKINVENTORY_API URockInventory : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	FRockInventoryData InventoryData;

	// Tab configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TArray<FRockInventoryTabInfo> Tabs;

	// Get tab info by index
	const FRockInventoryTabInfo* GetTabInfo(int32 TabIndex) const;
	// Get slot index in the AllSlots array
	int32 GetSlotIndex(int32 TabIndex, int32 X, int32 Y) const;
	// Get slot at specific coordinates in a tab
	FRockInventorySlot* GetSlotAt(int32 TabIndex, int32 X, int32 Y);
	// Add a new tab and initialize its slots
	int32 AddTab(FName TabID, int32 Width, int32 Height);
	// Get all slots in a specific tab - returns array view for efficiency
	TArrayView<FRockInventorySlot> GetTabSlots(int32 TabIndex);
	// Find tab index by name (for UI/scripting)
	int32 GetTabIndexByID(FName TabID) const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
