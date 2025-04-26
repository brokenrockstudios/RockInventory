// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStackHandle.h"
#include "Library/RockInventoryHelpers.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockItemStack.generated.h"

class URockItemInstance;
class URockItemDefinition;

/**
 * Represents a stack of items in the inventory system.
 * This struct handles the core functionality of item stacking, including stack size,
 * custom values, and runtime instance management.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	// A reference to it's handle, for fast access to the containing array
	UPROPERTY(VisibleAnywhere)
	FRockItemStackHandle ItemHandle;
	
private:
	// Generally this item stack is read only.
	// Because to modify it, requires special attention to marking the containing array dirty for replication.
	// Which should be exclusively handled by this plugin and minimal amount of other classes
	friend class URockInventory;
	friend class URockItemStackLibrary;
	friend class URockInventoryLibrary;
	friend class ARockInventoryWorldItem; // I don't like this being here, redesign to not need

	/** Unique identifier for the item */
	UPROPERTY(EditAnywhere)
	TObjectPtr<URockItemDefinition> Definition = nullptr;

	/** Runtime instance of the item, if required by the definition, likely often null */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URockItemInstance> RuntimeInstance = nullptr;

	/** Current number of items in the stack */
	UPROPERTY(EditAnywhere)
	int32 StackSize = 0;

	/** 
	 * Generic value that can be used for various purposes (durability, charges, etc.)
	 * The meaning of this value is determined by the item's definition
	 */
	UPROPERTY(EditAnywhere)
	int32 CustomValue1 = 0;
	/** Additional generic value for extended functionality */
	UPROPERTY()
	int32 CustomValue2 = 0;
	/** This is used to detect stale item handles that may have pointed to previous items */
	UPROPERTY(VisibleAnywhere)
	uint8 Generation = 0;
	
public:
	FRockItemStack() = default;
	FRockItemStack(URockItemDefinition* InDefinition, int32 InStackSize = 1);
	// create invalid stack
	static FRockItemStack Invalid();


	// Core functionality
	FName GetItemId() const;
	URockItemDefinition* GetDefinition() const { return Definition; }
	int32 GetStackSize() const { return StackSize; }
	int32 GetMaxStackSize() const;
	URockItemInstance* GetRuntimeInstance() const { return RuntimeInstance; }

	/** Gets the item definition for this item stack */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	bool operator==(const FRockItemStack& Other) const;
	FString GetDebugString() const;
	bool IsValid() const;
	void Reset();

	bool CanStackWith(const FRockItemStack& Other) const;
	/** Returns true if the stack is empty (StackSize <= 0) */
	inline bool IsEmpty() const;
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

USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryItemContainer : public FFastArraySerializer
{
	GENERATED_BODY()

private:
	// Pointer to the owning inventory
	UPROPERTY(NotReplicated)
	TObjectPtr<URockInventory> OwnerInventory = nullptr;

public:
	// Replicated list of item stacks
	UPROPERTY()
	TArray<FRockItemStack> AllSlots;

	// Set the owner inventory
	void SetOwningInventory(URockInventory* InOwningInventory);

	// Override PostReplicatedChange to notify the owner about changes
	//~ FFastArraySerializer contract
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~ End of FFastArraySerializer contract
	
	ROCKINVENTORY_FastArraySerializer_TArray_ACCESSORS(AllSlots);
	
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
