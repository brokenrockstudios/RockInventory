// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemStack.h"

#include "RockInventoryLogging.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"

FRockItemStack::FRockItemStack(URockItemDefinition* InDefinition, int32 InStackCount)
	: Definition(InDefinition),
	  StackCount(InStackCount)
{
	const bool bValidItem = ::IsValid(InDefinition) && InStackCount > 0;
	const bool bEmptyItem = !InDefinition && InStackCount == 0;
	ensureMsgf(bValidItem || bEmptyItem, TEXT("ItemStack has partially invalid config! Definition: %s, StackCount: %d"), *GetNameSafe(InDefinition), InStackCount);
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

int32 FRockItemStack::GetMaxStackCount() const
{
	if (Definition)
	{
		return Definition->MaxStackCount;
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
	RuntimeInstance = nullptr;
	StackCount = 0;
	CustomValue1 = 0;
	CustomValue2 = 0;
	bInitialized = 0;
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

int32 FRockItemStack::GetCustomValue1() const
{
	return CustomValue1;
}

int32 FRockItemStack::GetCustomValue2() const
{
	return CustomValue2;
}

TOptional<int32> FRockItemStack::GetCustomValueByTag(FGameplayTag CustomValueTag) const
{
	const URockItemDefinition* Def = GetDefinition();
	if (!Def) { return TOptional<int32>(); }
	if (CustomValueTag.MatchesTagExact(Def->CustomValue1Tag)) { return CustomValue1; }
	if (CustomValueTag.MatchesTagExact(Def->CustomValue2Tag)) { return CustomValue2; }
	return TOptional<int32>();
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

void FRockItemStack::CopyDataFrom(const FRockItemStack& InItemStack)
{
	ItemHandle = InItemStack.ItemHandle;
	Definition = InItemStack.Definition;
	StackCount = InItemStack.StackCount;
	CustomValue1 = InItemStack.CustomValue1;
	CustomValue2 = InItemStack.CustomValue2;
	RuntimeInstance = InItemStack.RuntimeInstance;
	Generation = InItemStack.Generation;
}

void FRockInventoryItemContainer::SetOwningInventory(URockInventory* InOwningInventory)
{
	OwnerInventory = InOwningInventory;
}

void FRockInventoryItemContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// Just forward to Change. Our Change logic handles "Invalid -> Valid" perfectly as an 'Added' event.
	PostReplicatedChange(AddedIndices, FinalSize);
}

void FRockInventoryItemContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	if (!OwnerInventory) { return; }

	// If the array actually shrinks, we MUST broadcast removal here because 
	// AllSlots[Index] will be invalid/gone in PostReplicated.
	for (const int32 Index : RemovedIndices)
	{
		if (PreviousItemHandles.IsValidIndex(Index) && PreviousItemHandles[Index].IsValid())
		{
			OwnerInventory->BroadcastItemChanged(PreviousItemHandles[Index], ERockItemChangeType::Removed);
			PreviousItemHandles[Index] = FRockItemStackHandle::Invalid();
		}
	}
}

void FRockInventoryItemContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (!OwnerInventory) { return; }

	if (PreviousItemHandles.Num() != AllSlots.Num())
	{
		PreviousItemHandles.SetNum(AllSlots.Num(), EAllowShrinking::No);
	}

	for (const int32 Index : ChangedIndices)
	{
		// I don't think this should ever be the case but just in case, we should be defensive about array bounds here
		if (!AllSlots.IsValidIndex(Index)) continue;

		const FRockItemStack& CurrentItem = AllSlots[Index];
		const FRockItemStackHandle PrevHandle = PreviousItemHandles.IsValidIndex(Index) ? PreviousItemHandles[Index] : FRockItemStackHandle::Invalid();

		const bool bIsCurrentlyValid = CurrentItem.IsValid();
		const bool bWasPreviouslyValid = PrevHandle.IsValid();

		// Item is invalid, There are 2 scenarios this is likely to be true 
		// 1. An item previously existed here and was removed. So we should broadcast a remove for the previous item handle
		// 2. This slot was previously empty and still is empty, so we shouldn't. This might happen on initial connect
		// Where maybe 5 valid items and 5 invalid items replicate. But in that case those 5 invalid items sholdn't have valid handles in our PreviousHandles
		// This is an initial sync edge case. Since it might only have fired a Change, and not necessarily an add
		if (!bIsCurrentlyValid)
		{
			if (bWasPreviouslyValid)
			{
				OwnerInventory->BroadcastItemChanged(PrevHandle, ERockItemChangeType::Removed);
			}
		}
		else // Item is valid
		{
			// Refresh the Cached Definition on the instance if it exists.
			if (CurrentItem.GetRuntimeInstance())
			{
				CurrentItem.RuntimeInstance->CachedDefinition = CurrentItem.GetDefinition();
			}

			if (!bWasPreviouslyValid)
			{
				// Scenario: Slot was empty, now has an item.
				// This falls under an Initial Sync edge case
				OwnerInventory->BroadcastItemChanged(CurrentItem.ItemHandle, ERockItemChangeType::Added);
			}
			else if (CurrentItem.ItemHandle.GetGeneration() == PrevHandle.GetGeneration())
			{
				// Scenario: Definitely the same item, just data/stack count changed.
				OwnerInventory->BroadcastItemChanged(CurrentItem.ItemHandle, ERockItemChangeType::Changed);
			}
			else
			{
				// Scenario: Explicit swap: generation mismatch. Valid Item
				OwnerInventory->BroadcastItemChanged(PrevHandle, ERockItemChangeType::Removed);
				OwnerInventory->BroadcastItemChanged(CurrentItem.ItemHandle, ERockItemChangeType::Added);
			}
		}

		// Sync the tracker for the next update
		PreviousItemHandles[Index] = CurrentItem.ItemHandle;
	}
}
