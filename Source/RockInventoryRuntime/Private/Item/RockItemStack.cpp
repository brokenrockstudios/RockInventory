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

int32 FRockItemStack::GetMaxStackSize() const
{
	if (Definition)
	{
		return Definition->MaxStackSize;
	}
	// No definition, no max stack size because this is an Invalid stack
	return 0;
}

bool FRockItemStack::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;

	// Serialize basic properties
	Ar << Definition;
	Ar << StackSize;
	Ar << CustomValue1;
	Ar << CustomValue2;
	Ar << Generation;

	UObject* Instance = RuntimeInstance.Get();
	Map->SerializeObject(Ar, URockItemInstance::StaticClass(), Instance);
	if (Ar.IsLoading())
	{
		RuntimeInstance = Cast<URockItemInstance>(Instance);
	}

	// The handle to itself.
	if (!ItemHandle.NetSerialize(Ar, Map, bOutSuccess))
	{
		bOutSuccess = false;
		return false;
	}

	return true;
}

bool FRockItemStack::operator==(const FRockItemStack& Other) const
{
	return Definition == Other.Definition &&
		StackSize == Other.StackSize &&
		CustomValue1 == Other.CustomValue1 &&
		CustomValue2 == Other.CustomValue2 &&
		RuntimeInstance == Other.RuntimeInstance &&
		Generation == Other.Generation &&
		ItemHandle == Other.ItemHandle;
}

FString FRockItemStack::GetDebugString() const
{
	return FString::Printf(TEXT("ItemId=[%s], StackSize=[%d]"), *GetItemId().ToString(), StackSize);
}

bool FRockItemStack::IsValid() const
{
	return (StackSize > 0) && Definition;
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


void FRockItemStack::TransferOwnership(UObject* NewOuter, URockInventory* InOwningInventory)
{
	if (RuntimeInstance)
	{
		// Note: I think we need to ForceNetUpdate after a 'rename' (change of ownership)?
		// The caller should be responsible for calling ForceNetUpdate.
		RuntimeInstance->Rename(nullptr, NewOuter);
		// Has no owning inventory in a world item
		RuntimeInstance->SetOwningInventory(InOwningInventory);
	}
}

void FRockInventoryItemContainer::SetOwningInventory(URockInventory* InOwningInventory)
{
	OwnerInventory = InOwningInventory;
}

void FRockInventoryItemContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (!OwnerInventory)
	{
		return;
	}

	for (const int32 Index : AddedIndices)
	{
		if (AllSlots.IsValidIndex(Index))
		{
			OwnerInventory->BroadcastItemChanged(AllSlots[Index].ItemHandle);
		}
	}
}

void FRockInventoryItemContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	if (!OwnerInventory)
	{
		return;
	}

	for (const int32 Index : RemovedIndices)
	{
		if (AllSlots.IsValidIndex(Index))
		{
			OwnerInventory->BroadcastItemChanged(AllSlots[Index].ItemHandle);
		}
	}
}

void FRockInventoryItemContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (!OwnerInventory)
	{
		return;
	}

	for (const int32 Index : ChangedIndices)
	{
		if (AllSlots.IsValidIndex(Index))
		{
			OwnerInventory->BroadcastItemChanged(AllSlots[Index].ItemHandle);
		}
	}
}

// UE_OBJPTR_DEPRECATED(5.0, "Conversion to a mutable pointer is deprecated.  Please pass a TObjectPtr<T>& instead so that assignment can be tracked accurately.")
// explicit FORCEINLINE operator T*& () { return GetInternalRef(); }
