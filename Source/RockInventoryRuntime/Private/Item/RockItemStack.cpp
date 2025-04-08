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

void FRockItemStack::SetDefinition(URockItemDefinition* InDefinition)
{
	Definition = InDefinition;
}

const URockItemDefinition* FRockItemStack::GetDefinition() const
{
	if (!Definition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemStack %s has no definition set!"), *GetDebugString());
		//ItemDefinition = UMyItemRegistry::Get()->FindDefinition(ItemID);
	}
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





// UE_OBJPTR_DEPRECATED(5.0, "Conversion to a mutable pointer is deprecated.  Please pass a TObjectPtr<T>& instead so that assignment can be tracked accurately.")
// explicit FORCEINLINE operator T*& () { return GetInternalRef(); }
