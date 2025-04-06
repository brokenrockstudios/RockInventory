// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockItemStack.generated.h"
//test

class URockItemInstance;
class URockItemDefinition;

// a stack of items
USTRUCT(BlueprintType)
struct ROCKINVENTORY_API FRockItemStack
{
	GENERATED_BODY()
	//  : public TSharedFromThis<FRockItemStack, ESPMode::ThreadSafe>
	
	// ID to look up the definition in your registry
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StackSize;

	// CustomValues are a generic value that can be used for anything. To avoid needing a Runtime Instance
	// e.g. Durability, ChargeCount, Seed for RNG, Bitmask, progress, timer reference, variant id, or level
	// If you want or need more than one of these, you should probably use a runtime instance or modify for a 2nd CustomValue. 
	// Generally I imagine most of the time it should just be Durability, ChargeCount, or something like that.
	UPROPERTY()
	int32 CustomValue1 = 0;

	UPROPERTY()
	int32 CustomValue2 = 0;

	UPROPERTY()
	TObjectPtr<URockItemInstance> RuntimeInstance = nullptr;

	/** Resolve and cache the item definition */
	void SetDefinition(URockItemDefinition* InDefinition);
	const URockItemDefinition* GetDefinition() const;
	UPROPERTY(Transient, NotReplicated)
	TObjectPtr<URockItemDefinition> CachedDefinition = nullptr;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	bool operator==(const FRockItemStack& Other) const
	{
		return ItemId == Other.ItemId
			&& StackSize == Other.StackSize
			&& CustomValue1 == Other.CustomValue1
			&& RuntimeInstance == Other.RuntimeInstance;
	}

	FString GetDebugString() const;
};

template <>
struct TStructOpsTypeTraits<FRockItemStack> : public TStructOpsTypeTraitsBase2<FRockItemStack>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
	};
};
