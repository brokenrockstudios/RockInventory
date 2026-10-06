// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryLibrary.h"

#include "RockInventoryLogging.h"
#include "Components/RockInventoryComponent.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventoryInterface.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockItemStackLibrary.h"


/** State a batch preview carries from one stack to the next: what the earlier stacks would have changed in the live inventory. */
struct FRockLootScratch
{
	/** Occupancy including the footprints of the new stacks decided so far. Filled by the first stack. */
	TArray<bool> Grid;
	bool bGridReady = false;
	/** Units earlier stacks would add to an existing stack, by absolute slot index. */
	TMap<int32, int32> MergedUnits;
	struct FSimulatedStack
	{
		FRockItemStack Stack;
		ERockItemOrientation Orientation = ERockItemOrientation::Horizontal;
	};
	/** New stacks earlier stacks would create, by absolute anchor slot index. Later stacks can merge into them. */
	TMap<int32, FSimulatedStack> NewStacks;
};

namespace
{
	/** Replicated inventory state is written on the authority only. A client call is refused and changes nothing. */
	bool HasMutationAuthority(URockInventory* Inventory, const TCHAR* Operation)
	{
		const AActor* OwningActor = Inventory ? Inventory->GetOwningActor() : nullptr;
		if (OwningActor && OwningActor->HasAuthority())
		{
			return true;
		}
		UE_LOG(LogRockInventory, Warning, TEXT("%s - refused, %s is not owned by an actor with authority"), Operation, *GetNameSafe(Inventory));
		return false;
	}

	enum class ELootSectionVerdict : uint8
	{
		Candidate,
		NoSlots,
		ExcludedByMetaTag,
		IntentNotAccepted,
		FilterRejects,
	};

	/** The hard checks of the loot plan for one section, cheapest first. The type restriction is the same for every slot of the section. */
	ELootSectionVerdict JudgeSectionForLoot(const FRockInventorySectionInfo& Section, const FRockItemStack& ItemStack, const FRockLootParams& Params)
	{
		if (Section.GetFirstSlotIndex() == INDEX_NONE || Section.GetNumSlots() <= 0)
		{
			return ELootSectionVerdict::NoSlots;
		}
		// The caller can keep whole sections out of the call by meta tag
		if (!Params.ExcludeSectionMetaTags.IsEmpty() && Section.GetMetaTags().HasAny(Params.ExcludeSectionMetaTags))
		{
			return ELootSectionVerdict::ExcludedByMetaTag;
		}
		if (!Section.AcceptsLootIntent(Params.Intent))
		{
			return ELootSectionVerdict::IntentNotAccepted;
		}
		if (!URockInventoryLibrary::CanItemBePlacedInSection(ItemStack, Section))
		{
			return ELootSectionVerdict::FilterRejects;
		}
		return ELootSectionVerdict::Candidate;
	}

	/** Soft preference: tier 0 when the section names a preference and the item matches it. Reads the definition's cached tags. */
	int32 LootTierOf(const FRockInventorySectionInfo& Section, const FRockItemStack& ItemStack)
	{
		const FGameplayTagQuery& Preference = Section.GetLootPreference();
		return !Preference.IsEmpty() && Preference.Matches(ItemStack.GetDefinition()->GetAllTags()) ? 0 : 1;
	}

	/** Sort key: tier, then priority, then config order (so equal ranks stay stable). Priority is biased to non-negative. */
	int64 LootPlanKey(const FRockLootPlanEntry& Entry)
	{
		return (static_cast<int64>(Entry.Tier) << 48) | ((static_cast<int64>(Entry.Priority) + 0x80000000LL) << 16) | static_cast<int64>(Entry.SectionIndex & 0xFFFF);
	}
}

void URockInventoryLibrary::BuildLootPlan(
	const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params, FRockLootPlan& OutPlan)
{
	OutPlan.Reset();
	if (!Inventory || !ItemStack.IsValid())
	{
		return;
	}
	for (int32 SectionIndex = 0; SectionIndex < Inventory->SlotSections.Num(); ++SectionIndex)
	{
		const FRockInventorySectionInfo& Section = Inventory->SlotSections[SectionIndex];
		if (JudgeSectionForLoot(Section, ItemStack, Params) != ELootSectionVerdict::Candidate)
		{
			continue;
		}
		FRockLootPlanEntry& Entry = OutPlan.AddDefaulted_GetRef();
		Entry.Inventory = Inventory;
		Entry.SectionIndex = SectionIndex;
		Entry.Tier = LootTierOf(Section, ItemStack);
		Entry.Priority = Section.GetLootPriority();
	}
	OutPlan.Sort([](const FRockLootPlanEntry& A, const FRockLootPlanEntry& B) { return LootPlanKey(A) < LootPlanKey(B); });
}

FString URockInventoryLibrary::DescribeLootPlan(const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params)
{
	if (!Inventory || !ItemStack.IsValid())
	{
		return TEXT("Loot plan: invalid inventory or item");
	}
	FRockLootPlan Plan;
	BuildLootPlan(Inventory, ItemStack, Params, Plan);

	FString Text = FString::Printf(TEXT("Loot plan for %s x%d, intent %d%s%s:"), *ItemStack.GetDebugString(), ItemStack.GetStackCount(), Params.Intent,
		Params.HasIntent(ERockLootIntent::Store) ? TEXT(" Store") : TEXT(""), Params.HasIntent(ERockLootIntent::Equip) ? TEXT(" Equip") : TEXT(""));
	int32 Rank = 1;
	for (const FRockLootPlanEntry& Entry : Plan)
	{
		const FRockInventorySectionInfo& Section = Inventory->SlotSections[Entry.SectionIndex];
		Text += FString::Printf(TEXT("\n  %d. %s (section %d): %s, priority %d"), Rank++, *Section.GetSectionTag().ToString(), Entry.SectionIndex,
			Entry.Tier == 0 ? TEXT("preferred") : TEXT("allowed"), Entry.Priority);
	}
	for (int32 SectionIndex = 0; SectionIndex < Inventory->SlotSections.Num(); ++SectionIndex)
	{
		const FRockInventorySectionInfo& Section = Inventory->SlotSections[SectionIndex];
		const TCHAR* Reason = nullptr;
		switch (JudgeSectionForLoot(Section, ItemStack, Params))
		{
		case ELootSectionVerdict::NoSlots: Reason = TEXT("no slots"); break;
		case ELootSectionVerdict::ExcludedByMetaTag: Reason = TEXT("excluded by a meta tag of the call"); break;
		case ELootSectionVerdict::IntentNotAccepted: Reason = TEXT("does not accept this intent"); break;
		case ELootSectionVerdict::FilterRejects: Reason = TEXT("section filter rejects the item"); break;
		default: break;
		}
		if (Reason)
		{
			Text += FString::Printf(TEXT("\n  skipped %s (section %d): %s"), *Section.GetSectionTag().ToString(), SectionIndex, Reason);
		}
	}
	if (Plan.IsEmpty())
	{
		Text += TEXT("\n  no section can take this item");
	}
	return Text;
}

namespace
{
	bool IsSlotPending(const TBitArray<>& PendingSlots, int32 AbsoluteIndex)
	{
		return PendingSlots.IsValidIndex(AbsoluteIndex) && PendingSlots[AbsoluteIndex];
	}

	/** Marks the cells a stack of this size would cover, the same way PrecomputeOccupancyGrids does for placed items. */
	void MarkFootprint(TArray<bool>& Grid, const FRockInventorySectionInfo& Section, int32 Column, int32 Row, FIntPoint Size)
	{
		if (Section.GetSlotSizePolicy() == ERockItemSizePolicy::IgnoreSize)
		{
			Size = FIntPoint(1, 1);
		}
		for (int32 Y = 0; Y < Size.Y; ++Y)
		{
			for (int32 X = 0; X < Size.X; ++X)
			{
				const int32 GridIndex = Section.GetFirstSlotIndex() + ((Row + Y) * Section.GetColumns() + (Column + X));
				if (Grid.IsValidIndex(GridIndex))
				{
					Grid[GridIndex] = true;
				}
			}
		}
	}

	/** The default orientation if the stack fits at this cell, else rotated for a non-square item. False when neither fits. */
	bool FindFitOrientation(const TArray<bool>& Grid, const FRockInventorySectionInfo& Section, int32 Column, int32 Row, FIntPoint ItemSize, ERockItemOrientation& OutOrientation)
	{
		OutOrientation = ERockItemOrientation::Horizontal;
		if (URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, Column, Row, FVector2D(ItemSize)))
		{
			return true;
		}
		if (ItemSize.X != ItemSize.Y)
		{
			OutOrientation = ERockItemOrientation::Vertical;
			return URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, Column, Row, FVector2D(ItemSize.Y, ItemSize.X));
		}
		return false;
	}
}

/** Slots another operation holds. The first operation listed for a slot is the one that counts. Empty when nothing is pending. */
TBitArray<> URockInventoryLibrary::BuildPendingSlots(const URockInventory* Inventory)
{
	TBitArray<> PendingSlots;
	if (Inventory->PendingSlotOperations.IsEmpty())
	{
		return PendingSlots;
	}
	const int32 NumSlots = Inventory->SlotData.Num();
	PendingSlots.Init(false, NumSlots);
	TBitArray<> Seen(false, NumSlots);
	for (const FRockPendingSlotOperation& Operation : Inventory->PendingSlotOperations)
	{
		const int32 SlotIndex = Operation.SlotHandle.GetAbsoluteIndex();
		if (!Operation.SlotHandle.IsValid() || !Seen.IsValidIndex(SlotIndex) || Seen[SlotIndex])
		{
			continue;
		}
		Seen[SlotIndex] = true;
		PendingSlots[SlotIndex] = Operation.SlotStatus == ERockSlotStatus::Pending;
	}
	return PendingSlots;
}

/** Pass 1: top up partial stacks across the plan. Same rule as CanMergeItemAtGridPosition with the Partial condition, without copying the stack. */
void URockInventoryLibrary::DecideMerges(
	const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootPlan& Plan, const TBitArray<>& PendingSlots,
	int32 SkipAbsoluteIndex, TArray<FRockLootPlacement>& OutPlacements, int32& InOutRemaining, FRockLootScratch* Scratch)
{
	for (const FRockLootPlanEntry& Entry : Plan)
	{
		if (InOutRemaining <= 0)
		{
			return;
		}
		const FRockInventorySectionInfo& Section = Inventory->SlotSections[Entry.SectionIndex];
		const int32 FirstSlotIndex = Section.GetFirstSlotIndex();
		const int32 NumSectionSlots = Section.GetNumSlots();
		for (int32 LocalSlotIndex = 0; LocalSlotIndex < NumSectionSlots && InOutRemaining > 0; ++LocalSlotIndex)
		{
			const int32 AbsoluteIndex = FirstSlotIndex + LocalSlotIndex;
			// Do not overwrite any pending operations, nor merge into a stack that is leaving
			if (AbsoluteIndex == SkipAbsoluteIndex || IsSlotPending(PendingSlots, AbsoluteIndex))
			{
				continue;
			}
			const FRockInventorySlotEntry& Slot = Inventory->SlotData[AbsoluteIndex];
			// A batch preview sees the new stacks and merged units of the earlier stacks too
			FRockLootScratch::FSimulatedStack* Simulated = Scratch ? Scratch->NewStacks.Find(AbsoluteIndex) : nullptr;
			const FRockItemStack* Existing = Simulated ? &Simulated->Stack : Inventory->GetItemByHandlePtr(Slot.ItemHandle);
			const int32 AddedUnits = (Scratch && !Simulated) ? Scratch->MergedUnits.FindRef(AbsoluteIndex) : 0;
			if (Existing && Existing->IsValid() && Existing->CanStackWith(ItemStack) && Existing->GetStackCount() + AddedUnits < Existing->GetMaxStackCount())
			{
				const int32 MergeCount = FMath::Min(Existing->GetMaxStackCount() - Existing->GetStackCount() - AddedUnits, InOutRemaining);
				FRockLootPlacement& Merge = OutPlacements.AddDefaulted_GetRef();
				Merge.Slot = Inventory->MakeSlotReference(Slot.SlotHandle);
				Merge.Count = MergeCount;
				Merge.Orientation = Simulated ? Simulated->Orientation : Slot.Orientation;
				InOutRemaining -= MergeCount;
				if (Simulated)
				{
					Simulated->Stack.StackCount += MergeCount;
				}
				else if (Scratch)
				{
					Scratch->MergedUnits.FindOrAdd(AbsoluteIndex) += MergeCount;
				}
			}
		}
	}
}

/**
 * Pass 2: what is left becomes one new stack in the first slot that fits, following the plan. Prefers the default orientation, then rotated for non-square items.
 * The footprint is marked in the grid when it is placed. Returns false when nothing fits.
 */
bool URockInventoryLibrary::DecideNewStack(
	const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootPlan& Plan, const TBitArray<>& PendingSlots,
	TArray<bool>& InOutGrid, TArray<FRockLootPlacement>& OutPlacements, int32& InOutRemaining, FRockLootScratch* Scratch)
{
	const FIntPoint ItemSize = URockItemStackLibrary::GetItemSize(ItemStack);
	for (const FRockLootPlanEntry& Entry : Plan)
	{
		const FRockInventorySectionInfo& Section = Inventory->SlotSections[Entry.SectionIndex];
		const int32 FirstSlotIndex = Section.GetFirstSlotIndex();
		const int32 NumSectionSlots = Section.GetNumSlots();
		const int32 Columns = Section.GetColumns();
		for (int32 LocalSlotIndex = 0; LocalSlotIndex < NumSectionSlots; ++LocalSlotIndex)
		{
			const int32 AbsoluteIndex = FirstSlotIndex + LocalSlotIndex;
			if (IsSlotPending(PendingSlots, AbsoluteIndex))
			{
				continue;
			}
			const int32 Column = LocalSlotIndex % Columns;
			const int32 Row = LocalSlotIndex / Columns;
			ERockItemOrientation FitOrientation;
			if (FindFitOrientation(InOutGrid, Section, Column, Row, ItemSize, FitOrientation))
			{
				FRockLootPlacement& NewStack = OutPlacements.AddDefaulted_GetRef();
				NewStack.Slot = Inventory->MakeSlotReference(Inventory->SlotData[AbsoluteIndex].SlotHandle);
				NewStack.Count = InOutRemaining;
				NewStack.Orientation = FitOrientation;
				NewStack.bNewStack = true;
				if (Scratch)
				{
					FRockLootScratch::FSimulatedStack& Simulated = Scratch->NewStacks.Add(AbsoluteIndex);
					Simulated.Stack = ItemStack;
					Simulated.Stack.StackCount = InOutRemaining;
					Simulated.Orientation = FitOrientation;
				}
				InOutRemaining = 0;
				MarkFootprint(InOutGrid, Section, Column, Row, FitOrientation == ERockItemOrientation::Horizontal ? ItemSize : FIntPoint(ItemSize.Y, ItemSize.X));
				return true;
			}
		}
	}
	return false;
}

/**
 * Equip with swap: the first occupied slot in plan order that the item fits in once its current stack is gone. That stack is routed through a Store call on a
 * scratch copy of the occupancy (the new item's footprint already marked, the leaving stack excluded from merging). Fills OutResult and returns true when the
 * displaced stack is fully stored; returns false, leaving OutResult untouched, when no slot qualifies or the displaced stack has nowhere to go.
 */
bool URockInventoryLibrary::DecideSwap(
	const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params, const FRockLootPlan& Plan,
	const TBitArray<>& PendingSlots, FRockLootResult& OutResult)
{
	const FIntPoint ItemSize = URockItemStackLibrary::GetItemSize(ItemStack);
	for (const FRockLootPlanEntry& Entry : Plan)
	{
		const FRockInventorySectionInfo& Section = Inventory->SlotSections[Entry.SectionIndex];
		const int32 FirstSlotIndex = Section.GetFirstSlotIndex();
		const int32 Columns = Section.GetColumns();
		for (int32 LocalSlotIndex = 0; LocalSlotIndex < Section.GetNumSlots(); ++LocalSlotIndex)
		{
			const int32 AbsoluteIndex = FirstSlotIndex + LocalSlotIndex;
			if (IsSlotPending(PendingSlots, AbsoluteIndex))
			{
				continue;
			}
			const FRockInventorySlotEntry& Slot = Inventory->SlotData[AbsoluteIndex];
			const FRockItemStack* Occupant = Inventory->GetItemByHandlePtr(Slot.ItemHandle);
			// Only the anchor slot of a stack carries its handle. A stack the item would merge into was already used by the merge pass (a full one is not worth swapping).
			if (!Occupant || !Occupant->IsValid() || Occupant->CanStackWith(ItemStack))
			{
				continue;
			}

			TArray<bool> Grid;
			URockInventoryLibrary::PrecomputeOccupancyGrids(Inventory, Grid, Slot.ItemHandle);
			const int32 Column = LocalSlotIndex % Columns;
			const int32 Row = LocalSlotIndex / Columns;
			ERockItemOrientation FitOrientation;
			if (!FindFitOrientation(Grid, Section, Column, Row, ItemSize, FitOrientation))
			{
				continue;
			}
			MarkFootprint(Grid, Section, Column, Row, FitOrientation == ERockItemOrientation::Horizontal ? ItemSize : FIntPoint(ItemSize.Y, ItemSize.X));

			// The first fitting occupant decides: if it cannot be stored the swap is refused, later slots are not tried
			FRockLootParams StoreParams;
			StoreParams.Intent = static_cast<int32>(ERockLootIntent::Store);
			StoreParams.ExcludeSectionMetaTags = Params.ExcludeSectionMetaTags;
			FRockLootPlan StorePlan;
			URockInventoryLibrary::BuildLootPlan(Inventory, *Occupant, StoreParams, StorePlan);

			TArray<FRockLootPlacement> DisplacedPlacements;
			int32 DisplacedRemaining = Occupant->GetStackCount();
			DecideMerges(Inventory, *Occupant, StorePlan, PendingSlots, AbsoluteIndex, DisplacedPlacements, DisplacedRemaining);
			if (DisplacedRemaining > 0)
			{
				DecideNewStack(Inventory, *Occupant, StorePlan, PendingSlots, Grid, DisplacedPlacements, DisplacedRemaining);
			}
			if (DisplacedRemaining > 0)
			{
				return false;
			}

			FRockLootPlacement& NewStack = OutResult.Placements.AddDefaulted_GetRef();
			NewStack.Slot = Inventory->MakeSlotReference(Slot.SlotHandle);
			NewStack.Count = ItemStack.GetStackCount();
			NewStack.Orientation = FitOrientation;
			NewStack.bNewStack = true;
			OutResult.Excess = 0;
			OutResult.bSwapped = true;
			OutResult.DisplacedSlot = Inventory->MakeSlotReference(Slot.SlotHandle);
			OutResult.DisplacedPlacements = MoveTemp(DisplacedPlacements);
			return true;
		}
	}
	return false;
}

/** Applies placements of one stack in order: merges into existing stacks, a new stack into its slot. */
void URockInventoryLibrary::ApplyPlacements(URockInventory* Inventory, const FRockItemStack& ItemStack, const TArray<FRockLootPlacement>& Placements)
{
	// The one copy: the merge and add calls take the stack, and its count is the part being placed
	FRockItemStack Placing = ItemStack;
	for (const FRockLootPlacement& Placement : Placements)
	{
		const FRockInventorySlotHandle SlotHandle = Placement.Slot.GetSlotHandle();
		Placing.StackCount = Placement.Count;
		if (!Placement.bNewStack)
		{
			URockInventoryLibrary::MergeItemAtGridPosition(Inventory, SlotHandle, Placing);
			continue;
		}
		const FRockItemStackHandle& ItemHandle = Inventory->AddItemToInventory(Placing);
		FRockInventorySlotEntry SlotEntry = Inventory->GetSlotByHandle(SlotHandle);
		SlotEntry.ItemHandle = ItemHandle;
		SlotEntry.Orientation = Placement.Orientation;
		Inventory->SetSlotByHandle(SlotHandle, SlotEntry);
	}
}

void URockInventoryLibrary::DecideLoot(
	const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params, FRockLootResult& OutResult, FRockLootScratch* Scratch)
{
	OutResult.Placements.Reset();
	OutResult.bSwapped = false;
	OutResult.DisplacedSlot = FRockSlotReference();
	OutResult.DisplacedPlacements.Reset();
	int32 Remaining = ItemStack.GetStackCount();
	OutResult.Excess = Remaining;

	// A batch preview keeps one grid across its stacks; a single call computes its own
	TArray<bool> LocalGrid;
	TArray<bool>& OccupancyGrid = Scratch ? Scratch->Grid : LocalGrid;
	if (!Scratch || !Scratch->bGridReady)
	{
		PrecomputeOccupancyGrids(Inventory, OccupancyGrid);
		if (Scratch)
		{
			Scratch->bGridReady = true;
		}
	}
	const TBitArray<> PendingSlots = BuildPendingSlots(Inventory);

	// The sections this call may use, in the order it tries them (computed once per call, not per slot)
	FRockLootPlan Plan;
	BuildLootPlan(Inventory, ItemStack, Params, Plan);

	DecideMerges(Inventory, ItemStack, Plan, PendingSlots, INDEX_NONE, OutResult.Placements, Remaining, Scratch);
	OutResult.Excess = Remaining;
	if (Remaining > 0)
	{
		DecideNewStack(Inventory, ItemStack, Plan, PendingSlots, OccupancyGrid, OutResult.Placements, Remaining, Scratch);
		OutResult.Excess = Remaining;
	}

	// Equip with swap: only when nothing was placed at all (empty slots and partial stacks come first). Not simulated in a batch preview.
	if (!Scratch && Remaining > 0 && OutResult.Placements.IsEmpty() && Params.CanSwap())
	{
		DecideSwap(Inventory, ItemStack, Params, Plan, PendingSlots, OutResult);
	}
}

void URockInventoryLibrary::CommitLoot(URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootResult& Decision)
{
	if (!Decision.bSwapped)
	{
		ApplyPlacements(Inventory, ItemStack, Decision.Placements);
		return;
	}
	// Swap: the displaced stack leaves its slot first, the new stack takes the slot, then the displaced stack goes where the decision stored it
	const FRockItemStack Displaced = SplitItemStackAtLocation(Inventory, Decision.DisplacedSlot.GetSlotHandle());
	ApplyPlacements(Inventory, ItemStack, Decision.Placements);
	if (Displaced.IsValid())
	{
		ApplyPlacements(Inventory, Displaced, Decision.DisplacedPlacements);
	}
}

bool URockInventoryLibrary::LootItemToInventory(
	URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params, FRockLootResult& OutResult)
{
	// Start off with the full stack size in the event we can't place it
	OutResult = FRockLootResult();
	OutResult.Excess = ItemStack.GetStackCount();
	UE_LOG(LogRockInventory, Verbose, TEXT("LootItemToInventory::ItemStack: %s"), *ItemStack.GetDebugString());
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("LootItemToInventory::Invalid Parameters. Inventory"));
		return false;
	}
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("LootItemToInventory::Invalid Parameters. ItemStack"));
		return false;
	}
	if (!HasMutationAuthority(Inventory, TEXT("LootItemToInventory")))
	{
		return false;
	}

	DecideLoot(Inventory, ItemStack, Params, OutResult);
	CommitLoot(Inventory, ItemStack, OutResult);

	// A partial placement still consumed some of the item. The caller must update their ItemStack with the excess.
	return OutResult.IsFullyPlaced();
}

FRockLootResult URockInventoryLibrary::PreviewLoot(const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params)
{
	FRockLootResult Result;
	Result.Excess = ItemStack.GetStackCount();
	if (Inventory && ItemStack.IsValid())
	{
		DecideLoot(Inventory, ItemStack, Params, Result);
	}
	return Result;
}

TArray<FRockLootResult> URockInventoryLibrary::PreviewLoot(const URockInventory* Inventory, const TArray<FRockItemStack>& ItemStacks, const FRockLootParams& Params)
{
	TArray<FRockLootResult> Results;
	Results.SetNum(ItemStacks.Num());
	FRockLootScratch Scratch;
	for (int32 Index = 0; Index < ItemStacks.Num(); ++Index)
	{
		Results[Index].Excess = ItemStacks[Index].GetStackCount();
		if (Inventory && ItemStacks[Index].IsValid())
		{
			DecideLoot(Inventory, ItemStacks[Index], Params, Results[Index], &Scratch);
		}
	}
	return Results;
}

FRockItemStack URockInventoryLibrary::SplitItemStackAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, int32 Quantity)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return FRockItemStack::Invalid();
	}
	if (!HasMutationAuthority(Inventory, TEXT("SplitItemStackAtLocation")))
	{
		return FRockItemStack::Invalid();
	}
	const int32 slotIndex = SlotHandle.GetAbsoluteIndex();
	if (!Inventory->SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid SlotHandle: %s"), *SlotHandle.ToString());
		return FRockItemStack::Invalid();
	}

	FRockInventorySlotEntry SourceSlot = Inventory->GetSlotByHandle(SlotHandle);
	FRockItemStack Item = Inventory->GetItemByHandle(SourceSlot.ItemHandle);

	if (!Item.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("No valid item at slot"));
		return FRockItemStack::Invalid();
	}

	const int32 CurrentStackSize = Item.GetStackCount();

	// If quantity is 0 or negative, remove the entire stack
	if (Quantity <= 0)
	{
		Quantity = CurrentStackSize;
	}

	// Create the return item stack with the requested quantity
	FRockItemStack OutItemStack = Item;
	OutItemStack.StackCount = FMath::Min(Quantity, CurrentStackSize);

	const FRockItemStackHandle CachedItemHandle = SourceSlot.ItemHandle;

	const bool bIsFullStackMove = (Quantity >= CurrentStackSize);
	if (bIsFullStackMove)
	{
		// They should be the same at this point, otherwise something else might be going on?
		checkf(Item.ItemHandle == SourceSlot.ItemHandle, TEXT("ItemHandle mismatch"));

		// Remove the item from the inventory. It's up to the caller to add it back if needed.
		Inventory->RemoveItemFromInventory(Item);

		// Remove entire stack - invalidate the slot and item
		SourceSlot.ItemHandle = FRockItemStackHandle::Invalid();
		SourceSlot.Orientation = ERockItemOrientation::Horizontal;
	}
	else
	{
		// Partial removal - just update the stack size
		Item.StackCount = CurrentStackSize - Quantity;
		checkf(Item.GetStackCount() > 0, TEXT("ItemStack size is 0 or negative. Should have used full stack move path"));
		Inventory->SetItemByHandle(CachedItemHandle, Item);
	}
	Inventory->SetSlotByHandle(SlotHandle, SourceSlot);
	// Set the item handle to invalid, since we are returning a new item stack. It's not the same item anymore.
	// If it gets added to an inventory, it will get a new handle.
	OutItemStack.ItemHandle = FRockItemStackHandle::Invalid();
	return OutItemStack;
}

bool URockInventoryLibrary::MoveItem(
	URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
	URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
	const FRockMoveItemParams& InMoveParams)
{
	if ((SourceInventory && !HasMutationAuthority(SourceInventory, TEXT("MoveItem")))
		|| (TargetInventory && !HasMutationAuthority(TargetInventory, TEXT("MoveItem"))))
	{
		return false;
	}
	if (SourceInventory && SourceInventory == TargetInventory && SourceSlotHandle == TargetSlotHandle)
	{
		const FRockInventorySlotEntry& CurrentSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
		if (!CurrentSlot.IsValid() || !CurrentSlot.ItemHandle.IsValid() || CurrentSlot.Orientation == InMoveParams.DesiredOrientation)
		{
			// Nothing to do, item is already in the target location
			return true;
		}

		// Same slot, different orientation: rotate in place if the rotated footprint fits.
		const FRockItemStack& RotatingItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
		if (!RotatingItem.IsValid())
		{
			return true;
		}
		const FRockInventorySectionInfo& RotatingSection = SourceInventory->GetSectionInfoBySlotHandle(SourceSlotHandle);
		const int32 RotatingLocalIndex = RotatingSection.GetLocalIndex(SourceSlotHandle.GetAbsoluteIndex());
		TArray<bool> RotatingGrid;
		PrecomputeOccupancyGrids(SourceInventory, RotatingGrid, CurrentSlot.ItemHandle);
		const FVector2D RotatedSize = FVector2D(URockItemStackLibrary::GetItemSizeForOrientation(RotatingItem, InMoveParams.DesiredOrientation));
		if (!CanItemFitInGridPosition(RotatingGrid, RotatingSection, RotatingLocalIndex % RotatingSection.GetColumns(), RotatingLocalIndex / RotatingSection.GetColumns(), RotatedSize))
		{
			return false;
		}
		FRockInventorySlotEntry RotatedSlot = CurrentSlot;
		RotatedSlot.Orientation = InMoveParams.DesiredOrientation;
		SourceInventory->SetSlotByHandle(SourceSlotHandle, RotatedSlot);
		return true;
	}

	// If the TargetInventory is 'null', should we assume we are trying to 'drop' the item?
	if (!SourceInventory || !TargetInventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Source or Target Inventory"));
		return false;
	}
	const FRockInventorySlotEntry& ValidatedSourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
	if (!ValidatedSourceSlot.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Source Slot Handle"));
		return false;
	}
	const FRockItemStack& ValidatedSourceItem = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
	if (!ValidatedSourceItem.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Source Slot is empty"));
		return false;
	}
	// Check TargetInventory if slot is empty
	const FRockInventorySlotEntry& ValidatedTargetSlot = TargetInventory->GetSlotByHandle(TargetSlotHandle);
	if (!ValidatedTargetSlot.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Target Slot Handle"));
		return false;
	}
	// Can CanItemBePlacedInSection of TargetInventory
	FRockInventorySectionInfo TargetSection = TargetInventory->GetSectionInfoBySlotHandle(TargetSlotHandle);
	if (!CanItemBePlacedInSection(ValidatedSourceItem, TargetSection))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Item cannot be placed in target section"));
		return false;
	}
	int32 MoveAmount = URockItemStackLibrary::CalculateMoveAmount(ValidatedSourceItem, InMoveParams.MoveMode, InMoveParams.MoveCount);
	if (MoveAmount <= 0)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid move amount calculated"));
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	/// Move
	TArray<bool> OccupancyGrid;

	// If moving between 2 different inventories, the ItemHandle at destination could in theory have the same index as the source
	// which means we need to only ignore the item if we are moving internal to the same inventory
	if (SourceInventory == TargetInventory)
	{
		PrecomputeOccupancyGrids(TargetInventory, OccupancyGrid, ValidatedSourceSlot.ItemHandle);
	}
	else
	{
		// Don't ignore any items in the target inventory
		PrecomputeOccupancyGrids(TargetInventory, OccupancyGrid);
	}
	const FRockInventorySectionInfo& targetSection = TargetInventory->GetSectionInfoBySlotHandle(TargetSlotHandle);
	const int32 localIndex = targetSection.GetLocalIndex(TargetSlotHandle.GetAbsoluteIndex());
	const int32 Column = localIndex % targetSection.GetColumns();
	const int32 Row = localIndex / targetSection.GetColumns();
	const FVector2D ItemSize = FVector2D(URockItemStackLibrary::GetItemSizeForOrientation(ValidatedSourceItem, InMoveParams.DesiredOrientation));

	if (CanItemFitInGridPosition(OccupancyGrid, targetSection, Column, Row, ItemSize))
	{
		FRockInventorySlotEntry targetSlot = ValidatedTargetSlot;
		const bool isFullStackMove = (MoveAmount == ValidatedSourceItem.GetStackCount());
		const bool isSameInventory = (SourceInventory == TargetInventory);

		if (isFullStackMove)
		{
			FRockInventorySlotEntry sourceSlot = ValidatedSourceSlot;
			// Invalidate the source slot
			sourceSlot.ItemHandle = FRockItemStackHandle::Invalid();
			sourceSlot.Orientation = ERockItemOrientation::Horizontal;
			SourceInventory->SetSlotByHandle(SourceSlotHandle, sourceSlot);

			if (isSameInventory)
			{
				// Same Inventory - just move the handle to the target slot.
				targetSlot.ItemHandle = ValidatedSourceItem.ItemHandle;
			}
			else
			{
				// Different inventory.
				// Release from source first. RemoveItemFromInventory unregisters the RuntimeInstance from its current
				// replication owner, so doing it after the add would unregister it from the target's owner instead.
				SourceInventory->RemoveItemFromInventory(ValidatedSourceItem);
				// Add to target
				targetSlot.ItemHandle = TargetInventory->AddItemToInventory(ValidatedSourceItem);
			}

			// Set up target slot with existing item handle
			targetSlot.Orientation = InMoveParams.DesiredOrientation;
			TargetInventory->SetSlotByHandle(TargetSlotHandle, targetSlot);
		}
		else
		{
			// Partial move

			// Split the source item stack based on the move amount
			auto ItemDef = ValidatedSourceItem.GetDefinition();

			// We currently aren't supporting partial moves of items that require runtime instances.
			if (!ItemDef->RuntimeInstanceClass.IsNull())
			{
				UE_LOG(LogRockInventory, Warning, TEXT("Partial moves of items that require runtime instances are not supported"));
				return false;
			}

			const FRockItemStack ItemToMove = SplitItemStackAtLocation(SourceInventory, SourceSlotHandle, MoveAmount);
			if (!ItemToMove.IsValid())
			{
				UE_LOG(LogRockInventory, Warning, TEXT("Failed to split item stack"));
				return false;
			}

			// Add split to target inventory.
			const FRockItemStackHandle& newItemHandle = TargetInventory->AddItemToInventory(ItemToMove);

			// Update target slot with new item
			targetSlot.ItemHandle = newItemHandle;
			targetSlot.Orientation = InMoveParams.DesiredOrientation;
			TargetInventory->SetSlotByHandle(TargetSlotHandle, targetSlot);
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////
	/// Merge into an existing item
	if (CanMergeItemAtGridPosition(TargetInventory, TargetSlotHandle, ValidatedSourceItem, ERockItemStackMergeCondition::Partial))
	{
		// Get the target item to calculate how much we can move
		const FRockItemStack& TargetItem = TargetInventory->GetItemByHandle(ValidatedTargetSlot.ItemHandle);
		if (!TargetItem.IsValid())
		{
			UE_LOG(LogRockInventory, Warning, TEXT("Invalid target item for merging"));
			return false;
		}

		const int32 targetCurrentStack = TargetItem.GetStackCount();
		const int32 targetMaxStack = TargetItem.GetMaxStackCount();
		const int32 sourceCurrentStack = ValidatedSourceItem.GetStackCount();

		// Calculate how much we can move
		const int32 availableSpace = targetMaxStack - targetCurrentStack;
		const int32 amountToMove = FMath::Min3(availableSpace, sourceCurrentStack, MoveAmount);

		if (amountToMove <= 0)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("No items can be merged"));
			return false;
		}

		// Update target item with new stack size
		FRockItemStack UpdatedTargetItem = TargetItem;
		UpdatedTargetItem.StackCount = targetCurrentStack + amountToMove;
		checkf(UpdatedTargetItem.GetStackCount() <= targetMaxStack,
		       TEXT("Updated target item stack size exceeds max: %d > %d"),
		       UpdatedTargetItem.GetStackCount(),
		       targetMaxStack);
		TargetInventory->SetItemByHandle(ValidatedTargetSlot.ItemHandle, UpdatedTargetItem);

		// Update source item with remaining stack size
		FRockItemStack UpdatedSourceItem = ValidatedSourceItem;
		UpdatedSourceItem.StackCount = sourceCurrentStack - amountToMove;
		checkf(UpdatedSourceItem.GetStackCount() >= 0,
		       TEXT("Updated source item stack size is negative: %d"),
		       UpdatedSourceItem.GetStackCount());

		const bool isSourceEmptied = (UpdatedSourceItem.GetStackCount() <= 0);

		if (isSourceEmptied)
		{
			FRockInventorySlotEntry sourceSlot = ValidatedSourceSlot;

			// Release the item
			SourceInventory->RemoveItemFromInventory(ValidatedSourceItem);

			// Clear the slot
			sourceSlot.ItemHandle = FRockItemStackHandle::Invalid();
			sourceSlot.Orientation = ERockItemOrientation::Horizontal;
			SourceInventory->SetSlotByHandle(SourceSlotHandle, sourceSlot);
		}
		else
		{
			// Source item still has items left, so we need to broadcast that it changed.
			SourceInventory->SetItemByHandle(ValidatedSourceSlot.ItemHandle, UpdatedSourceItem);
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// NOTE: Only cross this bridge when we get there.
	// TODO: Swap Item
	// Some games like Diablo support this but Tarkov does not.
	// The fact that some items can be placed 'into' other items makes this more complex.
	// We might not ever support this scenario.
	UE_LOG(LogRockInventory, Warning, TEXT("Item cannot be moved to target location"));
	return false;
}

bool URockInventoryLibrary::CanMergeItemAtGridPosition(
	const URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack,
	ERockItemStackMergeCondition MergeCondition)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return false;
	}
	if (!SlotHandle.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Slot Handle"));
		return false;
	}
	const FRockItemStack& ExistingItemStack = Inventory->GetItemBySlotHandle(SlotHandle);
	if (!ExistingItemStack.IsValid())
	{
		return false;
	}

	if (!ExistingItemStack.CanStackWith(ItemStack))
	{
		return false;
	}

	const int32 CurrentStackSize = ExistingItemStack.GetStackCount();
	const int32 MaxStackCount = ExistingItemStack.GetMaxStackCount();
	const int32 IncomingStackSize = ItemStack.GetStackCount();

	switch (MergeCondition)
	{
	case ERockItemStackMergeCondition::Full:
		// Can we merge the entire incoming stack?
		return (CurrentStackSize + IncomingStackSize) <= MaxStackCount;
	case ERockItemStackMergeCondition::Partial:
		// Is there any space at all in the existing stack?
		return CurrentStackSize < MaxStackCount;
	case ERockItemStackMergeCondition::None:
		// Cannot merge at all
		return CurrentStackSize >= MaxStackCount;
	default:
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid merge condition"));
		return false;
	}
}

int32 URockInventoryLibrary::MergeItemAtGridPosition(
	URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack)
{
	int32 stackSize = ItemStack.GetStackCount();
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return stackSize;
	}
	if (!HasMutationAuthority(Inventory, TEXT("MergeItemAtGridPosition")))
	{
		return stackSize;
	}
	if (!SlotHandle.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Slot Handle"));
		return stackSize;
	}
	const FRockInventorySlotEntry Slot = Inventory->GetSlotByHandle(SlotHandle);
	if (!Slot.IsValid() || !Slot.ItemHandle.IsValid())
	{
		// might be noisy?
		// UE_LOG(LogRockInventory, Warning, TEXT("Invalid Slot Handle"));
		return stackSize;
	}

	FRockItemStack ExistingItemStack = Inventory->GetItemBySlotHandle(SlotHandle);
	if (!ExistingItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Merging Failed: Invalid ItemStack"));
		return stackSize;
	}

	const int32 NewStackSize = ExistingItemStack.GetStackCount() + stackSize;
	const int32 MaxStackCount = ExistingItemStack.GetMaxStackCount();

	if (NewStackSize > MaxStackCount)
	{
		ExistingItemStack.StackCount = MaxStackCount;
		stackSize = NewStackSize - MaxStackCount;
	}
	else
	{
		ExistingItemStack.StackCount = NewStackSize;
		stackSize = 0;
	}
	Inventory->SetItemByHandle(Slot.ItemHandle, ExistingItemStack);
	return stackSize;
}

FRockItemStack URockInventoryLibrary::GetItemBySlotHandle(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemAtLocation: Invalid Inventory"));
		return FRockItemStack::Invalid();
	}
	return Inventory->GetItemBySlotHandle(SlotHandle);
}

FRockItemStack URockInventoryLibrary::GetItemByItemHandle(URockInventory* Inventory, const FRockItemStackHandle& ItemHandle)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemAtLocation: Invalid Inventory"));
		return FRockItemStack::Invalid();
	}
	return Inventory->GetItemByHandle(ItemHandle);
}

int32 URockInventoryLibrary::GetItemCount(const URockInventory* Inventory, const FName& ItemId)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Invalid Inventory"));
		return 0;
	}
	int32 ItemCount = 0;
	for (const FRockItemStack& ItemStack : Inventory->ItemData)
	{
		if (ItemStack.IsValid() && ItemStack.GetItemId() == ItemId)
		{
			ItemCount += ItemStack.GetStackCount();
		}
	}
	return ItemCount;
}

// bool URockInventoryLibrary::DropItem(URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle)
// {
// 	// Should we just call RemoveItem for 'drop' instead?
// 	// Since this Library shouldn't be responsible for 'spawning' the item, because who knows where we want to spawn it.
// 	
// }

bool URockInventoryLibrary::CanItemBePlacedInSection(
	const FRockItemStack& ItemStack,
	const FRockInventorySectionInfo& SectionInfo)
{
	// Check if the section has any type restrictions
	if (SectionInfo.GetSectionFilter().IsEmpty())
	{
		return true;
	}

	return SectionInfo.GetSectionFilter().Matches(ItemStack.GetDefinition()->GetAllTags());
}


void URockInventoryLibrary::PrecomputeOccupancyGrids(
	const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid, FRockItemStackHandle IgnoreItemHandle)
{
	const int32 totalGridSize = Inventory->SlotData.Num();
	OutOccupancyGrid.SetNum(totalGridSize);
	// Initialize all cells to false (unoccupied)
	OutOccupancyGrid.Init(false, totalGridSize);

	for (int32 TabIndex = 0; TabIndex < Inventory->SlotSections.Num(); ++TabIndex)
	{
		const FRockInventorySectionInfo& SectionInfo = Inventory->SlotSections[TabIndex];
		const int32 TabOffset = SectionInfo.GetFirstSlotIndex();

		// Mark occupied cells
		for (int32 SlotIndex = 0; SlotIndex < SectionInfo.GetNumSlots(); ++SlotIndex)
		{
			checkf(0 <= SlotIndex && SlotIndex < Inventory->SlotData.Num(),
			       TEXT("SlotIndex is out of range: %d (max: %d)"),
			       SlotIndex,
			       Inventory->SlotData.Num() - 1);

			const int32 Column = (SlotIndex) % SectionInfo.GetColumns();
			const int32 Row = (SlotIndex) / SectionInfo.GetColumns();

			const FRockInventorySlotEntry& ExistingItemSlot = Inventory->SlotData[TabOffset + SlotIndex];
			if (ExistingItemSlot.ItemHandle == IgnoreItemHandle)
			{
				continue; // Skip the item we are ignoring
			}

			const FRockItemStack& ExistingItemStack = Inventory->GetItemByHandle(ExistingItemSlot.ItemHandle);

			if (ExistingItemStack.IsValid())
			{
				const FVector2D ItemSize = FVector2D(URockItemStackLibrary::GetItemSizeForOrientation(ExistingItemStack, ExistingItemSlot.Orientation));
				auto SizePolicy = SectionInfo.GetSlotSizePolicy();

				// If size policy is IgnoreSize, only mark the single slot as occupied
				if (SizePolicy == ERockItemSizePolicy::IgnoreSize)
				{
					const int32 GridIndex = TabOffset + SlotIndex;
					checkf(0 <= GridIndex && GridIndex < totalGridSize,
					       TEXT("Grid index out of range: %d (max: %d)"),
					       GridIndex,
					       totalGridSize - 1);
					OutOccupancyGrid[GridIndex] = true;
				}
				else
				{
					// Mark all cells this item occupies as true
					for (int32 Y = 0; Y < ItemSize.Y; ++Y)
					{
						for (int32 X = 0; X < ItemSize.X; ++X)
						{
							const int32 GridX = Column + X;
							const int32 GridY = Row + Y;
							const int32 GridIndex = TabOffset + (GridY * SectionInfo.GetColumns() + GridX);
							checkf(0 <= GridIndex && GridIndex < totalGridSize,
							       TEXT("Grid index out of range: %d (max: %d)"),
							       GridIndex,
							       totalGridSize - 1);
							OutOccupancyGrid[GridIndex] = true;
						}
					}
				}
			}
		}
	}
}

bool URockInventoryLibrary::CanItemFitInGridPosition(
	const TArray<bool>& OccupancyGrid, const FRockInventorySectionInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize)
{
	// If this is an unrestricted section, we only need to check if the slot is occupied
	// Item size is irrelevant.
	if (TabInfo.GetSlotSizePolicy() == ERockItemSizePolicy::IgnoreSize)
	{
		// Without this, an out-of-range X would wrap into the next row (or another section).
		if (X < 0 || Y < 0 || X >= TabInfo.GetColumns() || Y >= TabInfo.GetRows())
		{
			return false;
		}
		const int32 GridIndex = TabInfo.GetFirstSlotIndex() + (Y * TabInfo.GetColumns() + X);
		if (GridIndex < 0 || GridIndex >= OccupancyGrid.Num())
		{
			return false;
		}
		return !OccupancyGrid[GridIndex];
	}

	const int32 ItemSizeX = ItemSize.X;
	const int32 ItemSizeY = ItemSize.Y;

	// pre-check to avoid wasting time on partial fits
	if (X < 0 || Y < 0 || X + ItemSizeX > TabInfo.GetColumns() || Y + ItemSizeY > TabInfo.GetRows())
	{
		// Out of bounds
		return false;
	}

	for (int32 ItemY = 0; ItemY < ItemSizeY; ++ItemY)
	{
		for (int32 ItemX = 0; ItemX < ItemSizeX; ++ItemX)
		{
			const int32 GridIndex = TabInfo.GetFirstSlotIndex() + ((Y + ItemY) * TabInfo.GetColumns() + (X + ItemX));
			if (GridIndex < 0 || GridIndex >= OccupancyGrid.Num())
			{
				// Out of bounds
				return false;
			}
			if (OccupancyGrid[GridIndex])
			{
				// Cell is occupied
				return false;
			}
		}
	}
	return true;
}

TArray<FString> URockInventoryLibrary::GetInventoryContentsDebug(const URockInventory* Inventory)
{
	if (!Inventory)
	{
		return {TEXT("Invalid Inventory")};
	}

	TArray<FString> InventoryContents;
	for (const FRockInventorySlotEntry& Slot : Inventory->SlotData)
	{
		FRockInventorySectionInfo section = Inventory->GetSectionInfoBySlotHandle(Slot.SlotHandle);
		const int32 localSlotIndex = section.GetLocalIndex(Slot.SlotHandle.GetAbsoluteIndex());

		const FRockItemStack& ItemStack = Inventory->GetItemByHandle(Slot.ItemHandle);
		auto SectionInfo = Inventory->GetSectionInfoBySlotHandle(Slot.SlotHandle);

		FString LineItem = FString::Printf(
			TEXT("Section:[%s] SlotIdx:[%d]; localIndex:[%d] ItemIdx:[%s], Item:[%s] Count:[%d]"),
			*SectionInfo.GetSectionTag().ToString(),
			Slot.SlotHandle.GetAbsoluteIndex(),
			localSlotIndex,
			*Slot.ItemHandle.ToString(),
			ItemStack.GetDefinition() ? *ItemStack.GetDefinition()->Name.ToString() : TEXT("None"),
			ItemStack.GetStackCount());

		InventoryContents.Add(LineItem);
	}

	for (const FRockItemStack& ItemStack : Inventory->ItemData)
	{
		FString LineItem = FString::Printf(
			TEXT("ItemIdx:[%s], Item:[%s] Count:[%d]"),
			*ItemStack.ItemHandle.ToString(),
			ItemStack.GetDefinition() ? *ItemStack.GetDefinition()->Name.ToString() : TEXT("None"),
			ItemStack.GetStackCount());

		InventoryContents.Add(LineItem);
	}
	return InventoryContents;
}

UObject* URockInventoryLibrary::GetTopLevelOwner(UObject* Instance)
{
	UObject* Current = Instance;
	while (Current)
	{
		if (AActor* Actor = Cast<AActor>(Current))
		{
			return Actor;
		}
		else if (UActorComponent* Comp = Cast<UActorComponent>(Current))
		{
			return Comp;
		}
		else if (const URockInventory* Inv = Cast<URockInventory>(Current))
		{
			Current = Inv->GetOwner();
			if (!Current)
			{
				// If we didn't have a proper owner, try and get the outer instead
				Current = Inv->GetOuter();
			}
		}
		else if (const URockItemInstance* ItemInstance = Cast<URockItemInstance>(Current))
		{
			Current = ItemInstance->OwningInventory;

			// This might happen if the item is on a WorldItem and not in an inventory
			if (!Current)
			{
				Current = ItemInstance->GetOuter();
			}
		}
		else
		{
			UE_LOG(LogRockInventory, Warning, TEXT("GetOwningActor Failed"));
			break;
		}
	}
	return nullptr;
}

URockInventory* URockInventoryLibrary::GetInventory(AActor* Actor, bool bFindComponentByClass)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	// Fast path
	if (const IRockInventoryOwnerInterface* InventoryOwner = Cast<IRockInventoryOwnerInterface>(Actor))
	{
		if (URockInventory* inventory = InventoryOwner->GetInventory())
		{
			return inventory;
		}
		// If the interface returns null, we could still try to find the component thru the actor's components
	}
	// Slow path
	if (bFindComponentByClass)
	{
		// If we didn't find it directly, search through all components
		if (URockInventoryComponent* Component = Actor->FindComponentByClass<URockInventoryComponent>())
		{
			return Component->Inventory;
		}
	}

	return nullptr;
}

int32 URockInventoryLibrary::GetSlotIndex(const FRockInventorySlotHandle& SlotHandle)
{
	return SlotHandle.GetAbsoluteIndex();
}

void URockInventoryLibrary::SetCustomValue1(URockInventory* Inventory, const FRockItemStackHandle& ItemHandle, int32 NewValue)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetCustomValue1: Invalid Inventory"));
		return;
	}
	if (!HasMutationAuthority(Inventory, TEXT("SetCustomValue1")))
	{
		return;
	}
	FRockItemStack ItemStack = Inventory->GetItemByHandle(ItemHandle);
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetCustomValue1: Invalid ItemHandle"));
		return;
	}
	ItemStack.CustomValue1 = NewValue;
	Inventory->SetItemByHandle(ItemHandle, ItemStack);
}

void URockInventoryLibrary::SetCustomValue2(URockInventory* Inventory, const FRockItemStackHandle& ItemHandle, int32 NewValue)
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetCustomValue2: Invalid Inventory"));
		return;
	}
	if (!HasMutationAuthority(Inventory, TEXT("SetCustomValue2")))
	{
		return;
	}
	FRockItemStack ItemStack = Inventory->GetItemByHandle(ItemHandle);
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetCustomValue2: Invalid ItemHandle"));
		return;
	}
	ItemStack.CustomValue2 = NewValue;
	Inventory->SetItemByHandle(ItemHandle, ItemStack);
}

FRockInventorySlotHandle URockInventoryLibrary::FindFirstSlotInSection(URockInventory* Inventory, FGameplayTag SectionTag)
{
	if (!Inventory) { return FRockInventorySlotHandle::Invalid(); }

	// We use the Core Query system to build the specific search
	FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithSectionTag(SectionTag);
	if (const FRockInventorySlotEntry* Slot = Inventory->FindFirstSlot(Q))
	{
		return Slot->SlotHandle; // Return the handle (safe for BP/external use)
	}

	return FRockInventorySlotHandle::Invalid();
}

TArray<FRockInventorySlotHandle> URockInventoryLibrary::FindAllSlotsInSection(URockInventory* Inventory, FGameplayTag SectionTag)
{
	TArray<FRockInventorySlotHandle> Slots;
	if (!Inventory) { return Slots; }

	// We use the Core Query system to build the specific search
	FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithSectionTag(SectionTag);
	TArray<FRockInventorySlotEntry> FoundSlots = Inventory->FindAllSlots(Q);
	for (const FRockInventorySlotEntry& Slot : FoundSlots)
	{
		Slots.Add(Slot.SlotHandle);
	}
	return Slots;
}

FRockInventorySlotHandle URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(URockInventory* Inventory, FGameplayTag SectionMetaTag)
{
	if (!Inventory) { return FRockInventorySlotHandle::Invalid(); }

	// We use the Core Query system to build the specific search
	FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithMetaTag(SectionMetaTag);
	if (const FRockInventorySlotEntry* Slot = Inventory->FindFirstSlot(Q))
	{
		return Slot->SlotHandle; // Return the handle (safe for BP/external use)
	}

	return FRockInventorySlotHandle::Invalid();
}

TArray<FRockInventorySlotHandle> URockInventoryLibrary::FindAllSlotsInSectionsWithMetaTag(URockInventory* Inventory, FGameplayTag SectionMetaTag)
{
	TArray<FRockInventorySlotHandle> Slots;
	if (!Inventory) { return Slots; }

	// We use the Core Query system to build the specific search
	FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithMetaTag(SectionMetaTag);
	TArray<FRockInventorySlotEntry> FoundSlots = Inventory->FindAllSlots(Q);
	for (FRockInventorySlotEntry slot : FoundSlots)
	{
		Slots.Add(slot.SlotHandle);
	}
	return Slots;
}
