// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Client/RockInventoryClientModel.h"

#include "Engine/World.h"
#include "Inventory/Events/RockInventoryChangeBatch.h"
#include "Inventory/RockInventory.h"

namespace
{
	/** Two sections that lay out and filter the same way. The filter is a function of the section's tags and size, so those are what is compared. */
	bool SameSection(const FRockInventorySectionInfo& A, const FRockInventorySectionInfo& B)
	{
		return A.GetSectionTag() == B.GetSectionTag()
			&& A.GetSectionIndex() == B.GetSectionIndex()
			&& A.GetFirstSlotIndex() == B.GetFirstSlotIndex()
			&& A.GetNumSlots() == B.GetNumSlots()
			&& A.GetColumns() == B.GetColumns();
	}

	const FRockItemStack* FindStack(const FRockInventoryData& Data, int32 Index)
	{
		return Data.Stacks.IsValidIndex(Index) && Data.Stacks[Index].IsValid() ? &Data.Stacks[Index] : nullptr;
	}
}

FRockInventoryPresentationDiff FRockInventoryPresentationDiff::Between(const FRockInventoryData& Old, ERockInventorySyncState OldState, const FRockInventoryData& New, ERockInventorySyncState NewState)
{
	FRockInventoryPresentationDiff Diff;
	Diff.PreviousSyncState = OldState;
	Diff.bSyncStateChanged = OldState != NewState;

	Diff.bLayoutChanged = Old.Sections.Num() != New.Sections.Num() || Old.Slots.Num() != New.Slots.Num();
	for (int32 SectionIndex = 0; !Diff.bLayoutChanged && SectionIndex < New.Sections.Num(); ++SectionIndex)
	{
		Diff.bLayoutChanged = !SameSection(Old.Sections[SectionIndex], New.Sections[SectionIndex]);
	}

	const int32 NumSlots = FMath::Max(Old.Slots.Num(), New.Slots.Num());
	for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
	{
		const bool bInOld = Old.Slots.IsValidIndex(SlotIndex);
		const bool bInNew = New.Slots.IsValidIndex(SlotIndex);
		if (bInOld != bInNew || (bInOld && Old.Slots[SlotIndex] != New.Slots[SlotIndex]))
		{
			Diff.ChangedSlots.Add(FRockInventorySlotHandle(SlotIndex));
		}
	}

	const int32 NumStacks = FMath::Max(Old.Stacks.Num(), New.Stacks.Num());
	for (int32 StackIndex = 0; StackIndex < NumStacks; ++StackIndex)
	{
		const FRockItemStack* OldStack = FindStack(Old, StackIndex);
		const FRockItemStack* NewStack = FindStack(New, StackIndex);
		if (!OldStack && !NewStack)
		{
			continue;
		}
		if (!OldStack)
		{
			Diff.StacksCreated.Add(NewStack->ItemHandle);
		}
		else if (!NewStack)
		{
			Diff.StacksRemoved.Add(OldStack->ItemHandle);
		}
		else if (OldStack->ItemHandle != NewStack->ItemHandle)
		{
			// The index was reused by another item (a new generation): that is a removal and a creation, not a modification.
			Diff.StacksRemoved.Add(OldStack->ItemHandle);
			Diff.StacksCreated.Add(NewStack->ItemHandle);
		}
		else if (*OldStack != *NewStack)
		{
			Diff.StacksModified.Add(NewStack->ItemHandle);
		}
	}
	return Diff;
}

void URockInventoryClientModel::Bind(URockInventory* InInventory)
{
	Unbind();
	Inventory = InInventory;
	if (!InInventory)
	{
		return;
	}
	InInventory->OnChangeBatch.AddDynamic(this, &ThisClass::HandleChangeBatch);
	SyncStateHandle = InInventory->OnSyncStateChanged.AddUObject(this, &ThisClass::HandleSyncState);

	// Take the current state silently: a listener that attaches after Bind reads it with the getters.
	Data = FRockInventoryData::FromInventory(InInventory);
	Revision = static_cast<int32>(InInventory->GetRevision());
	SyncState = InInventory->GetSyncState();
}

void URockInventoryClientModel::Unbind()
{
	if (URockInventory* Bound = Inventory.Get())
	{
		Bound->OnChangeBatch.RemoveDynamic(this, &ThisClass::HandleChangeBatch);
		Bound->OnSyncStateChanged.Remove(SyncStateHandle);
	}
	SyncStateHandle.Reset();
	Inventory.Reset();
}

FRockInventoryPresentationDiff URockInventoryClientModel::Rebuild()
{
	const URockInventory* Source = Inventory.Get();
	if (!Source)
	{
		return FRockInventoryPresentationDiff();
	}
	return SetState(FRockInventoryData::FromInventory(Source), static_cast<int32>(Source->GetRevision()), Source->GetSyncState());
}

FRockInventoryPresentationDiff URockInventoryClientModel::SetState(FRockInventoryData&& NewData, int32 NewRevision, ERockInventorySyncState NewSyncState)
{
	FRockInventoryPresentationDiff Diff = FRockInventoryPresentationDiff::Between(Data, SyncState, NewData, NewSyncState);
	Data = MoveTemp(NewData);
	Revision = NewRevision;
	SyncState = NewSyncState;
	if (!Diff.IsEmpty())
	{
		OnChanged.Broadcast(*this, Diff);
	}
	return Diff;
}

void URockInventoryClientModel::HandleChangeBatch(const FRockInventoryChangeBatch& Batch)
{
	Rebuild();
}

void URockInventoryClientModel::HandleSyncState(URockInventory& Changed, ERockInventorySyncState NewState)
{
	// The data may have arrived before the grant: Rebuild takes both together.
	Rebuild();
}

FRockInventorySlotEntry URockInventoryClientModel::GetSlotByHandle(const FRockInventorySlotHandle& SlotHandle) const
{
	const FRockInventorySlotEntry* Slot = Data.GetSlot(SlotHandle);
	return Slot ? *Slot : FRockInventorySlotEntry::Invalid();
}

const FRockInventorySlotEntry& URockInventoryClientModel::GetSlotByAbsoluteIndex(int32 AbsoluteIndex) const
{
	return Data.Slots.IsValidIndex(AbsoluteIndex) ? Data.Slots[AbsoluteIndex] : FRockInventorySlotEntry::Invalid();
}

FRockInventorySlotEntry URockInventoryClientModel::GetSlotByItemHandle(const FRockItemStackHandle& ItemHandle) const
{
	if (ItemHandle.IsValid())
	{
		for (const FRockInventorySlotEntry& Slot : Data.Slots)
		{
			if (Slot.ItemHandle == ItemHandle)
			{
				return Slot;
			}
		}
	}
	return FRockInventorySlotEntry::Invalid();
}

FRockItemStack URockInventoryClientModel::GetItemByHandle(const FRockItemStackHandle& ItemHandle) const
{
	const FRockItemStack* Stack = Data.GetStack(ItemHandle);
	return Stack ? *Stack : FRockItemStack::Invalid();
}

FRockItemStack URockInventoryClientModel::GetItemBySlotHandle(const FRockInventorySlotHandle& SlotHandle) const
{
	const FRockItemStack* Stack = Data.GetSlotStack(SlotHandle);
	return Stack ? *Stack : FRockItemStack::Invalid();
}

int32 URockInventoryClientModel::GetSectionIndex(const FGameplayTag& SectionTag) const
{
	if (!SectionTag.IsValid())
	{
		return INDEX_NONE;
	}
	for (int32 Index = 0; Index < Data.Sections.Num(); ++Index)
	{
		if (Data.Sections[Index].GetSectionTag() == SectionTag)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

const FRockInventorySectionInfo& URockInventoryClientModel::GetSectionInfo(const FGameplayTag& SectionTag) const
{
	const int32 Index = GetSectionIndex(SectionTag);
	return Index != INDEX_NONE ? Data.Sections[Index] : FRockInventorySectionInfo::Invalid();
}

const FRockInventorySectionInfo& URockInventoryClientModel::GetSectionInfoBySlotHandle(const FRockInventorySlotHandle& SlotHandle) const
{
	const FRockInventorySectionInfo* Section = Data.FindSection(SlotHandle);
	return Section ? *Section : FRockInventorySectionInfo::Invalid();
}

URockInventoryClientModel* URockInventoryClientModelSubsystem::GetModel(URockInventory* Inventory)
{
	const UWorld* World = Inventory ? Inventory->GetWorld() : nullptr;
	URockInventoryClientModelSubsystem* Subsystem = World ? World->GetSubsystem<URockInventoryClientModelSubsystem>() : nullptr;
	return Subsystem ? Subsystem->FindOrCreateModel(Inventory) : nullptr;
}

URockInventoryClientModel* URockInventoryClientModelSubsystem::FindOrCreateModel(URockInventory* Inventory)
{
	if (!Inventory)
	{
		return nullptr;
	}
	// Models of destroyed inventories: drop them here, the only place that grows the map.
	for (auto It = Models.CreateIterator(); It; ++It)
	{
		if (!It.Key())
		{
			It.Value()->Unbind();
			It.RemoveCurrent();
		}
	}
	TObjectPtr<URockInventoryClientModel>& Model = Models.FindOrAdd(Inventory);
	if (!Model)
	{
		Model = NewObject<URockInventoryClientModel>(this);
		Model->Bind(Inventory);
	}
	return Model;
}
