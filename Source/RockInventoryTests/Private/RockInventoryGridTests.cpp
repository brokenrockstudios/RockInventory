// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Library/RockInventoryLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace
{
	FRockInventorySectionInfo MakeSection(int32 Columns, int32 Rows, ERockItemSizePolicy Policy = ERockItemSizePolicy::RespectSize)
	{
		FRockInventorySectionInfo Section(RockInventoryTags::Inventory_Section_Backpack, 0, Columns, Rows, Policy);
		Section.Initialize(0, 0);
		return Section;
	}

	TArray<bool> EmptyGrid(const FRockInventorySectionInfo& Section)
	{
		TArray<bool> Grid;
		Grid.Init(false, Section.GetNumSlots());
		return Grid;
	}
}

// The occupancy math behind every placement decision. Pure functions: no world needed.
TEST_CLASS(RockInventoryGridTests, "BRS.RockInventory.Grid")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(Fit_InsideAnEmptyGrid_IsTrue)
	{
		const FRockInventorySectionInfo Section = MakeSection(4, 3);
		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemFitInGridPosition(EmptyGrid(Section), Section, 1, 1, FVector2D(2, 2))));
	}

	TEST_METHOD(Fit_FlushAgainstTheFarEdges_IsTrue)
	{
		const FRockInventorySectionInfo Section = MakeSection(4, 3);
		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemFitInGridPosition(EmptyGrid(Section), Section, 2, 1, FVector2D(2, 2))));
	}

	TEST_METHOD(Fit_OverflowingRightOrBottom_IsFalse)
	{
		const FRockInventorySectionInfo Section = MakeSection(4, 3);
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(EmptyGrid(Section), Section, 3, 0, FVector2D(2, 1))));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(EmptyGrid(Section), Section, 0, 2, FVector2D(1, 2))));
	}

	TEST_METHOD(Fit_NegativeOrigin_IsFalse)
	{
		const FRockInventorySectionInfo Section = MakeSection(4, 3);
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(EmptyGrid(Section), Section, -1, 0, FVector2D(1, 1))));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(EmptyGrid(Section), Section, 0, -1, FVector2D(1, 1))));
	}

	TEST_METHOD(Fit_AnyOccupiedCellInTheFootprint_Blocks)
	{
		const FRockInventorySectionInfo Section = MakeSection(4, 3);
		TArray<bool> Grid = EmptyGrid(Section);
		Grid[1 * 4 + 1] = true; // (1,1)

		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, 0, 0, FVector2D(2, 2))));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, 1, 1, FVector2D(1, 1))));
		// The same item fits once it clears the occupied cell.
		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, 2, 0, FVector2D(2, 2))));
	}

	TEST_METHOD(Fit_IgnoreSizeSection_OnlyChecksTheTargetCell)
	{
		const FRockInventorySectionInfo Section = MakeSection(1, 1, ERockItemSizePolicy::IgnoreSize);
		TArray<bool> Grid = EmptyGrid(Section);

		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, 0, 0, FVector2D(3, 3))));
		Grid[0] = true;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, 0, 0, FVector2D(1, 1))));
	}

	TEST_METHOD(Fit_IgnoreSizeSection_OutOfRangeCellIsFalseInsteadOfWrapping)
	{
		const FRockInventorySectionInfo Section = MakeSection(3, 2, ERockItemSizePolicy::IgnoreSize);
		const TArray<bool> Grid = EmptyGrid(Section);

		// X == Columns would previously alias cell (0, 1).
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, 3, 0, FVector2D(1, 1))));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, -1, 1, FVector2D(1, 1))));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, 0, 2, FVector2D(1, 1))));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Section, 2, 1, FVector2D(1, 1))));
	}

	TEST_METHOD(Fit_UsesTheSectionsOffsetIntoTheGlobalGrid)
	{
		// Two sections share one occupancy array; the second must not read the first's cells.
		Fixture.Init({
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 1),
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Pockets, 0, 2, 1),
		});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0));
		TArray<bool> Grid;
		URockInventoryLibrary::PrecomputeOccupancyGrids(Fixture.Inventory, Grid);

		const FRockInventorySectionInfo& Pocket = Fixture.Inventory->GetSectionInfo(RockInventoryTags::Inventory_Section_Pockets);
		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemFitInGridPosition(Grid, Pocket, 0, 0, FVector2D(1, 1))));
	}

	TEST_METHOD(Occupancy_MarksTheFullFootprintOfAMultiCellItem)
	{
		Fixture.InitGrid(4, 4);
		URockItemDefinition* Crate = Fixture.MakeDefinition("Crate", 1, FIntPoint(2, 2));
		Fixture.PlaceAt(Crate, 1, Fixture.SlotAt(1, 1));

		TArray<bool> Grid;
		URockInventoryLibrary::PrecomputeOccupancyGrids(Fixture.Inventory, Grid);

		ASSERT_THAT(AreEqual(16, Grid.Num()));
		int32 Occupied = 0;
		for (const bool bCell : Grid) { Occupied += bCell ? 1 : 0; }
		ASSERT_THAT(AreEqual(4, Occupied));
		ASSERT_THAT(IsTrue(Grid[Fixture.SlotAt(1, 1).GetAbsoluteIndex()]));
		ASSERT_THAT(IsTrue(Grid[Fixture.SlotAt(2, 1).GetAbsoluteIndex()]));
		ASSERT_THAT(IsTrue(Grid[Fixture.SlotAt(1, 2).GetAbsoluteIndex()]));
		ASSERT_THAT(IsTrue(Grid[Fixture.SlotAt(2, 2).GetAbsoluteIndex()]));
		ASSERT_THAT(IsFalse(Grid[Fixture.SlotAt(0, 0).GetAbsoluteIndex()]));
	}

	TEST_METHOD(Occupancy_VerticalItem_MarksTheSwappedFootprint)
	{
		Fixture.InitGrid(4, 4);
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(3, 1));
		Fixture.PlaceAt(Rifle, 1, Fixture.SlotAt(0, 0));
		FRockInventorySlotEntry Entry = Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0));
		Entry.Orientation = ERockItemOrientation::Vertical;
		Fixture.Inventory->SetSlotByHandle(Fixture.SlotAt(0, 0), Entry);

		TArray<bool> Grid;
		URockInventoryLibrary::PrecomputeOccupancyGrids(Fixture.Inventory, Grid);

		ASSERT_THAT(AreEqual(3, Grid.FilterByPredicate([](bool bCell) { return bCell; }).Num()));
		ASSERT_THAT(IsTrue(Grid[Fixture.SlotAt(0, 0).GetAbsoluteIndex()]));
		ASSERT_THAT(IsTrue(Grid[Fixture.SlotAt(0, 1).GetAbsoluteIndex()]));
		ASSERT_THAT(IsTrue(Grid[Fixture.SlotAt(0, 2).GetAbsoluteIndex()]));
		ASSERT_THAT(IsFalse(Grid[Fixture.SlotAt(1, 0).GetAbsoluteIndex()]));
	}

	TEST_METHOD(Occupancy_AnIgnoredItemLeavesItsCellsFree)
	{
		Fixture.InitGrid(4, 4);
		URockItemDefinition* Crate = Fixture.MakeDefinition("Crate", 1, FIntPoint(2, 2));
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Crate, 1, Fixture.SlotAt(0, 0));

		TArray<bool> Grid;
		URockInventoryLibrary::PrecomputeOccupancyGrids(Fixture.Inventory, Grid, Handle);

		ASSERT_THAT(AreEqual(0, Grid.FilterByPredicate([](bool bCell) { return bCell; }).Num()));
	}

	TEST_METHOD(Occupancy_IgnoreSizeSection_MarksOnlyTheAnchorCell)
	{
		Fixture.InitGrid(3, 1, ERockItemSizePolicy::IgnoreSize);
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(2, 1));
		Fixture.PlaceAt(Rifle, 1, Fixture.SlotAt(0, 0));

		TArray<bool> Grid;
		URockInventoryLibrary::PrecomputeOccupancyGrids(Fixture.Inventory, Grid);

		ASSERT_THAT(IsTrue(Grid[0]));
		ASSERT_THAT(IsFalse(Grid[1]));
		ASSERT_THAT(IsFalse(Grid[2]));
	}
};

// ForEachSlot and the Find* helpers, driven by FRockInventoryQuery predicates.
TEST_CLASS(RockInventoryQueryTests, "BRS.RockInventory.Query")
{
	FRockInventoryFixture Fixture;

	void InitTwoSections()
	{
		Fixture.Init({
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 2),
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Pockets, 0, 2, 1),
		});
	}

	TEST_METHOD(ForEachSlot_SectionQuery_VisitsOnlyThatSection)
	{
		InitTwoSections();

		TArray<int32> Visited;
		bool bAllInPocket = true;
		Fixture.Inventory->ForEachSlot(
			FRockInventoryQuery::ForSectionWithSectionTag(RockInventoryTags::Inventory_Section_Pockets),
			[&](const FRockInventorySectionInfo* Section, const FRockInventorySlotEntry* Slot)
			{
				Visited.Add(Slot->SlotHandle.GetAbsoluteIndex());
				bAllInPocket &= Section->GetSectionTag() == RockInventoryTags::Inventory_Section_Pockets;
				return true;
			});

		ASSERT_THAT(AreEqual(2, Visited.Num()));
		ASSERT_THAT(IsTrue(bAllInPocket));
		ASSERT_THAT(AreEqual(4, Visited[0]));
		ASSERT_THAT(AreEqual(5, Visited[1]));
	}

	TEST_METHOD(ForEachSlot_VisitorReturningFalse_StopsEarly)
	{
		InitTwoSections();

		int32 Visits = 0;
		Fixture.Inventory->ForEachSlot(FRockInventoryQuery(), [&](const FRockInventorySectionInfo*, const FRockInventorySlotEntry*)
		{
			++Visits;
			return false;
		});

		ASSERT_THAT(AreEqual(1, Visits));
	}

	TEST_METHOD(ForEachSlot_EmptyQuery_VisitsEverySlot)
	{
		InitTwoSections();

		int32 Visits = 0;
		Fixture.Inventory->ForEachSlot(FRockInventoryQuery(), [&](const FRockInventorySectionInfo*, const FRockInventorySlotEntry*)
		{
			++Visits;
			return true;
		});

		ASSERT_THAT(AreEqual(6, Visits));
	}

	TEST_METHOD(ForEachSlot_ItemPredicate_SkipsEmptySlots)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(1, 0));

		FRockInventoryQuery Query;
		Query.ItemPredicate = [](const FRockItemStack*) { return true; };
		int32 Visits = 0;
		Fixture.Inventory->ForEachSlot(Query, [&](const FRockInventorySectionInfo*, const FRockInventorySlotEntry* Slot)
		{
			++Visits;
			return true;
		});

		ASSERT_THAT(AreEqual(1, Visits));
	}

	TEST_METHOD(FindFirstSlot_ReturnsTheFirstSlotOfTheMatchingSection)
	{
		InitTwoSections();

		const FRockInventorySlotEntry* Slot = Fixture.Inventory->FindFirstSlot(FRockInventoryQuery::ForSectionWithSectionTag(RockInventoryTags::Inventory_Section_Pockets));

		ASSERT_THAT(IsNotNull(Slot));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Slot->SlotHandle));
	}

	TEST_METHOD(FindFirstSlot_NothingMatches_ReturnsNull)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		ASSERT_THAT(IsNull(Fixture.Inventory->FindFirstSlot(FRockInventoryQuery::ForItemWithDefinition(Apple))));
	}

	TEST_METHOD(FindAllItemHandles_ByDefinition_ReturnsOnlyMatchingStacks)
	{
		Fixture.InitGrid(4, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		URockItemDefinition* Pear = Fixture.MakeDefinition("Pear");
		const FRockItemStackHandle AppleA = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Pear, 1, Fixture.SlotAt(1, 0));
		const FRockItemStackHandle AppleB = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(3, 0));

		const TArray<FRockItemStackHandle> Found = Fixture.Inventory->FindAllItemHandles(FRockInventoryQuery::ForItemWithDefinition(Apple));

		ASSERT_THAT(AreEqual(2, Found.Num()));
		ASSERT_THAT(IsTrue(Found.Contains(AppleA)));
		ASSERT_THAT(IsTrue(Found.Contains(AppleB)));
	}

	TEST_METHOD(FindAllItemHandles_SectionQuery_SkipsEmptySlots)
	{
		InitTwoSections();
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Held = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 1, 0));

		const TArray<FRockItemStackHandle> Found = Fixture.Inventory->FindAllItemHandles(FRockInventoryQuery::ForSectionWithSectionTag(RockInventoryTags::Inventory_Section_Backpack));

		ASSERT_THAT(AreEqual(1, Found.Num()));
		ASSERT_THAT(AreEqual(Held, Found[0]));
	}

	TEST_METHOD(FindAllItemHandles_EmptyInventory_ReturnsNothing)
	{
		InitTwoSections();

		ASSERT_THAT(AreEqual(0, Fixture.Inventory->FindAllItemHandles(FRockInventoryQuery()).Num()));
	}

	TEST_METHOD(FindAllSlots_LockedAndUnlocked_PartitionTheSlots)
	{
		Fixture.InitGrid(2, 2);
		FRockInventorySlotEntry Locked = Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(1, 0));
		Locked.bIsLocked = true;
		Fixture.Inventory->SetSlotByHandle(Fixture.SlotAt(1, 0), Locked);

		const TArray<FRockInventorySlotEntry> LockedSlots = Fixture.Inventory->FindAllSlots(FRockInventoryQuery::ForSlotLocked());
		const TArray<FRockInventorySlotEntry> UnlockedSlots = Fixture.Inventory->FindAllSlots(FRockInventoryQuery::ForSlotUnlocked());

		ASSERT_THAT(AreEqual(1, LockedSlots.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), LockedSlots[0].SlotHandle));
		ASSERT_THAT(AreEqual(3, UnlockedSlots.Num()));
	}

	TEST_METHOD(Query_AndSlot_NarrowsAnExistingQuery)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		const FRockItemStackHandle Open = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(2, 0));
		FRockInventorySlotEntry Locked = Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0));
		Locked.bIsLocked = true;
		Fixture.Inventory->SetSlotByHandle(Fixture.SlotAt(0, 0), Locked);

		FRockInventoryQuery Query = FRockInventoryQuery::ForItemWithDefinition(Apple);
		Query.AndSlot([](const FRockInventorySlotEntry* Slot) { return !Slot->bIsLocked; });
		const TArray<FRockItemStackHandle> Found = Fixture.Inventory->FindAllItemHandles(Query);

		ASSERT_THAT(AreEqual(1, Found.Num()));
		ASSERT_THAT(AreEqual(Open, Found[0]));
	}

	TEST_METHOD(Library_FindSlotsInSection_ReturnsTheSectionsSlotsOnly)
	{
		InitTwoSections();

		const TArray<FRockInventorySlotHandle> Main = URockInventoryLibrary::FindAllSlotsInSection(Fixture.Inventory, RockInventoryTags::Inventory_Section_Backpack);
		const TArray<FRockInventorySlotHandle> Pocket = URockInventoryLibrary::FindAllSlotsInSection(Fixture.Inventory, RockInventoryTags::Inventory_Section_Pockets);

		ASSERT_THAT(AreEqual(4, Main.Num()));
		ASSERT_THAT(AreEqual(2, Pocket.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), URockInventoryLibrary::FindFirstSlotInSection(Fixture.Inventory, RockInventoryTags::Inventory_Section_Pockets)));
	}

	TEST_METHOD(Library_FindSlotsInUnknownSection_ReturnsNothing)
	{
		Fixture.InitGrid(2, 1);

		ASSERT_THAT(IsFalse(URockInventoryLibrary::FindFirstSlotInSection(Fixture.Inventory, RockInventoryTags::Inventory_Section_Pockets).IsValid()));
		ASSERT_THAT(AreEqual(0, URockInventoryLibrary::FindAllSlotsInSection(Fixture.Inventory, RockInventoryTags::Inventory_Section_Pockets).Num()));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
