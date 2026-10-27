// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemStack.h"

#include "RockInventoryLogging.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"

FRockItemStack::FRockItemStack(URockItemDefinition* InDefinition, int32 InStackCount)
	: StackCount(InStackCount)
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

const FRockItemStack& FRockItemStack::Invalid()
{
	static FRockItemStack InvalidRockItemStack(nullptr, 0);
	return InvalidRockItemStack;
}

FName FRockItemStack::GetItemId() const
{
	if (Definition)
	{
		return Definition->ItemId;
	}
	return NAME_None;
}

URockItemDefinition* FRockItemStack::GetDefinition() const
{
	return Definition;
}

int32 FRockItemStack::GetStackCount() const
{
	return StackCount;
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

URockItemInstance* FRockItemStack::GetRuntimeInstance() const
{
	return RuntimeInstance;
}

FString FRockItemStack::GetDebugString() const
{
	return FString::Printf(TEXT("ItemId=[%s], StackSize=[%d]"), *GetItemId().ToString(), StackCount);
}

bool FRockItemStack::IsValid() const
{
	return (StackCount > 0) && Definition;
}

void FRockItemStack::Reset()
{
	Definition = nullptr;
	StackCount = 0;
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

	// Check definition's stackability rules?
	//if (Definition)
	{
	}

	// Check runtime instance's fragments?

	return true;
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

bool FRockItemStack::operator==(const FRockItemStack& Other) const
{
	return Definition == Other.Definition &&
		StackCount == Other.StackCount &&
		RuntimeInstance == Other.RuntimeInstance &&
		CustomValue1 == Other.CustomValue1 &&
		CustomValue2 == Other.CustomValue2 &&
		Generation == Other.Generation &&
		ItemHandle == Other.ItemHandle;
}

bool FRockItemStack::operator!=(const FRockItemStack& Other) const
{
	return !(*this == Other);
}

bool FRockItemStack::IsEmpty() const
{
	return StackCount <= 0;
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
			// TODO: Replace with a OnItemRemoved event instead?
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
			// TODO: Replace with a OnItemRemoved event instead
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
