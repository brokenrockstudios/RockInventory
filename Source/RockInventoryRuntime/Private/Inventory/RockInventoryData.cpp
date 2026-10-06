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
	for (int32 Index = 0; Index < Data.Stacks.Num(); ++Index)
	{
		if (!Data.Stacks[Index].IsValid())
		{
			Data.FreeStackIndices.Add(Index);
		}
	}
	return Data;
}

void FRockInventoryData::Init(const TArray<FRockInventorySectionInfo>& InSections)
{
	Sections.Reset();
	Slots.Reset();
	Stacks.Reset();
	FreeStackIndices.Reset();

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
	int32 Index;
	if (FreeStackIndices.Num() > 0)
	{
		Index = FreeStackIndices.Pop(EAllowShrinking::No);
	}
	else
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
	FreeStackIndices.Add(Index);
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
	Occupancy.Init(false, Slots.Num());

	for (const FRockInventorySectionInfo& Section : Sections)
	{
		const int32 Offset = Section.GetFirstSlotIndex();
		for (int32 LocalIndex = 0; LocalIndex < Section.GetNumSlots(); ++LocalIndex)
		{
			const FRockInventorySlotEntry& Slot = Slots[Offset + LocalIndex];
			if (Slot.ItemHandle == IgnoreHandle)
			{
				continue;
			}
			const FRockItemStack* Stack = GetStack(Slot.ItemHandle);
			if (!Stack)
			{
				continue;
			}
			if (Section.GetSlotSizePolicy() == ERockItemSizePolicy::IgnoreSize)
			{
				Occupancy[Offset + LocalIndex] = true;
				continue;
			}
			const int32 Column = LocalIndex % Section.GetColumns();
			const int32 Row = LocalIndex / Section.GetColumns();
			const FIntPoint Size = URockItemStackLibrary::GetItemSizeForOrientation(*Stack, Slot.Orientation);
			for (int32 Y = 0; Y < Size.Y; ++Y)
			{
				for (int32 X = 0; X < Size.X; ++X)
				{
					const int32 GridIndex = Offset + (Row + Y) * Section.GetColumns() + (Column + X);
					if (Occupancy.IsValidIndex(GridIndex))
					{
						Occupancy[GridIndex] = true;
					}
				}
			}
		}
	}
	return Occupancy;
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
