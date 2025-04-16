// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Item/RockItemStack.h"

#include "RockInventoryLogging.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"

FRockItemStack::FRockItemStack()
	: Definition(nullptr)
	  , StackSize(0)
	  , CustomValue1(0)
	  , CustomValue2(0)
{
	// Default empty itemslot
}

FRockItemStack::FRockItemStack(URockItemDefinition* InDefinition, int32 InStackSize)
	: StackSize(InStackSize)
	  , CustomValue1(0)
{
	if (InDefinition)
	{
		Definition = InDefinition;
	}
	else
	{
		UE_LOG(LogRockInventory, Error, TEXT("ItemStack %s has no ItemId set!"), *GetDebugString());
	}
}

FRockItemStack FRockItemStack::Invalid()
{
	return FRockItemStack();
}

FName FRockItemStack::GetItemId() const
{
	if (Definition)
	{
		return Definition->ItemId;
	}
	return NAME_None;
}

bool FRockItemStack::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	Ar << Definition;
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
	return FString::Printf(TEXT("ItemId=[%s], StackSize=[%d]"), *GetItemId().ToString(), StackSize);
}

bool FRockItemStack::IsValid() const
{
	// If we have at least 1 of an ItemID, we should be valid.
	// We should assume a StackSize of 0 should be invalid and emptied out.
	return (StackSize > 0) && Definition && bIsOccupied;
	// Many items won't ever have RuntimeInstances
	// A definition is 'nice to have' but not required for valid item, since we only lazy load the definition in many scenarios
}

bool FRockItemStack::IsOccupied() const
{
	return bIsOccupied;
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
	Definition = nullptr;
	StackSize = 0;
	CustomValue1 = 0;
	CustomValue2 = 0;
	RuntimeInstance = nullptr;
}

bool FRockItemStack::CanStackWith(const FRockItemStack& Other) const
{
	if (IsEmpty() || Other.IsEmpty())
	{
		return false;
	}
	if (Definition != Other.Definition)
	{
		return false;
	}
	if (CustomValue1 != Other.CustomValue1)
	{
		return false;
	}
	if (CustomValue2 != Other.CustomValue2)
	{
		return false;
	}
	return true;
}

bool FRockItemStack::IsEmpty() const
{
	return StackSize <= 0;
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


// UE_OBJPTR_DEPRECATED(5.0, "Conversion to a mutable pointer is deprecated.  Please pass a TObjectPtr<T>& instead so that assignment can be tracked accurately.")
// explicit FORCEINLINE operator T*& () { return GetInternalRef(); }
