// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Item/RockItemStack.h"
#include "Item/RockItemInstance.h"

void FRockItemStack::SetDefinition(URockItemDefinition* InDefinition)
{
	CachedDefinition = InDefinition;
}

const URockItemDefinition* FRockItemStack::GetDefinition() const
{
	if (!CachedDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemStack %s has no definition set!"), *GetDebugString());
		//ItemDefinition = UMyItemRegistry::Get()->FindDefinition(ItemID);
	}
	return CachedDefinition;
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

FString FRockItemStack::GetDebugString() const
{
	return FString::Printf(TEXT("ItemId=[%s], StackSize=[%d]"), *ItemId.ToString(), StackSize);
}


// UE_OBJPTR_DEPRECATED(5.0, "Conversion to a mutable pointer is deprecated.  Please pass a TObjectPtr<T>& instead so that assignment can be tracked accurately.")
// explicit FORCEINLINE operator T*& () { return GetInternalRef(); }
