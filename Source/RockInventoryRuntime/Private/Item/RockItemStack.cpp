// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemStack.h"

#include "RockInventoryLogging.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"

FRockItemStack::FRockItemStack(URockItemDefinition* InDefinition, int32 InStackSize)
	: StackSize(InStackSize)
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

	// Serialize basic properties
	Ar << Definition;
	Ar << StackSize;
	Ar << CustomValue1;
	Ar << CustomValue2;

	UObject* Instance = RuntimeInstance.Get();
	Map->SerializeObject(Ar, URockItemInstance::StaticClass(), Instance);
	if (Ar.IsLoading())
	{
		RuntimeInstance = Cast<URockItemInstance>(Instance);
	}
	return true;
}

bool FRockItemStack::operator==(const FRockItemStack& Other) const
{
	return Definition == Other.Definition &&
		StackSize == Other.StackSize &&
		CustomValue1 == Other.CustomValue1 &&
		CustomValue2 == Other.CustomValue2 &&
		RuntimeInstance == Other.RuntimeInstance;
}

FString FRockItemStack::GetDebugString() const
{
	return FString::Printf(TEXT("ItemId=[%s], StackSize=[%d]"), *GetItemId().ToString(), StackSize);
}

bool FRockItemStack::IsValid() const
{
	return (StackSize > 0) && Definition;
}

bool FRockItemStack::IsOccupied() const
{
	return bIsOccupied;
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

// UE_OBJPTR_DEPRECATED(5.0, "Conversion to a mutable pointer is deprecated.  Please pass a TObjectPtr<T>& instead so that assignment can be tracked accurately.")
// explicit FORCEINLINE operator T*& () { return GetInternalRef(); }
