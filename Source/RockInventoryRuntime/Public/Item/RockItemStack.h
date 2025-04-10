// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RockItemStack.generated.h"

class URockItemInstance;
class URockItemDefinition;

// a stack of items
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemStack
{
	GENERATED_BODY()

	FRockItemStack();
	FRockItemStack(const FName& InItemId, int32 InStackSize = 1);
	FRockItemStack(URockItemDefinition* InDefinition, int32 InStackSize = 1);
	
	// Is an ItemID even needed if we have a definition?
	// If we assume a definition has to be provided to spawn the item,
	// ID to look up the definition in your registry
	// If we were to provide both a definition and item, which should take priority?
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StackSize = 0;

	// CustomValues are a generic value that can be used for anything. To avoid needing a Runtime Instance
	// e.g. Durability, ChargeCount, Seed for RNG, Bitmask, progress, timer reference, variant id, or level
	// If you want or need more than one of these, you should probably use a runtime instance or modify for a 2nd CustomValue. 
	// Generally I imagine most of the time it should just be Durability, ChargeCount, or something like that.
	UPROPERTY(EditAnywhere)
	int32 CustomValue1 = 0;
	
	// UPROPERTY()
	// ERockItemStackCustomValueType CustomValue1Type = ERockItemStackCustomValueType::None;
	// Durability/Quality/Charges/Ammo/ VariantID?
	// Bitmask/

	UPROPERTY()
	int32 CustomValue2 = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<URockItemInstance> RuntimeInstance = nullptr;

	// LazyLoad
	FName GetItemId() const;
	const URockItemDefinition* GetDefinition() const;
	
	/** Resolve and cache the item definition */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated)
	TObjectPtr<URockItemDefinition> Definition = nullptr;
	
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	bool operator==(const FRockItemStack& Other) const;
	FString GetDebugString() const;
	bool IsValid() const;
	void SetStackSize(int32 InStackSize);
	void Reset();

	// Do we want to have a reference to the inventory component that owns this item stack?
	// It might be useful for 'ChangedEvents' ? 
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
