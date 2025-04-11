// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Item/RockItemStack.h"

#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"

FRockItemStack::FRockItemStack()
	: StackSize(0)
	  , CustomValue1(0)
	  , RuntimeInstance(nullptr)
	  , Definition(nullptr)
{
	// Default empty itemslot
}

FRockItemStack::FRockItemStack(const FName& InItemId, int32 InStackSize)
	: ItemId(InItemId),
	  StackSize(InStackSize)
	  , CustomValue1(0)
	  , RuntimeInstance(nullptr)
	  , Definition(nullptr)
{
	if (!ItemId.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("ItemStack %s has no ItemId set!"), *GetDebugString());
	}
}

FRockItemStack::FRockItemStack(URockItemDefinition* InDefinition, int32 InStackSize)
	: StackSize(InStackSize)
	  , CustomValue1(0)
	  , RuntimeInstance(nullptr)
	  , Definition(InDefinition)
{
	if (InDefinition)
	{
		ItemId = InDefinition->ItemId;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ItemStack %s has no ItemId set!"), *GetDebugString());
	}
}

FName FRockItemStack::GetItemId() const
{
	return ItemId;
}

const URockItemDefinition* FRockItemStack::GetDefinition() const
{
	return Definition;
}

bool FRockItemStack::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	Ar << ItemId;
	Ar << StackSize;
	Ar << CustomValue1;

	UObject* NewObj = RuntimeInstance.Get();
	Map->SerializeObject(Ar, URockItemInstance::StaticClass(), NewObj);

	return true;
}

bool FRockItemStack::operator==(const FRockItemStack& Other) const
{
	return //ItemId == Other.ItemId &&
		StackSize == Other.StackSize &&
		CustomValue1 == Other.CustomValue1 &&
		RuntimeInstance == Other.RuntimeInstance;
}

FString FRockItemStack::GetDebugString() const
{
	return FString::Printf(TEXT("ItemId=[%s], StackSize=[%d]"), *ItemId.ToString(), StackSize);
}

bool FRockItemStack::IsValid() const
{
	// If we have at least 1 of an ItemID, we should be valid.
	// We should assume a StackSize of 0 should be invalid and emptied out.
	return (StackSize > 0) && ItemId.IsValid();
	
	// Many items won't ever have RuntimeInstances
	// A definition is 'nice to have' but not required for valid item, since we only lazy load the definition in many scenarios
}

void FRockItemStack::SetStackSize(int32 InStackSize)
{
	StackSize = InStackSize;
	if (StackSize <= 0)
	{
		Reset();
	}
}

void FRockItemStack::Reset()
{
	ItemId = NAME_None;
	StackSize = 0;
	CustomValue1 = 0;
	CustomValue2 = 0;
	RuntimeInstance = nullptr;
	Definition = nullptr;
}

bool FRockItemStack::IsEmpty() const
{
	return StackSize <= 0;
}

bool FRockItemStack::CanStackWith(const FRockItemStack& Other) const
{
	if (!IsValid() || !Other.IsValid())
	{
		return false;
	}
	if (ItemId != Other.ItemId)
	{
		return false;
	}
	// TODO: We might be able to support this later. But for now, disallow stacking when we have RuntimeInstances
	if (RuntimeInstance || Other.RuntimeInstance)
	{
		return false;
	}
	// TODO: We might be able to support this later. But for now, disallow stacking when we have different CustomValues
	if (CustomValue1 != Other.CustomValue1 || CustomValue2 != Other.CustomValue2)
	{
		return false;
	}

	// Check if we have room to stack
	const int32 MaxStackSize = GetMaxStackSize();
	if (MaxStackSize <= 0)
	{
		return false;
	}
	return (StackSize + Other.StackSize) <= MaxStackSize;
}

int32 FRockItemStack::GetMaxStackSize() const
{
	if (const URockItemDefinition* Def = GetDefinition())
	{
		return Def->MaxStackSize;
	}
	return DEFAULT_MAX_STACK_SIZE;
}

bool FRockItemStack::IsFull() const
{
	return StackSize >= GetMaxStackSize();
}

void FRockItemStack::SetCustomValue1(int32 NewValue)
{
	CustomValue1 = NewValue;
	// Add any additional validation or side effects here
}

void FRockItemStack::SetCustomValue2(int32 NewValue)
{
	CustomValue2 = NewValue;
	// Add any additional validation or side effects here
}

void FRockItemStack::ValidateStackSize(int32 NewSize)
{
	if (NewSize < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to set negative stack size for item %s"), *ItemId.ToString());
		NewSize = 0;
	}

	const int32 MaxStackSize = GetMaxStackSize();
	if (MaxStackSize > 0 && NewSize > MaxStackSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to exceed max stack size for item %s"), *ItemId.ToString());
		NewSize = MaxStackSize;
	}

	StackSize = NewSize;
	if (StackSize <= 0)
	{
		Reset();
	}
}

// UE_OBJPTR_DEPRECATED(5.0, "Conversion to a mutable pointer is deprecated.  Please pass a TObjectPtr<T>& instead so that assignment can be tracked accurately.")
// explicit FORCEINLINE operator T*& () { return GetInternalRef(); }
