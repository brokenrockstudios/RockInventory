// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemStackHandle.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockItemStack.generated.h"

class URockItemInstance;
class URockItemDefinition;

/** Default maximum stack size when no definition is available */
constexpr int32 DEFAULT_MAX_STACK_SIZE = 1;

/**
 * Represents a stack of items in the inventory system.
 * This struct handles the core functionality of item stacking, including stack size,
 * custom values, and runtime instance management.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FRockItemStack();
	FRockItemStack(const FName& InItemId, int32 InStackSize = 1);
	FRockItemStack(URockItemDefinition* InDefinition, int32 InStackSize = 1);
	
	/** Unique identifier for the item type */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ItemId = NAME_None;
	
	/** Current number of items in the stack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 Generation = 0;
	
	// Handle for the item stack in the inventory
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FRockItemStackHandle Handle;


	/** Runtime instance of the item, if required by the definition */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<URockItemInstance> RuntimeInstance = nullptr;

	/** Cached item definition for quick access */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<URockItemDefinition> Definition = nullptr;

	// Core functionality
	FName GetItemId() const;
	const URockItemDefinition* GetDefinition() const;
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
	bool operator==(const FRockItemStack& Other) const;
	FString GetDebugString() const;
	bool IsValid() const;
	void SetStackSize(int32 InStackSize);
	void Reset();

	/** Returns true if the stack is empty (StackSize <= 0) */
	inline bool IsEmpty() const;
	/** Returns true if this stack can be combined with another stack */
	bool CanStackWith(const FRockItemStack& Other) const;
	/** Returns the maximum stack size allowed for this item */
	int32 GetMaxStackSize() const;
	/** Returns true if the stack is at its maximum size */
	bool IsFull() const;
	/** Safely modifies CustomValue1 with bounds checking */
	void SetCustomValue1(int32 NewValue);
	/** Safely modifies CustomValue2 with bounds checking */
	void SetCustomValue2(int32 NewValue);
private:
	/** Internal validation for stack size changes */
	void ValidateStackSize(int32 NewSize);
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
