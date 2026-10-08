// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventoryData.h"

#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockInventoryLibrary.h"
#include "Library/RockItemStackLibrary.h"

namespace
{
	enum class EMoveKind : uint8
	{
		NoOp,
		Rotate,
		PlaceFull,
		PlaceSplit,
		Merge,
	};

	struct FMovePlan
	{
		EMoveKind Kind = EMoveKind::NoOp;
		/** Units that leave the source (the whole stack for PlaceFull, the part for PlaceSplit, what fits for Merge). */
		int32 Amount = 0;
	};

	bool Fits(const TArray<bool>& Occupancy, const FRockInventorySectionInfo& Section,
		const FRockInventorySlotHandle& Slot, const FRockItemStack& Stack, ERockItemOrientation Orientation)
	{
		const int32 LocalIndex = Section.GetLocalIndex(Slot.GetAbsoluteIndex());
		const FVector2D Size = FVector2D(URockItemStackLibrary::GetItemSizeForOrientation(Stack, Orientation));
		return URockInventoryLibrary::CanItemFitInGridPosition(Occupancy, Section, LocalIndex % Section.GetColumns(), LocalIndex / Section.GetColumns(), Size);
	}

	ERockMoveRefusal PlanMove(
		const FRockInventoryData& Source, const FRockInventorySlotHandle& SourceSlot,
		const FRockInventoryData& Target, const FRockInventorySlotHandle& TargetSlot,
		const FRockMoveItemParams& Params, FMovePlan& OutPlan)
	{
		OutPlan = FMovePlan();
		const bool bSameData = &Source == &Target;

		if (bSameData && SourceSlot == TargetSlot)
		{
			const FRockInventorySlotEntry* Slot = Source.GetSlot(SourceSlot);
			if (!Slot || !Slot->ItemHandle.IsValid() || Slot->Orientation == Params.DesiredOrientation)
			{
				return ERockMoveRefusal::None;
			}
			const FRockItemStack* Stack = Source.GetStack(Slot->ItemHandle);
			const FRockInventorySectionInfo* Section = Source.FindSection(SourceSlot);
			if (!Stack || !Section)
			{
				return ERockMoveRefusal::None;
			}
			if (!Fits(Source.BuildOccupancy(Slot->ItemHandle), *Section, SourceSlot, *Stack, Params.DesiredOrientation))
			{
				return ERockMoveRefusal::NoRoomToRotate;
			}
			OutPlan.Kind = EMoveKind::Rotate;
			return ERockMoveRefusal::None;
		}

		const FRockInventorySlotEntry* Slot = Source.GetSlot(SourceSlot);
		if (!Slot)
		{
			return ERockMoveRefusal::InvalidSourceSlot;
		}
		const FRockItemStack* SourceStack = Source.GetStack(Slot->ItemHandle);
		if (!SourceStack)
		{
			return ERockMoveRefusal::EmptySource;
		}
		const FRockInventorySlotEntry* TargetEntry = Target.GetSlot(TargetSlot);
		const FRockInventorySectionInfo* TargetSection = Target.FindSection(TargetSlot);
		if (!TargetEntry || !TargetSection)
		{
			return ERockMoveRefusal::InvalidTargetSlot;
		}
		if (!URockInventoryLibrary::CanItemBePlacedInSection(*SourceStack, *TargetSection))
		{
			return ERockMoveRefusal::SectionRejectsItem;
		}
		const int32 MoveAmount = URockItemStackLibrary::CalculateMoveAmount(*SourceStack, Params.MoveMode, Params.MoveCount);
		if (MoveAmount <= 0)
		{
			return ERockMoveRefusal::InvalidAmount;
		}

		// Inside one inventory the moving stack's own cells are free, even for a split that leaves part of it behind (as it always was).
		// Across inventories a handle in the target can equal the source handle, so nothing is ignored there.
		const TArray<bool> Occupancy = Target.BuildOccupancy(bSameData ? Slot->ItemHandle : FRockItemStackHandle::Invalid());
		if (Fits(Occupancy, *TargetSection, TargetSlot, *SourceStack, Params.DesiredOrientation))
		{
			if (MoveAmount == SourceStack->GetStackCount())
			{
				OutPlan.Kind = EMoveKind::PlaceFull;
			}
			else
			{
				if (!SourceStack->GetDefinition()->RuntimeInstanceClass.IsNull())
				{
					return ERockMoveRefusal::PartialMoveOfInstancedItem;
				}
				OutPlan.Kind = EMoveKind::PlaceSplit;
			}
			OutPlan.Amount = MoveAmount;
			return ERockMoveRefusal::None;
		}

		const FRockItemStack* TargetStack = Target.GetStack(TargetEntry->ItemHandle);
		if (!TargetStack || !TargetStack->CanStackWith(*SourceStack) || TargetStack->GetStackCount() >= TargetStack->GetMaxStackCount())
		{
			return ERockMoveRefusal::NoRoom;
		}
		const int32 Space = TargetStack->GetMaxStackCount() - TargetStack->GetStackCount();
		OutPlan.Amount = FMath::Min3(Space, SourceStack->GetStackCount(), MoveAmount);
		if (OutPlan.Amount <= 0)
		{
			return ERockMoveRefusal::NothingToMerge;
		}
		OutPlan.Kind = EMoveKind::Merge;
		return ERockMoveRefusal::None;
	}

	void AddSlotChange(FRockInventoryChangeSet& Set, ERockInventorySide Side, const FRockInventorySlotEntry& Before, const FRockInventorySlotEntry& After)
	{
		FRockInventoryChange& Change = Set.Changes.AddDefaulted_GetRef();
		Change.Type = ERockDataChangeType::Slot;
		Change.Side = Side;
		Change.Slot = After.SlotHandle;
		Change.SlotBefore = Before;
		Change.SlotAfter = After;
	}

	void AddStackChange(FRockInventoryChangeSet& Set, ERockDataChangeType Type, ERockInventorySide Side,
		const FRockItemStackHandle& Handle, const FRockItemStack& Before, const FRockItemStack& After)
	{
		FRockInventoryChange& Change = Set.Changes.AddDefaulted_GetRef();
		Change.Type = Type;
		Change.Side = Side;
		Change.Stack = Handle;
		Change.StackBefore = Before;
		Change.StackAfter = After;
	}
}

const FRockInventoryChange* FRockInventoryChangeSet::FindSlotChange(ERockInventorySide Side, FRockInventorySlotHandle Slot) const
{
	return Changes.FindByPredicate([&](const FRockInventoryChange& Change)
	{
		return Change.Type == ERockDataChangeType::Slot && Change.Side == Side && Change.Slot == Slot;
	});
}

const FRockInventoryChange* FRockInventoryChangeSet::FindStackChange(ERockInventorySide Side, FRockItemStackHandle Stack) const
{
	return Changes.FindByPredicate([&](const FRockInventoryChange& Change)
	{
		return Change.Type != ERockDataChangeType::Slot && Change.Side == Side && Change.Stack == Stack;
	});
}

FRockInventoryData FRockInventoryData::FromInventory(const URockInventory* Inventory)
{
	FRockInventoryData Data;
	if (!Inventory)
	{
		return Data;
	}
	Data.Sections = Inventory->SlotSections;
	Data.Slots = Inventory->SlotData.AllSlots;
	Data.Stacks = Inventory->ItemData.AllSlots;
	return Data;
}

void FRockInventoryData::Init(const TArray<FRockInventorySectionInfo>& InSections)
{
	Sections.Reset();
	Slots.Reset();
	Stacks.Reset();

	int32 TotalSlots = 0;
	for (int32 SectionIndex = 0; SectionIndex < InSections.Num(); ++SectionIndex)
	{
		FRockInventorySectionInfo Section = InSections[SectionIndex];
		Section.Initialize(TotalSlots, SectionIndex);
		TotalSlots += Section.GetNumSlots();
		Sections.Add(Section);
	}
	Slots.SetNum(TotalSlots);
	for (int32 SlotIndex = 0; SlotIndex < TotalSlots; ++SlotIndex)
	{
		Slots[SlotIndex].SlotHandle = FRockInventorySlotHandle(SlotIndex);
	}
}

const FRockInventorySlotEntry* FRockInventoryData::GetSlot(const FRockInventorySlotHandle& SlotHandle) const
{
	const int32 Index = SlotHandle.GetAbsoluteIndex();
	if (!Slots.IsValidIndex(Index) || !Slots[Index].IsValid())
	{
		return nullptr;
	}
	return &Slots[Index];
}

const FRockItemStack* FRockInventoryData::GetStack(const FRockItemStackHandle& StackHandle) const
{
	if (!StackHandle.IsValid() || !Stacks.IsValidIndex(StackHandle.GetIndex()))
	{
		return nullptr;
	}
	const FRockItemStack& Stack = Stacks[StackHandle.GetIndex()];
	return Stack.GetGeneration() == StackHandle.GetGeneration() && Stack.IsValid() ? &Stack : nullptr;
}

const FRockItemStack* FRockInventoryData::GetSlotStack(const FRockInventorySlotHandle& SlotHandle) const
{
	const FRockInventorySlotEntry* Slot = GetSlot(SlotHandle);
	return Slot ? GetStack(Slot->ItemHandle) : nullptr;
}

const FRockInventorySectionInfo* FRockInventoryData::FindSection(const FRockInventorySlotHandle& SlotHandle) const
{
	return Sections.FindByPredicate([&](const FRockInventorySectionInfo& Section) { return Section.ContainsSlotHandle(SlotHandle); });
}

FRockItemStackHandle FRockInventoryData::AllocateStack(const FRockItemStack& Copy)
{
	// The lowest free index, derived from the data alone: the server's inventory does the same, so a client that predicts a
	// move on a copy of the replicated data picks the handle the server will.
	int32 Index = Stacks.IndexOfByPredicate([](const FRockItemStack& Stack) { return !Stack.IsValid(); });
	if (Index == INDEX_NONE)
	{
		Index = Stacks.AddDefaulted();
		Stacks[Index].Generation = 0;
	}
	FRockItemStack& Stack = Stacks[Index];
	Stack.Definition = Copy.Definition;
	Stack.RuntimeInstance = Copy.RuntimeInstance;
	Stack.StackCount = Copy.StackCount;
	Stack.CustomValue1 = Copy.CustomValue1;
	Stack.CustomValue2 = Copy.CustomValue2;
	Stack.bInitialized = Copy.bInitialized;
	Stack.ItemHandle = FRockItemStackHandle::Create(Index, Stack.Generation);
	return Stack.ItemHandle;
}

void FRockInventoryData::FreeStack(const FRockItemStackHandle& StackHandle)
{
	const int32 Index = StackHandle.GetIndex();
	FRockItemStack& Stack = Stacks[Index];
	Stack.Generation = static_cast<uint16>(FRockItemStackHandle::NextGeneration(Stack.Generation));
	Stack.ItemHandle = FRockItemStackHandle::Create(Index, Stack.Generation);
	Stack.Reset();
}

FRockItemStackHandle FRockInventoryData::PlaceStack(const FRockItemStack& Stack, const FRockInventorySlotHandle& SlotHandle, ERockItemOrientation Orientation)
{
	if (!ensureMsgf(GetSlot(SlotHandle), TEXT("PlaceStack - invalid slot %s"), *SlotHandle.ToString()))
	{
		return FRockItemStackHandle::Invalid();
	}
	const FRockItemStackHandle Handle = AllocateStack(Stack);
	FRockInventorySlotEntry& Slot = Slots[SlotHandle.GetAbsoluteIndex()];
	Slot.ItemHandle = Handle;
	Slot.Orientation = Orientation;
	return Handle;
}

TArray<bool> FRockInventoryData::BuildOccupancy(const FRockItemStackHandle& IgnoreHandle) const
{
	TArray<bool> Occupancy;
	FillOccupancy(Sections, Slots, [this](const FRockItemStackHandle& Handle) { return GetStack(Handle); }, IgnoreHandle, Occupancy);
	return Occupancy;
}

void FRockInventoryData::FillOccupancy(
	const TArray<FRockInventorySectionInfo>& InSections, const TArray<FRockInventorySlotEntry>& InSlots,
	const TFunctionRef<const FRockItemStack*(const FRockItemStackHandle&)>& GetStack,
	const FRockItemStackHandle& IgnoreHandle, TArray<bool>& OutOccupancy)
{
	OutOccupancy.Init(false, InSlots.Num());
	for (const FRockInventorySectionInfo& Section : InSections)
	{
		const int32 Offset = Section.GetFirstSlotIndex();
		for (int32 LocalIndex = 0; LocalIndex < Section.GetNumSlots(); ++LocalIndex)
		{
			const FRockInventorySlotEntry& Slot = InSlots[Offset + LocalIndex];
			if (Slot.ItemHandle == IgnoreHandle)
			{
				continue;
			}
			const FRockItemStack* Stack = GetStack(Slot.ItemHandle);
			if (!Stack)
			{
				continue;
			}
			MarkFootprint(OutOccupancy, Section, LocalIndex % Section.GetColumns(), LocalIndex / Section.GetColumns(),
				URockItemStackLibrary::GetItemSizeForOrientation(*Stack, Slot.Orientation));
		}
	}
}

void FRockInventoryData::MarkFootprint(TArray<bool>& InOutOccupancy, const FRockInventorySectionInfo& Section, int32 Column, int32 Row, FIntPoint Size)
{
	if (Section.GetSlotSizePolicy() == ERockItemSizePolicy::IgnoreSize)
	{
		Size = FIntPoint(1, 1);
	}
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			const int32 GridIndex = Section.GetFirstSlotIndex() + (Row + Y) * Section.GetColumns() + (Column + X);
			if (InOutOccupancy.IsValidIndex(GridIndex))
			{
				InOutOccupancy[GridIndex] = true;
			}
		}
	}
}

ERockAddRefusal FRockInventoryData::PlanAdd(
	const FRockInventorySlotHandle& Slot, const FRockItemStack& Stack, ERockItemOrientation Orientation, bool& bOutMerge, int32& OutAmount) const
{
	bOutMerge = false;
	OutAmount = 0;
	if (!Stack.IsValid())
	{
		return ERockAddRefusal::InvalidStack;
	}
	const FRockInventorySlotEntry* Entry = GetSlot(Slot);
	const FRockInventorySectionInfo* Section = FindSection(Slot);
	if (!Entry || !Section)
	{
		return ERockAddRefusal::InvalidSlot;
	}
	if (!URockInventoryLibrary::CanItemBePlacedInSection(Stack, *Section))
	{
		return ERockAddRefusal::SectionRejectsItem;
	}

	if (const FRockItemStack* Existing = GetStack(Entry->ItemHandle))
	{
		if (!Existing->CanStackWith(Stack))
		{
			return ERockAddRefusal::NoRoom;
		}
		OutAmount = FMath::Min(Existing->GetMaxStackCount() - Existing->GetStackCount(), Stack.GetStackCount());
		if (OutAmount <= 0)
		{
			OutAmount = 0;
			return ERockAddRefusal::NothingToMerge;
		}
		bOutMerge = true;
		return ERockAddRefusal::None;
	}

	if (!Fits(BuildOccupancy(), *Section, Slot, Stack, Orientation))
	{
		return ERockAddRefusal::NoRoom;
	}
	OutAmount = FMath::Min(Stack.GetMaxStackCount(), Stack.GetStackCount());
	return ERockAddRefusal::None;
}

ERockAddRefusal FRockInventoryData::CanAdd(const FRockInventorySlotHandle& Slot, const FRockItemStack& Stack, ERockItemOrientation Orientation) const
{
	bool bMerge;
	int32 Amount;
	return PlanAdd(Slot, Stack, Orientation, bMerge, Amount);
}

ERockAddRefusal FRockInventoryData::ApplyAdd(
	const FRockInventorySlotHandle& Slot, const FRockItemStack& Stack, ERockItemOrientation Orientation,
	FRockInventoryChangeSet& OutChanges, int32& OutAdded)
{
	OutChanges.Changes.Reset();
	bool bMerge;
	const ERockAddRefusal Refusal = PlanAdd(Slot, Stack, Orientation, bMerge, OutAdded);
	if (Refusal != ERockAddRefusal::None)
	{
		return Refusal;
	}

	if (bMerge)
	{
		const FRockItemStackHandle Handle = Slots[Slot.GetAbsoluteIndex()].ItemHandle;
		FRockItemStack& Existing = Stacks[Handle.GetIndex()];
		const FRockItemStack Before = Existing;
		Existing.StackCount += OutAdded;
		AddStackChange(OutChanges, ERockDataChangeType::StackModified, ERockInventorySide::Target, Handle, Before, Existing);
		return ERockAddRefusal::None;
	}

	FRockItemStack Part = Stack;
	Part.StackCount = OutAdded;
	const FRockItemStackHandle Handle = AllocateStack(Part);
	AddStackChange(OutChanges, ERockDataChangeType::StackCreated, ERockInventorySide::Target, Handle, FRockItemStack(), Stacks[Handle.GetIndex()]);

	FRockInventorySlotEntry& To = Slots[Slot.GetAbsoluteIndex()];
	const FRockInventorySlotEntry ToBefore = To;
	To.ItemHandle = Handle;
	To.Orientation = Orientation;
	AddSlotChange(OutChanges, ERockInventorySide::Target, ToBefore, To);
	return ERockAddRefusal::None;
}

ERockRemoveRefusal FRockInventoryData::CanRemove(const FRockInventorySlotHandle& Slot, int32 Count) const
{
	if (!GetSlot(Slot))
	{
		return ERockRemoveRefusal::InvalidSlot;
	}
	const FRockItemStack* Stack = GetSlotStack(Slot);
	if (!Stack)
	{
		return ERockRemoveRefusal::EmptySlot;
	}
	return Count > Stack->GetStackCount() ? ERockRemoveRefusal::NotEnoughItems : ERockRemoveRefusal::None;
}

ERockRemoveRefusal FRockInventoryData::ApplyRemove(const FRockInventorySlotHandle& Slot, int32 Count, FRockInventoryChangeSet& OutChanges)
{
	OutChanges.Changes.Reset();
	const ERockRemoveRefusal Refusal = CanRemove(Slot, Count);
	if (Refusal != ERockRemoveRefusal::None)
	{
		return Refusal;
	}

	FRockInventorySlotEntry& From = Slots[Slot.GetAbsoluteIndex()];
	const FRockItemStackHandle Handle = From.ItemHandle;
	FRockItemStack& Stack = Stacks[Handle.GetIndex()];
	const FRockItemStack Before = Stack;
	if (Count <= 0 || Count == Stack.GetStackCount())
	{
		const FRockInventorySlotEntry FromBefore = From;
		From.ItemHandle = FRockItemStackHandle::Invalid();
		From.Orientation = ERockItemOrientation::Horizontal;
		AddSlotChange(OutChanges, ERockInventorySide::Source, FromBefore, From);
		FreeStack(Handle);
		AddStackChange(OutChanges, ERockDataChangeType::StackRemoved, ERockInventorySide::Source, Handle, Before, FRockItemStack());
		return ERockRemoveRefusal::None;
	}

	Stack.StackCount -= Count;
	AddStackChange(OutChanges, ERockDataChangeType::StackModified, ERockInventorySide::Source, Handle, Before, Stack);
	return ERockRemoveRefusal::None;
}

int32 FRockInventoryData::CountMatching(const TFunctionRef<bool(const FRockItemStack&)>& Matches) const
{
	int32 Total = 0;
	for (const FRockItemStack& Stack : Stacks)
	{
		if (Stack.IsValid() && Matches(Stack))
		{
			Total += Stack.GetStackCount();
		}
	}
	return Total;
}

int32 FRockInventoryData::RemoveMatching(
	const TFunctionRef<bool(const FRockItemStack&)>& Matches, int32 Count, bool bAllOrNothing, FRockInventoryChangeSet& OutChanges)
{
	OutChanges.Changes.Reset();
	if (Count <= 0)
	{
		return 0;
	}
	if (bAllOrNothing && CountMatching(Matches) < Count)
	{
		return 0;
	}

	int32 Remaining = Count;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && Remaining > 0; ++SlotIndex)
	{
		const FRockInventorySlotHandle SlotHandle = Slots[SlotIndex].SlotHandle;
		const FRockItemStack* Stack = GetSlotStack(SlotHandle);
		if (!Stack || !Matches(*Stack))
		{
			continue;
		}
		const int32 Take = FMath::Min(Remaining, Stack->GetStackCount());
		FRockInventoryChangeSet One;
		ApplyRemove(SlotHandle, Take, One);
		OutChanges.Changes.Append(One.Changes);
		Remaining -= Take;
	}
	return Count - Remaining;
}

ERockMoveRefusal FRockInventoryData::CanMove(
	const FRockInventoryData& Source, const FRockInventorySlotHandle& SourceSlot,
	const FRockInventoryData& Target, const FRockInventorySlotHandle& TargetSlot,
	const FRockMoveItemParams& Params)
{
	FMovePlan Plan;
	return PlanMove(Source, SourceSlot, Target, TargetSlot, Params, Plan);
}

ERockMoveRefusal FRockInventoryData::ApplyMove(
	FRockInventoryData& Source, const FRockInventorySlotHandle& SourceSlot,
	FRockInventoryData& Target, const FRockInventorySlotHandle& TargetSlot,
	const FRockMoveItemParams& Params, FRockInventoryChangeSet& OutChanges)
{
	OutChanges.Changes.Reset();
	FMovePlan Plan;
	const ERockMoveRefusal Refusal = PlanMove(Source, SourceSlot, Target, TargetSlot, Params, Plan);
	if (Refusal != ERockMoveRefusal::None)
	{
		return Refusal;
	}
	const bool bSameData = &Source == &Target;

	switch (Plan.Kind)
	{
	case EMoveKind::NoOp:
		break;

	case EMoveKind::Rotate:
	{
		FRockInventorySlotEntry& Slot = Source.Slots[SourceSlot.GetAbsoluteIndex()];
		const FRockInventorySlotEntry Before = Slot;
		Slot.Orientation = Params.DesiredOrientation;
		AddSlotChange(OutChanges, ERockInventorySide::Source, Before, Slot);
		break;
	}

	case EMoveKind::PlaceFull:
	{
		const FRockItemStackHandle SourceHandle = Source.Slots[SourceSlot.GetAbsoluteIndex()].ItemHandle;
		const FRockItemStack SourceStack = *Source.GetStack(SourceHandle);

		FRockInventorySlotEntry& From = Source.Slots[SourceSlot.GetAbsoluteIndex()];
		const FRockInventorySlotEntry FromBefore = From;
		From.ItemHandle = FRockItemStackHandle::Invalid();
		From.Orientation = ERockItemOrientation::Horizontal;
		AddSlotChange(OutChanges, ERockInventorySide::Source, FromBefore, From);

		FRockItemStackHandle TargetHandle = SourceHandle;
		if (!bSameData)
		{
			Source.FreeStack(SourceHandle);
			AddStackChange(OutChanges, ERockDataChangeType::StackRemoved, ERockInventorySide::Source, SourceHandle, SourceStack, FRockItemStack());
			TargetHandle = Target.AllocateStack(SourceStack);
			AddStackChange(OutChanges, ERockDataChangeType::StackCreated, ERockInventorySide::Target, TargetHandle, FRockItemStack(), Target.Stacks[TargetHandle.GetIndex()]);
		}

		FRockInventorySlotEntry& To = Target.Slots[TargetSlot.GetAbsoluteIndex()];
		const FRockInventorySlotEntry ToBefore = To;
		To.ItemHandle = TargetHandle;
		To.Orientation = Params.DesiredOrientation;
		AddSlotChange(OutChanges, ERockInventorySide::Target, ToBefore, To);
		break;
	}

	case EMoveKind::PlaceSplit:
	{
		const FRockItemStackHandle SourceHandle = Source.Slots[SourceSlot.GetAbsoluteIndex()].ItemHandle;
		FRockItemStack& SourceStack = Source.Stacks[SourceHandle.GetIndex()];
		const FRockItemStack SourceBefore = SourceStack;
		FRockItemStack Part = SourceStack;
		Part.StackCount = Plan.Amount;
		SourceStack.StackCount -= Plan.Amount;
		AddStackChange(OutChanges, ERockDataChangeType::StackModified, ERockInventorySide::Source, SourceHandle, SourceBefore, SourceStack);

		const FRockItemStackHandle TargetHandle = Target.AllocateStack(Part);
		AddStackChange(OutChanges, ERockDataChangeType::StackCreated, ERockInventorySide::Target, TargetHandle, FRockItemStack(), Target.Stacks[TargetHandle.GetIndex()]);

		FRockInventorySlotEntry& To = Target.Slots[TargetSlot.GetAbsoluteIndex()];
		const FRockInventorySlotEntry ToBefore = To;
		To.ItemHandle = TargetHandle;
		To.Orientation = Params.DesiredOrientation;
		AddSlotChange(OutChanges, ERockInventorySide::Target, ToBefore, To);
		break;
	}

	case EMoveKind::Merge:
	{
		const FRockItemStackHandle SourceHandle = Source.Slots[SourceSlot.GetAbsoluteIndex()].ItemHandle;
		const FRockItemStackHandle TargetHandle = Target.Slots[TargetSlot.GetAbsoluteIndex()].ItemHandle;

		FRockItemStack& TargetStack = Target.Stacks[TargetHandle.GetIndex()];
		const FRockItemStack TargetBefore = TargetStack;
		TargetStack.StackCount += Plan.Amount;
		AddStackChange(OutChanges, ERockDataChangeType::StackModified, ERockInventorySide::Target, TargetHandle, TargetBefore, TargetStack);

		FRockItemStack& SourceStack = Source.Stacks[SourceHandle.GetIndex()];
		const FRockItemStack SourceBefore = SourceStack;
		SourceStack.StackCount -= Plan.Amount;
		if (SourceStack.StackCount <= 0)
		{
			Source.FreeStack(SourceHandle);
			AddStackChange(OutChanges, ERockDataChangeType::StackRemoved, ERockInventorySide::Source, SourceHandle, SourceBefore, FRockItemStack());

			FRockInventorySlotEntry& From = Source.Slots[SourceSlot.GetAbsoluteIndex()];
			const FRockInventorySlotEntry FromBefore = From;
			From.ItemHandle = FRockItemStackHandle::Invalid();
			From.Orientation = ERockItemOrientation::Horizontal;
			AddSlotChange(OutChanges, ERockInventorySide::Source, FromBefore, From);
		}
		else
		{
			AddStackChange(OutChanges, ERockDataChangeType::StackModified, ERockInventorySide::Source, SourceHandle, SourceBefore, SourceStack);
		}
		break;
	}
	}
	return ERockMoveRefusal::None;
}
