// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"
#include "RockInventoryTestTags.h"

#include "Library/RockInventoryLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// LootItemToInventory is the real "put this somewhere" path: merge into partial stacks, otherwise first slot where it fits.
TEST_CLASS(RockInventoryLootTests, "BRS.RockInventory.Loot")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(Loot_IntoEmptyInventory_PlacesInFirstSlot)
	{
		Fixture.InitGrid(3, 3);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(0, Excess));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Slot));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetItemBySlotHandle(Slot).GetStackCount()));
		ASSERT_THAT(IsTrue(Fixture.Inventory->GetItemBySlotHandle(Slot).GetDefinition() == Apple));
	}

	TEST_METHOD(Loot_MultiCellItems_TileWithoutOverlap)
	{
		Fixture.InitGrid(4, 4);
		URockItemDefinition* Crate = Fixture.MakeDefinition("Crate", 1, FIntPoint(2, 2));

		const FRockInventorySlotHandle Expected[] = {
			Fixture.SlotAt(0, 0), Fixture.SlotAt(2, 0), Fixture.SlotAt(0, 2), Fixture.SlotAt(2, 2)};
		for (const FRockInventorySlotHandle& ExpectedSlot : Expected)
		{
			FRockInventorySlotHandle Slot;
			int32 Excess = -1;
			ASSERT_THAT(IsTrue(Fixture.Loot(Crate, 1, Slot, Excess)));
			ASSERT_THAT(AreEqual(ExpectedSlot, Slot));
		}

		// The grid is now full of 2x2 crates; nothing else fits.
		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsFalse(Fixture.Loot(Crate, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(1, Excess));
		ASSERT_THAT(AreEqual(4, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Loot_SmallItemFillsGapNextToLargeItem)
	{
		Fixture.InitGrid(3, 2);
		URockItemDefinition* Crate = Fixture.MakeDefinition("Crate", 1, FIntPoint(2, 2));
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Crate, 1, Slot, Excess)));
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		// Columns 0-1 are covered by the crate, so the first free cell is column 2.
		ASSERT_THAT(AreEqual(Fixture.SlotAt(2, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(2, 1), Slot));
	}

	TEST_METHOD(Loot_ItemLargerThanSection_FailsAndReportsFullExcess)
	{
		Fixture.InitGrid(2, 2);
		URockItemDefinition* Plank = Fixture.MakeDefinition("Plank", 1, FIntPoint(3, 1));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsFalse(Fixture.Loot(Plank, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(1, Excess));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Loot_ItemOnlyFitsRotated_PlacesVertical)
	{
		// A 1-wide, 3-tall column: the 3x1 plank can only go in rotated.
		Fixture.InitGrid(1, 3);
		URockItemDefinition* Plank = Fixture.MakeDefinition("Plank", 1, FIntPoint(3, 1));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Plank, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(0, Excess));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Inventory->GetSlotByHandle(Slot).Orientation == ERockItemOrientation::Vertical));
	}

	TEST_METHOD(Loot_ItemFitsUnrotated_StaysHorizontal)
	{
		Fixture.InitGrid(3, 3);
		URockItemDefinition* Plank = Fixture.MakeDefinition("Plank", 1, FIntPoint(3, 1));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Plank, 1, Slot, Excess)));
		ASSERT_THAT(IsTrue(Fixture.Inventory->GetSlotByHandle(Slot).Orientation == ERockItemOrientation::Horizontal));
	}

	TEST_METHOD(Loot_RotatedPlacement_BlocksTheRotatedFootprint)
	{
		Fixture.InitGrid(1, 3);
		URockItemDefinition* Plank = Fixture.MakeDefinition("Plank", 1, FIntPoint(3, 1));
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Plank, 1, Slot, Excess)));
		// The rotated plank covers the whole column, so there is no room left.
		ASSERT_THAT(IsFalse(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(1, Excess));
	}

	TEST_METHOD(Loot_StackableItem_FillsPartialStackThenSpillsToNextSlot)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 6, Slot, Excess)));
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 6, Slot, Excess)));

		ASSERT_THAT(AreEqual(0, Excess));
		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(1, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(12, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Arrow")));
	}

	TEST_METHOD(Loot_FullInventory_ReturnsRemainingAsExcess)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(0, 0));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsFalse(Fixture.Loot(Arrow, 3, Slot, Excess)));
		ASSERT_THAT(AreEqual(3, Excess));
		ASSERT_THAT(AreEqual(5, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
	}

	TEST_METHOD(Loot_PartiallyFits_ConsumesWhatFitsAndReportsTheRest)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsFalse(Fixture.Loot(Arrow, 4, Slot, Excess)));
		ASSERT_THAT(AreEqual(2, Excess));
		ASSERT_THAT(AreEqual(5, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
	}

	TEST_METHOD(Loot_StacksWithDifferentCustomValues_DoNotMerge)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Battery = Fixture.MakeDefinition("Battery", 10);
		const FRockItemStackHandle Charged = Fixture.PlaceAt(Battery, 1, Fixture.SlotAt(0, 0));
		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, Charged, 50);

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Battery, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Slot));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
	}

	TEST_METHOD(Loot_SkipsSlotsWithAPendingOperation)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.Inventory->RegisterSlotStatus(nullptr, Fixture.SlotAt(0, 0), ERockSlotStatus::Pending);

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Slot));
	}

	TEST_METHOD(Loot_IgnoreSizeSection_AcceptsOversizedItemInOneCell)
	{
		// Equipment-style sections ignore the item's footprint and treat everything as 1x1.
		Fixture.InitGrid(1, 1, ERockItemSizePolicy::IgnoreSize);
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(5, 2));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Rifle, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Slot));
	}

	TEST_METHOD(Loot_InvalidStack_FailsWithoutTouchingInventory)
	{
		Fixture.InitGrid(2, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid Parameters. ItemStack"), ELogVerbosity::Warning);

		FRockLootResult Result;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack::Invalid(), FRockLootParams(), Result)));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Loot_NullInventory_FailsAndWarns)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid Parameters. Inventory"), ELogVerbosity::Warning);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockLootResult Result;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::LootItemToInventory(nullptr, FRockItemStack(Apple, 2), FRockLootParams(), Result)));
		const int32 Excess = Result.Excess;
		ASSERT_THAT(AreEqual(2, Excess));
	}
};

// LootItemToInventory across several sections: which section wins, what the section filter gates, merge versus empty slot.
// These pin the CURRENT behavior (T-33) so the section-major refactor (T-34) and the priority feature (T-36) have a baseline.
// T-36 changed the two merge-versus-empty-slot rules to merge first; the Merge_Later* tests pin that.
TEST_CLASS(RockInventoryLootSectionTests, "BRS.RockInventory.Loot.Sections")
{
	FRockInventoryFixture Fixture;

	FGameplayTagContainer Tags(const FGameplayTag& A) const { return FGameplayTagContainer(A); }

	TEST_METHOD(Placement_FollowsConfigOrder_FirstSectionFillsFirst)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 3, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		const FRockInventorySlotHandle Expected[] = {
			Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0),
			Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 1, 0),
			Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0)};
		for (const FRockInventorySlotHandle& ExpectedSlot : Expected)
		{
			FRockInventorySlotHandle Slot;
			int32 Excess = -1;
			ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
			ASSERT_THAT(AreEqual(ExpectedSlot, Slot));
		}
	}

	TEST_METHOD(Placement_ConfigOrderIsThePriority_NotTheSectionTag)
	{
		// Same sections as above in the opposite order: Backpack is now first and wins.
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Slot));
	}

	TEST_METHOD(Placement_FullFirstSection_SpillsIntoTheNext)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
	}

	TEST_METHOD(Placement_PendingSlotInFirstSection_FallsToTheNextSection)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.Inventory->RegisterSlotStatus(nullptr, Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), ERockSlotStatus::Pending);

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
	}

	TEST_METHOD(Placement_SizePolicyIsPerSection)
	{
		// An IgnoreSize section takes the oversized rifle in one cell; once it is taken the second rifle needs the real footprint in the next section.
		FRockInventorySectionInfo Equipment(RockInventoryTags::Inventory_Section_Pockets, 0, 1, 1, ERockItemSizePolicy::IgnoreSize);
		Fixture.Init({Equipment, FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 5, 2)});
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(5, 2));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Rifle, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Loot(Rifle, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
		ASSERT_THAT(IsFalse(Fixture.Loot(Rifle, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(1, Excess));
	}

	TEST_METHOD(Filter_SectionNotAcceptingTheItem_IsSkipped)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1, Tags(RockInventoryTestTags::Weapon)),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 2, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		URockItemDefinition* Sword = Fixture.MakeTaggedDefinition("Sword", Tags(RockInventoryTestTags::Weapon));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		// The filtered section is first in config order, but the apple does not carry the tag.
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Loot(Sword, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Slot));
	}

	TEST_METHOD(Filter_MatchesAnyOfTheRequiredTags)
	{
		FGameplayTagContainer WeaponOrFood;
		WeaponOrFood.AddTag(RockInventoryTestTags::Weapon);
		WeaponOrFood.AddTag(RockInventoryTestTags::Food);
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1, WeaponOrFood),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Sword = Fixture.MakeTaggedDefinition("Sword", Tags(RockInventoryTestTags::Weapon));
		URockItemDefinition* Apple = Fixture.MakeTaggedDefinition("Apple", Tags(RockInventoryTestTags::Food));
		URockItemDefinition* Rock = Fixture.MakeDefinition("Rock");

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		// The untagged rock goes first, while both Pockets slots are still free, so only the filter keeps it out.
		ASSERT_THAT(IsTrue(Fixture.Loot(Rock, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Loot(Sword, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 1, 0), Slot));
	}

	TEST_METHOD(Filter_ExcludeQuery_RejectsOnlyTheExcludedTag)
	{
		FRockInventorySectionInfo NoWeapons(RockInventoryTags::Inventory_Section_Pockets, 0, 2, 1);
		NoWeapons.SetSectionFilter(FGameplayTagQuery::MakeQuery_MatchNoTags(Tags(RockInventoryTestTags::Weapon)));
		Fixture.Init({NoWeapons, FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Sword = Fixture.MakeTaggedDefinition("Sword", Tags(RockInventoryTestTags::Weapon));
		URockItemDefinition* Rock = Fixture.MakeDefinition("Rock");

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Sword, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Loot(Rock, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Slot));
	}

	TEST_METHOD(Filter_NoSectionAccepts_FailsWithFullExcessAndStoresNothing)
	{
		Fixture.Init({FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1, Tags(RockInventoryTestTags::Weapon))});
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsFalse(Fixture.Loot(Arrow, 4, Slot, Excess)));
		ASSERT_THAT(AreEqual(4, Excess));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Filter_EmptyFilter_AcceptsTaggedAndUntaggedItems)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Sword = Fixture.MakeTaggedDefinition("Sword", Tags(RockInventoryTestTags::Weapon));
		URockItemDefinition* Rock = Fixture.MakeDefinition("Rock");

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Sword, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Slot));
		ASSERT_THAT(IsTrue(Fixture.Loot(Rock, 1, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 1, 0), Slot));
	}

	TEST_METHOD(Filter_AlsoGatesMerging_ASectionThatRejectsTheItemIsNotToppedUp)
	{
		// The partial stack of untagged arrows sits in a Weapon-only section (arranged with PlaceAt, which bypasses the filter).
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1, Tags(RockInventoryTestTags::Weapon)),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockInventorySlotHandle PocketSlot = Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0);
		Fixture.PlaceAt(Arrow, 5, PocketSlot);

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 3, Slot, Excess)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
		ASSERT_THAT(AreEqual(5, Fixture.Inventory->GetItemBySlotHandle(PocketSlot).GetStackCount()));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemBySlotHandle(Slot).GetStackCount()));
	}

	TEST_METHOD(Merge_EarlierPartialStack_IsToppedUpBeforeAnyEmptySlot)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(0, 0));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 3, Slot, Excess)));
		ASSERT_THAT(AreEqual(8, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Merge_LaterPartialStack_IsToppedUpBeforeAnEarlierEmptySlot)
	{
		// Merge first (T-36): the partial stack in slot 2 takes the whole incoming stack, so slot 0 stays empty.
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(2, 0));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 3, Slot, Excess)));
		ASSERT_THAT(AreEqual(0, Excess));
		ASSERT_THAT(AreEqual(8, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(2, 0)).GetStackCount()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).IsValid()));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Merge_LaterPartialStackOverflow_FillsItThenStartsANewStackInTheEarlierEmptySlot)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(2, 0));

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 8, Slot, Excess)));
		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(2, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Slot));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
	}

	TEST_METHOD(Merge_LaterSectionsPartialStack_IsToppedUpBeforeAnEarlierSectionsEmptySlot)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockInventorySlotHandle BackpackSlot = Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0);
		Fixture.PlaceAt(Arrow, 5, BackpackSlot);

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 3, Slot, Excess)));
		ASSERT_THAT(AreEqual(8, Fixture.Inventory->GetItemBySlotHandle(BackpackSlot).GetStackCount()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0)).IsValid()));
	}

	TEST_METHOD(Merge_OverflowOfAFullerSection_SpillsIntoTheNextSection)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockInventorySlotHandle PocketSlot = Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0);
		Fixture.PlaceAt(Arrow, 8, PocketSlot);

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 5, Slot, Excess)));
		ASSERT_THAT(AreEqual(0, Excess));
		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemBySlotHandle(PocketSlot).GetStackCount()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Slot));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemBySlotHandle(Slot).GetStackCount()));
	}
};

// The merge helpers LootItemToInventory and MoveItem are built on.
TEST_CLASS(RockInventoryMergeTests, "BRS.RockInventory.Merge")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(CanMerge_Full_NeedsRoomForTheWholeIncomingStack)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 6, Fixture.SlotAt(0, 0));

		const FRockInventorySlotHandle Slot = Fixture.SlotAt(0, 0);
		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanMergeItemAtGridPosition(Fixture.Inventory, Slot, FRockItemStack(Arrow, 4), ERockItemStackMergeCondition::Full)));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanMergeItemAtGridPosition(Fixture.Inventory, Slot, FRockItemStack(Arrow, 5), ERockItemStackMergeCondition::Full)));
	}

	TEST_METHOD(CanMerge_Partial_NeedsAnyRoomAtAll)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 6, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Arrow, 10, Fixture.SlotAt(1, 0));

		const FRockItemStack Incoming(Arrow, 5);
		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanMergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(0, 0), Incoming, ERockItemStackMergeCondition::Partial)));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanMergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(1, 0), Incoming, ERockItemStackMergeCondition::Partial)));
	}

	TEST_METHOD(CanMerge_None_IsTrueOnlyWhenTheExistingStackHasNoRoom)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 6, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Arrow, 10, Fixture.SlotAt(1, 0));

		const FRockItemStack Incoming(Arrow, 1);
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanMergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(0, 0), Incoming, ERockItemStackMergeCondition::None)));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanMergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(1, 0), Incoming, ERockItemStackMergeCondition::None)));
	}

	TEST_METHOD(CanMerge_DifferentDefinitions_IsFalse)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		URockItemDefinition* Bolt = Fixture.MakeDefinition("Bolt", 10);
		Fixture.PlaceAt(Arrow, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanMergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Bolt, 1), ERockItemStackMergeCondition::Partial)));
	}

	TEST_METHOD(CanMerge_EmptySlot_IsFalse)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanMergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Arrow, 1), ERockItemStackMergeCondition::Partial)));
	}

	TEST_METHOD(Merge_OverflowingMax_CapsStackAndReturnsExcess)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 8, Fixture.SlotAt(0, 0));

		const int32 Excess = URockInventoryLibrary::MergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Arrow, 5));
		ASSERT_THAT(AreEqual(3, Excess));
		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
	}

	TEST_METHOD(Merge_FittingExactly_LeavesNoExcess)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 8, Fixture.SlotAt(0, 0));

		const int32 Excess = URockInventoryLibrary::MergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Arrow, 2));
		ASSERT_THAT(AreEqual(0, Excess));
		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
	}

	TEST_METHOD(Merge_IntoEmptySlot_ReturnsWholeStackUntouched)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		const int32 Excess = URockInventoryLibrary::MergeItemAtGridPosition(Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Arrow, 5));
		ASSERT_THAT(AreEqual(5, Excess));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}
};

// The loot API shape (T-35): FRockLootParams in, FRockLootResult out, PreviewLoot as the read-only twin of the real call.
TEST_CLASS(RockInventoryLootApiTests, "BRS.RockInventory.Loot.Api")
{
	FRockInventoryFixture Fixture;

	static bool SamePlacement(const FRockLootPlacement& A, const FRockLootPlacement& B)
	{
		return A.Slot.GetInventory() == B.Slot.GetInventory() && A.Slot.GetSlotHandle() == B.Slot.GetSlotHandle() && A.Count == B.Count
			&& A.Orientation == B.Orientation && A.bNewStack == B.bNewStack;
	}

	// Previews, then loots, and checks the two agree placement for placement.
	bool PreviewMatchesRealCall(URockItemDefinition* Definition, int32 Count, const FRockLootParams& Params, FRockLootResult& OutReal)
	{
		const FRockItemStack Stack(Definition, Count);
		const FRockLootResult Preview = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, Stack, Params);
		URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, Stack, Params, OutReal);
		if (Preview.Excess != OutReal.Excess || Preview.Placements.Num() != OutReal.Placements.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Preview.Placements.Num(); ++Index)
		{
			if (!SamePlacement(Preview.Placements[Index], OutReal.Placements[Index]))
			{
				return false;
			}
		}
		return true;
	}

	TEST_METHOD(Loot_IntoEmptyInventory_ReportsOneNewStackPlacement)
	{
		Fixture.InitGrid(3, 3);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);

		FRockLootResult Result;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Apple, 3), FRockLootParams(), Result)));

		ASSERT_THAT(AreEqual(0, Result.Excess));
		ASSERT_THAT(IsTrue(Result.IsFullyPlaced()));
		ASSERT_THAT(AreEqual(1, Result.Placements.Num()));
		ASSERT_THAT(AreEqual(3, Result.GetPlacedCount()));
		const FRockLootPlacement& Placement = Result.Placements[0];
		ASSERT_THAT(IsTrue(Placement.bNewStack));
		ASSERT_THAT(AreEqual(3, Placement.Count));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Placement.Slot.GetSlotHandle()));
		ASSERT_THAT(IsTrue(Placement.Slot.GetInventory() == Fixture.Inventory));
		ASSERT_THAT(IsTrue(Result.FindNewStack() == &Result.Placements[0]));
	}

	TEST_METHOD(Loot_MergesThenSpills_ReportsTheMergeBeforeTheNewStack)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));

		FRockLootResult Result;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Arrow, 5), FRockLootParams(), Result)));

		ASSERT_THAT(AreEqual(2, Result.Placements.Num()));
		ASSERT_THAT(IsFalse(Result.Placements[0].bNewStack));
		ASSERT_THAT(AreEqual(2, Result.Placements[0].Count));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Result.Placements[0].Slot.GetSlotHandle()));
		ASSERT_THAT(IsTrue(Result.Placements[1].bNewStack));
		ASSERT_THAT(AreEqual(3, Result.Placements[1].Count));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Result.Placements[1].Slot.GetSlotHandle()));
		ASSERT_THAT(AreEqual(5, Result.GetPlacedCount()));
	}

	TEST_METHOD(Loot_OnlyMerges_HasNoNewStack)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		Fixture.PlaceAt(Arrow, 2, Fixture.SlotAt(0, 0));

		FRockLootResult Result;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Arrow, 2), FRockLootParams(), Result)));

		ASSERT_THAT(AreEqual(1, Result.Placements.Num()));
		ASSERT_THAT(IsTrue(Result.FindNewStack() == nullptr));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Loot_Rotated_ReportsTheChosenOrientation)
	{
		Fixture.InitGrid(1, 3);
		URockItemDefinition* Plank = Fixture.MakeDefinition("Plank", 1, FIntPoint(3, 1));

		FRockLootResult Result;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Plank, 1), FRockLootParams(), Result)));

		ASSERT_THAT(AreEqual(1, Result.Placements.Num()));
		ASSERT_THAT(IsTrue(Result.Placements[0].Orientation == ERockItemOrientation::Vertical));
	}

	TEST_METHOD(Loot_NoRoom_ReportsFullExcessAndNoPlacements)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(0, 0));

		FRockLootResult Result;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Arrow, 3), FRockLootParams(), Result)));

		ASSERT_THAT(AreEqual(3, Result.Excess));
		ASSERT_THAT(IsTrue(Result.Placements.IsEmpty()));
		ASSERT_THAT(AreEqual(0, Result.GetPlacedCount()));
	}

	TEST_METHOD(Loot_ResultIsResetOnEveryCall)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockLootResult Result;
		URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Apple, 1), FRockLootParams(), Result);
		URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Apple, 1), FRockLootParams(), Result);

		ASSERT_THAT(AreEqual(1, Result.Placements.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Result.Placements[0].Slot.GetSlotHandle()));
	}

	TEST_METHOD(Preview_ChangesNothing)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));

		const FRockLootResult Preview = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, FRockItemStack(Arrow, 5), FRockLootParams());

		ASSERT_THAT(AreEqual(2, Preview.Placements.Num()));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(3, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Arrow")));
	}

	TEST_METHOD(Preview_MatchesTheRealCall_EmptyInventory)
	{
		Fixture.InitGrid(3, 3);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockLootResult Real;
		ASSERT_THAT(IsTrue(PreviewMatchesRealCall(Apple, 1, FRockLootParams(), Real)));
	}

	TEST_METHOD(Preview_MatchesTheRealCall_MergeThenNewStack)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));

		FRockLootResult Real;
		ASSERT_THAT(IsTrue(PreviewMatchesRealCall(Arrow, 5, FRockLootParams(), Real)));
		ASSERT_THAT(AreEqual(2, Real.Placements.Num()));
	}

	TEST_METHOD(Preview_MatchesTheRealCall_RotatedAndPartial)
	{
		// A 1-wide column fits the rotated plank once; the second plank is excess.
		Fixture.InitGrid(1, 3);
		URockItemDefinition* Plank = Fixture.MakeDefinition("Plank", 1, FIntPoint(3, 1));

		FRockLootResult First;
		ASSERT_THAT(IsTrue(PreviewMatchesRealCall(Plank, 1, FRockLootParams(), First)));
		FRockLootResult Second;
		ASSERT_THAT(IsTrue(PreviewMatchesRealCall(Plank, 1, FRockLootParams(), Second)));
		ASSERT_THAT(AreEqual(1, Second.Excess));
	}

	TEST_METHOD(Preview_NoRoom_ReportsExcess)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(0, 0));

		const FRockLootResult Preview = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, FRockItemStack(Arrow, 3), FRockLootParams());

		ASSERT_THAT(AreEqual(3, Preview.Excess));
		ASSERT_THAT(IsTrue(Preview.Placements.IsEmpty()));
	}

	TEST_METHOD(Preview_InvalidInput_ReturnsFullExcessWithoutWarning)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		const FRockLootResult NullInventory = URockInventoryLibrary::PreviewLoot(nullptr, FRockItemStack(Apple, 2), FRockLootParams());
		const FRockLootResult InvalidStack = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, FRockItemStack::Invalid(), FRockLootParams());

		ASSERT_THAT(AreEqual(2, NullInventory.Excess));
		ASSERT_THAT(IsTrue(NullInventory.Placements.IsEmpty()));
		ASSERT_THAT(IsTrue(InvalidStack.Placements.IsEmpty()));
	}

	TEST_METHOD(Preview_OnAClient_StillWorks)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.Owner->SetRole(ROLE_SimulatedProxy);
		ASSERT_THAT(IsFalse(Fixture.Owner->HasAuthority()));

		const FRockLootResult Preview = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, FRockItemStack(Apple, 1), FRockLootParams());

		ASSERT_THAT(AreEqual(1, Preview.Placements.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Preview.Placements[0].Slot.GetSlotHandle()));
	}

	TEST_METHOD(ExcludeSectionMetaTags_SkipsTheSection_PreviewAndRealCallAgree)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1, FGameplayTagContainer(), FGameplayTagContainer(RockInventoryTestTags::MetaA)),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 2, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		FRockLootParams Params;
		Params.ExcludeSectionMetaTags.AddTag(RockInventoryTestTags::MetaA);

		FRockLootResult Real;
		ASSERT_THAT(IsTrue(PreviewMatchesRealCall(Apple, 1, Params, Real)));

		ASSERT_THAT(AreEqual(1, Real.Placements.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Real.Placements[0].Slot.GetSlotHandle()));
	}

	TEST_METHOD(ExcludeSectionMetaTags_NotSet_UsesTheFirstSectionAsBefore)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1, FGameplayTagContainer(), FGameplayTagContainer(RockInventoryTestTags::MetaA)),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 2, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockLootResult Real;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Apple, 1), FRockLootParams(), Real)));

		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Real.Placements[0].Slot.GetSlotHandle()));
	}

	TEST_METHOD(ExcludeSectionMetaTags_DoesNotMergeIntoAnExcludedSection)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1, FGameplayTagContainer(), FGameplayTagContainer(RockInventoryTestTags::MetaA)),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 5);
		const FRockInventorySlotHandle PocketSlot = Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0);
		Fixture.PlaceAt(Arrow, 2, PocketSlot);
		FRockLootParams Params;
		Params.ExcludeSectionMetaTags.AddTag(RockInventoryTestTags::MetaA);

		FRockLootResult Real;
		ASSERT_THAT(IsTrue(PreviewMatchesRealCall(Arrow, 2, Params, Real)));

		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetItemBySlotHandle(PocketSlot).GetStackCount()));
		ASSERT_THAT(AreEqual(1, Real.Placements.Num()));
		ASSERT_THAT(IsTrue(Real.Placements[0].bNewStack));
	}

	TEST_METHOD(ExcludeSectionMetaTags_EverySectionExcluded_PlacesNothing)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1, FGameplayTagContainer(), FGameplayTagContainer(RockInventoryTestTags::MetaA))});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		FRockLootParams Params;
		Params.ExcludeSectionMetaTags.AddTag(RockInventoryTestTags::MetaA);

		FRockLootResult Real;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Apple, 1), Params, Real)));

		ASSERT_THAT(AreEqual(1, Real.Excess));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(ExcludeSectionMetaTags_AnyOfSeveralTagsExcludes)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1, FGameplayTagContainer(), FGameplayTagContainer(RockInventoryTestTags::MetaB)),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		FRockLootParams Params;
		Params.ExcludeSectionMetaTags.AddTag(RockInventoryTestTags::MetaA);
		Params.ExcludeSectionMetaTags.AddTag(RockInventoryTestTags::MetaB);

		const FRockLootResult Preview = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, FRockItemStack(Apple, 1), Params);

		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), Preview.Placements[0].Slot.GetSlotHandle()));
	}

	// Sections accept Store by default, so an Equip-only call finds no section (T-36). Per-section behavior is in Loot.Priority.
	TEST_METHOD(Intent_EquipOnlyCall_FindsNoDefaultSection_PreviewAndRealCallAgree)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		FRockLootParams Params;
		Params.Intent = static_cast<int32>(ERockLootIntent::Equip);
		Params.bAllowSwap = true;

		FRockLootResult Real;
		ASSERT_THAT(IsTrue(PreviewMatchesRealCall(Apple, 1, Params, Real)));

		ASSERT_THAT(IsTrue(Real.Placements.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Real.Excess));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Params_DefaultsAreAPlainPickup)
	{
		const FRockLootParams Params;

		ASSERT_THAT(IsTrue(Params.HasIntent(ERockLootIntent::Store)));
		ASSERT_THAT(IsTrue(Params.HasIntent(ERockLootIntent::Equip)));
		ASSERT_THAT(IsFalse(Params.bAllowSwap));
		ASSERT_THAT(IsTrue(Params.ExcludeSectionMetaTags.IsEmpty()));
	}

	TEST_METHOD(Params_HasIntent_ReadsTheFlags)
	{
		FRockLootParams Params;
		Params.Intent = static_cast<int32>(ERockLootIntent::Store);

		ASSERT_THAT(IsTrue(Params.HasIntent(ERockLootIntent::Store)));
		ASSERT_THAT(IsFalse(Params.HasIntent(ERockLootIntent::Equip)));
	}
};

// Section placement rules (T-36): AcceptedLootIntents, LootPriority, LootPreference and the merge-then-fill passes.
// Layout "Player": Head (Equip, headgear), Primary (Equip, weapon or wieldable, prefers a non-sidearm weapon),
// Secondary (Equip, weapon, prefers a sidearm, priority 1), Backpack (Store, priority 10). All equipment sections are 1x1.
TEST_CLASS(RockInventoryLootPriorityTests, "BRS.RockInventory.Loot.Priority")
{
	FRockInventoryFixture Fixture;

	static FGameplayTagContainer Tags(const FGameplayTag& A) { return FGameplayTagContainer(A); }
	static FGameplayTagContainer Tags(const FGameplayTag& A, const FGameplayTag& B) { FGameplayTagContainer C(A); C.AddTag(B); return C; }

	static int32 AsInt(ERockLootIntent Intent) { return static_cast<int32>(Intent); }

	static FGameplayTagQuery WeaponButNotSidearm()
	{
		using Expr = FGameplayTagQueryExpression;
		return FGameplayTagQuery::BuildQuery(
			Expr().AllExprMatch()
				.AddExpr(Expr().AnyTagsMatch().AddTag(RockInventoryTestTags::Weapon))
				.AddExpr(Expr().NoTagsMatch().AddTag(RockInventoryTestTags::Sidearm)));
	}

	void InitPlayer(int32 BackpackColumns = 4)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTestTags::SectionHead, 1, 1, Tags(RockInventoryTestTags::Headgear))
				.SetAcceptedLootIntents(ERockLootIntent::Equip),
			FRockInventoryFixture::MakeSection(RockInventoryTestTags::SectionPrimary, 1, 1, Tags(RockInventoryTestTags::Weapon, RockInventoryTestTags::Wieldable))
				.SetAcceptedLootIntents(ERockLootIntent::Equip)
				.SetLootPreference(WeaponButNotSidearm()),
			FRockInventoryFixture::MakeSection(RockInventoryTestTags::SectionSecondary, 1, 1, Tags(RockInventoryTestTags::Weapon))
				.SetAcceptedLootIntents(ERockLootIntent::Equip)
				.SetLootPriority(1)
				.SetLootPreference(FGameplayTagQuery::MakeQuery_MatchAnyTags(Tags(RockInventoryTestTags::Sidearm))),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, BackpackColumns, 1)
				.SetLootPriority(10)});
	}

	FRockInventorySlotHandle HeadSlot() const { return Fixture.SlotAt(RockInventoryTestTags::SectionHead, 0, 0); }
	FRockInventorySlotHandle PrimarySlot() const { return Fixture.SlotAt(RockInventoryTestTags::SectionPrimary, 0, 0); }
	FRockInventorySlotHandle SecondarySlot() const { return Fixture.SlotAt(RockInventoryTestTags::SectionSecondary, 0, 0); }
	FRockInventorySlotHandle BackpackSlot(int32 Column) const { return Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, Column, 0); }

	URockItemDefinition* Sword(FName Id = "Sword") { return Fixture.MakeTaggedDefinition(Id, Tags(RockInventoryTestTags::Weapon)); }
	URockItemDefinition* Pistol(FName Id = "Pistol") { return Fixture.MakeTaggedDefinition(Id, Tags(RockInventoryTestTags::Weapon, RockInventoryTestTags::Sidearm)); }
	URockItemDefinition* Pot(FName Id = "Pot") { return Fixture.MakeTaggedDefinition(Id, Tags(RockInventoryTestTags::Headgear, RockInventoryTestTags::Wieldable)); }

	// Previews, loots, checks the two agree on the slot, and returns the slot of the new stack (invalid when nothing was placed).
	FRockInventorySlotHandle LootWith(URockItemDefinition* Definition, ERockLootIntent Intent, int32 Count = 1)
	{
		FRockLootParams Params;
		Params.Intent = AsInt(Intent);
		const FRockItemStack Stack(Definition, Count);
		const FRockLootResult Preview = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, Stack, Params);
		FRockLootResult Real;
		URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, Stack, Params, Real);
		bool bAgree = Preview.Placements.Num() == Real.Placements.Num() && Preview.Excess == Real.Excess;
		for (int32 Index = 0; bAgree && Index < Real.Placements.Num(); ++Index)
		{
			bAgree = Preview.Placements[Index].Slot.GetSlotHandle() == Real.Placements[Index].Slot.GetSlotHandle();
		}
		if (!bAgree)
		{
			AddError(TEXT("PreviewLoot and LootItemToInventory disagree"));
		}
		const FRockLootPlacement* NewStack = Real.FindNewStack();
		return NewStack ? NewStack->Slot.GetSlotHandle() : FRockInventorySlotHandle();
	}

	TEST_METHOD(Sword_StoreAndEquip_GoesPrimaryThenSecondaryThenBackpack)
	{
		InitPlayer();
		const ERockLootIntent Both = ERockLootIntent::Store | ERockLootIntent::Equip;

		ASSERT_THAT(AreEqual(PrimarySlot(), LootWith(Sword("S1"), Both)));
		// Primary is occupied, Secondary only allows the weapon (it prefers sidearms), the backpack is last by priority
		ASSERT_THAT(AreEqual(SecondarySlot(), LootWith(Sword("S2"), Both)));
		ASSERT_THAT(AreEqual(BackpackSlot(0), LootWith(Sword("S3"), Both)));
	}

	TEST_METHOD(Pistol_PrefersSecondaryOverPrimary)
	{
		InitPlayer();
		const ERockLootIntent Both = ERockLootIntent::Store | ERockLootIntent::Equip;

		ASSERT_THAT(AreEqual(SecondarySlot(), LootWith(Pistol("P1"), Both)));
		// Secondary is taken, so the pistol falls through to Primary (allowed, just not preferred)
		ASSERT_THAT(AreEqual(PrimarySlot(), LootWith(Pistol("P2"), Both)));
		ASSERT_THAT(AreEqual(BackpackSlot(0), LootWith(Pistol("P3"), Both)));
	}

	TEST_METHOD(Pot_GoesHeadThenPrimaryThenBackpack)
	{
		InitPlayer();
		const ERockLootIntent Both = ERockLootIntent::Store | ERockLootIntent::Equip;

		ASSERT_THAT(AreEqual(HeadSlot(), LootWith(Pot("Pot1"), Both)));
		ASSERT_THAT(AreEqual(PrimarySlot(), LootWith(Pot("Pot2"), Both)));
		// The Secondary filter wants a weapon, so the third pot goes to storage
		ASSERT_THAT(AreEqual(BackpackSlot(0), LootWith(Pot("Pot3"), Both)));
	}

	TEST_METHOD(Store_NeverLandsInASectionThatAcceptsOnlyEquip)
	{
		InitPlayer();

		ASSERT_THAT(AreEqual(BackpackSlot(0), LootWith(Sword("S1"), ERockLootIntent::Store)));
		ASSERT_THAT(AreEqual(BackpackSlot(1), LootWith(Pot("Pot1"), ERockLootIntent::Store)));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemBySlotHandle(PrimarySlot()).IsValid()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemBySlotHandle(HeadSlot()).IsValid()));
	}

	TEST_METHOD(Equip_NeverLandsInStorage_EvenWhenEquipmentIsFull)
	{
		InitPlayer();

		ASSERT_THAT(AreEqual(PrimarySlot(), LootWith(Sword("S1"), ERockLootIntent::Equip)));
		ASSERT_THAT(AreEqual(SecondarySlot(), LootWith(Sword("S2"), ERockLootIntent::Equip)));
		// Both weapon slots are taken: refused, and nothing reaches the backpack
		ASSERT_THAT(IsFalse(LootWith(Sword("S3"), ERockLootIntent::Equip).IsValid()));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetNumItemStacks()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemBySlotHandle(BackpackSlot(0)).IsValid()));
	}

	TEST_METHOD(Equip_ItemNoEquipmentSectionAccepts_IsRefused)
	{
		InitPlayer();
		URockItemDefinition* Apple = Fixture.MakeTaggedDefinition("Apple", Tags(RockInventoryTestTags::Food));
		FRockLootParams Params;
		Params.Intent = AsInt(ERockLootIntent::Equip);

		FRockLootResult Result;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Apple, 1), Params, Result)));
		ASSERT_THAT(IsTrue(Result.Placements.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Result.Excess));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Apple_StoreOrBoth_GoesToTheBackpack)
	{
		InitPlayer();
		URockItemDefinition* Apple = Fixture.MakeTaggedDefinition("Apple", Tags(RockInventoryTestTags::Food));

		ASSERT_THAT(AreEqual(BackpackSlot(0), LootWith(Apple, ERockLootIntent::Store)));
		ASSERT_THAT(AreEqual(BackpackSlot(1), LootWith(Fixture.MakeTaggedDefinition("Apple2", Tags(RockInventoryTestTags::Food)), ERockLootIntent::Store | ERockLootIntent::Equip)));
	}

	TEST_METHOD(OccupiedPrimary_FallsThroughToTheNextSection)
	{
		InitPlayer();
		Fixture.PlaceAt(Sword("Held"), 1, PrimarySlot());

		ASSERT_THAT(AreEqual(SecondarySlot(), LootWith(Sword("S2"), ERockLootIntent::Equip)));
	}

	TEST_METHOD(SectionAcceptingNoIntent_IsNeverAutoFilled)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Storage, 2, 1).SetAcceptedLootIntents(ERockLootIntent::None),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 2, 1)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		ASSERT_THAT(AreEqual(BackpackSlot(0), LootWith(Apple, ERockLootIntent::Store | ERockLootIntent::Equip)));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Storage, 0, 0)).IsValid()));
	}

	TEST_METHOD(SectionAcceptingBothIntents_TakesEitherCall)
	{
		Fixture.Init({FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1).SetAcceptedLootIntents(ERockLootIntent::Store | ERockLootIntent::Equip)});
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		ASSERT_THAT(IsTrue(LootWith(Apple, ERockLootIntent::Store).IsValid()));
		ASSERT_THAT(IsTrue(LootWith(Fixture.MakeDefinition("Apple2"), ERockLootIntent::Equip).IsValid()));
	}

	TEST_METHOD(EqualPriorities_KeepConfigOrder)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Storage, 1, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});

		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Storage, 0, 0), LootWith(Fixture.MakeDefinition("A"), ERockLootIntent::Store)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), LootWith(Fixture.MakeDefinition("B"), ERockLootIntent::Store)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), LootWith(Fixture.MakeDefinition("C"), ERockLootIntent::Store)));
	}

	TEST_METHOD(LowerPriorityNumber_WinsOverConfigOrder_IncludingNegative)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Storage, 1, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1).SetLootPriority(-5),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1).SetLootPriority(-1)});

		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), LootWith(Fixture.MakeDefinition("A"), ERockLootIntent::Store)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, 0, 0), LootWith(Fixture.MakeDefinition("B"), ERockLootIntent::Store)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Storage, 0, 0), LootWith(Fixture.MakeDefinition("C"), ERockLootIntent::Store)));
	}

	TEST_METHOD(Preference_BeatsPriority_ButNeverExcludes)
	{
		// Pockets has the better priority; Backpack prefers food and so is tried first for food only.
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1).SetLootPriority(0),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 2, 1).SetLootPriority(5)
				.SetLootPreference(FGameplayTagQuery::MakeQuery_MatchAnyTags(Tags(RockInventoryTestTags::Food)))});

		ASSERT_THAT(AreEqual(BackpackSlot(0), LootWith(Fixture.MakeTaggedDefinition("Apple", Tags(RockInventoryTestTags::Food)), ERockLootIntent::Store)));
		// Not food: the preference does not match, so the better priority wins ...
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), LootWith(Sword(), ERockLootIntent::Store)));
		// ... and a section that merely allows the item is still used when the preferred one is not an option
		ASSERT_THAT(AreEqual(BackpackSlot(1), LootWith(Sword("S2"), ERockLootIntent::Store)));
	}

	TEST_METHOD(Merge_PartialStackInALaterPlanSection_IsToppedUpBeforeEmptySpaceInAnEarlierOne)
	{
		InitPlayer();
		URockItemDefinition* Arrow = Fixture.MakeTaggedDefinition("Arrow", Tags(RockInventoryTestTags::Food), 10);
		Fixture.PlaceAt(Arrow, 5, BackpackSlot(2));

		FRockLootParams Params;
		Params.Intent = AsInt(ERockLootIntent::Store);
		FRockLootResult Result;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Arrow, 4), Params, Result)));
		ASSERT_THAT(AreEqual(9, Fixture.Inventory->GetItemBySlotHandle(BackpackSlot(2)).GetStackCount()));
		ASSERT_THAT(IsNull(Result.FindNewStack()));
	}

	TEST_METHOD(Merge_IntoAnEquipOnlySection_IsNotDoneForAStoreCall)
	{
		InitPlayer();
		URockItemDefinition* Arrow = Fixture.MakeTaggedDefinition("Arrow", Tags(RockInventoryTestTags::Weapon), 10);
		Fixture.PlaceAt(Arrow, 5, PrimarySlot());

		FRockLootParams Params;
		Params.Intent = AsInt(ERockLootIntent::Store);
		FRockLootResult Result;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Arrow, 4), Params, Result)));
		ASSERT_THAT(AreEqual(5, Fixture.Inventory->GetItemBySlotHandle(PrimarySlot()).GetStackCount()));
		ASSERT_THAT(AreEqual(BackpackSlot(0), Result.FindNewStack()->Slot.GetSlotHandle()));
	}

	TEST_METHOD(DescribeLootPlan_ListsTheOrderAndWhySectionsAreSkipped)
	{
		InitPlayer();
		FRockLootParams Params;
		Params.Intent = AsInt(ERockLootIntent::Store | ERockLootIntent::Equip);
		const FString Text = URockInventoryLibrary::DescribeLootPlan(Fixture.Inventory, FRockItemStack(Pistol(), 1), Params);

		const int32 Secondary = Text.Find(RockInventoryTestTags::SectionSecondary.ToString());
		const int32 Primary = Text.Find(RockInventoryTestTags::SectionPrimary.ToString());
		const int32 Backpack = Text.Find(RockInventoryTags::Inventory_Section_Backpack.GetTag().ToString());
		ASSERT_THAT(IsTrue(Secondary != INDEX_NONE && Primary != INDEX_NONE && Backpack != INDEX_NONE));
		ASSERT_THAT(IsTrue(Secondary < Primary && Primary < Backpack));
		ASSERT_THAT(IsTrue(Text.Contains(TEXT("preferred"))));
		ASSERT_THAT(IsTrue(Text.Contains(TEXT("skipped"))));
		ASSERT_THAT(IsTrue(Text.Contains(TEXT("section filter rejects the item"))));

		Params.Intent = AsInt(ERockLootIntent::Store);
		ASSERT_THAT(IsTrue(URockInventoryLibrary::DescribeLootPlan(Fixture.Inventory, FRockItemStack(Pistol(), 1), Params).Contains(TEXT("does not accept this intent"))));
	}

	TEST_METHOD(BuildLootPlan_ReturnsRootEntriesInOrder)
	{
		InitPlayer();
		FRockLootParams Params;
		Params.Intent = AsInt(ERockLootIntent::Store | ERockLootIntent::Equip);
		FRockLootPlan Plan;
		URockInventoryLibrary::BuildLootPlan(Fixture.Inventory, FRockItemStack(Pistol(), 1), Params, Plan);

		ASSERT_THAT(AreEqual(3, Plan.Num()));
		ASSERT_THAT(AreEqual(2, Plan[0].SectionIndex));
		ASSERT_THAT(AreEqual(1, Plan[1].SectionIndex));
		ASSERT_THAT(AreEqual(3, Plan[2].SectionIndex));
		for (const FRockLootPlanEntry& Entry : Plan)
		{
			ASSERT_THAT(IsTrue(Entry.Inventory == Fixture.Inventory));
		}
	}

	// Timing log, no threshold: 200 single items into 12 sections of 5x5 slots (300 slots), each section carrying a preference and a priority.
	TEST_METHOD(Timing_TwoHundredItemsIntoTwelveSections_IsLogged)
	{
		TArray<FRockInventorySectionInfo> Sections;
		for (int32 Index = 0; Index < 12; ++Index)
		{
			Sections.Add(FRockInventoryFixture::MakeSection(RockInventoryTestTags::BulkSections[Index], 5, 5)
				.SetLootPriority(Index % 4)
				.SetLootPreference(FGameplayTagQuery::MakeQuery_MatchAnyTags(Tags(Index % 2 ? RockInventoryTestTags::Food : RockInventoryTestTags::Weapon))));
		}
		Fixture.Init(Sections);
		URockItemDefinition* Apple = Fixture.MakeTaggedDefinition("Apple", Tags(RockInventoryTestTags::Food), 5);
		URockItemDefinition* Blade = Fixture.MakeTaggedDefinition("Blade", Tags(RockInventoryTestTags::Weapon));

		FRockLootParams Params;
		int32 Placed = 0;
		const double Start = FPlatformTime::Seconds();
		for (int32 Index = 0; Index < 200; ++Index)
		{
			FRockLootResult Result;
			Placed += URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Index % 2 ? Apple : Blade, 1), Params, Result) ? 1 : 0;
		}
		const double Seconds = FPlatformTime::Seconds() - Start;
		UE_LOG(LogTemp, Display, TEXT("RockInventory loot timing: 200 loot calls into 12 sections / 300 slots took %.3f ms (%.1f us per call), %d fully placed"),
			Seconds * 1000.0, Seconds * 1.0e6 / 200.0, Placed);
		ASSERT_THAT(AreEqual(200, Placed));
	}
};
// Equip and swap (T-62). Layout "Swap": Primary (Equip, weapon filter, 1x1), Backpack (Store, priority 10, 2x1 unless a test says otherwise).
// An Equip call with bAllowSwap takes an empty equipment slot first; with none it displaces the first occupied one and stores that stack through Store.
TEST_CLASS(RockInventoryLootSwapTests, "BRS.RockInventory.Loot.Swap")
{
	FRockInventoryFixture Fixture;

	static FGameplayTagContainer Tags(const FGameplayTag& A) { return FGameplayTagContainer(A); }
	static int32 AsInt(ERockLootIntent Intent) { return static_cast<int32>(Intent); }

	void InitSwap(int32 BackpackColumns = 2, int32 PrimaryColumns = 1)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTestTags::SectionPrimary, PrimaryColumns, 1, Tags(RockInventoryTestTags::Weapon))
				.SetAcceptedLootIntents(ERockLootIntent::Equip),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, BackpackColumns, 1)
				.SetLootPriority(10)});
	}

	FRockInventorySlotHandle PrimarySlot(int32 Column = 0) const { return Fixture.SlotAt(RockInventoryTestTags::SectionPrimary, Column, 0); }
	FRockInventorySlotHandle BackpackSlot(int32 Column) const { return Fixture.SlotAt(RockInventoryTags::Inventory_Section_Backpack, Column, 0); }
	URockItemDefinition* Sword(FName Id) { return Fixture.MakeTaggedDefinition(Id, Tags(RockInventoryTestTags::Weapon)); }

	const URockItemDefinition* DefinitionIn(const FRockInventorySlotHandle& Slot) const
	{
		const FRockItemStack Stack = Fixture.Inventory->GetItemBySlotHandle(Slot);
		return Stack.IsValid() ? Stack.GetDefinition() : nullptr;
	}

	static FRockLootParams EquipParams(bool bAllowSwap, ERockLootIntent Intent = ERockLootIntent::Equip)
	{
		FRockLootParams Params;
		Params.Intent = AsInt(Intent);
		Params.bAllowSwap = bAllowSwap;
		return Params;
	}

	// Previews, loots, checks the two agree (placements, excess, swap and where the displaced stack goes), and returns the real result.
	FRockLootResult LootAndCompare(URockItemDefinition* Definition, const FRockLootParams& Params, int32 Count = 1)
	{
		const FRockItemStack Stack(Definition, Count);
		const FRockLootResult Preview = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, Stack, Params);
		FRockLootResult Real;
		URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, Stack, Params, Real);
		auto Agree = [](const TArray<FRockLootPlacement>& A, const TArray<FRockLootPlacement>& B)
		{
			if (A.Num() != B.Num())
			{
				return false;
			}
			for (int32 Index = 0; Index < A.Num(); ++Index)
			{
				if (A[Index].Slot.GetSlotHandle() != B[Index].Slot.GetSlotHandle() || A[Index].Count != B[Index].Count
					|| A[Index].bNewStack != B[Index].bNewStack || A[Index].Orientation != B[Index].Orientation)
				{
					return false;
				}
			}
			return true;
		};
		if (!Agree(Preview.Placements, Real.Placements) || !Agree(Preview.DisplacedPlacements, Real.DisplacedPlacements)
			|| Preview.Excess != Real.Excess || Preview.bSwapped != Real.bSwapped
			|| Preview.DisplacedSlot.GetSlotHandle() != Real.DisplacedSlot.GetSlotHandle())
		{
			AddError(TEXT("PreviewLoot and LootItemToInventory disagree"));
		}
		return Real;
	}

	TEST_METHOD(EmptyEquipmentSlot_IsTakenBeforeASwap)
	{
		InitSwap(2, 2);
		URockItemDefinition* Held = Sword("Held");
		Fixture.PlaceAt(Held, 1, PrimarySlot(0));

		const FRockLootResult Result = LootAndCompare(Sword("New"), EquipParams(true));

		ASSERT_THAT(IsFalse(Result.bSwapped));
		ASSERT_THAT(AreEqual(PrimarySlot(1), Result.FindNewStack()->Slot.GetSlotHandle()));
		ASSERT_THAT(IsTrue(DefinitionIn(PrimarySlot(0)) == Held));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Swap_PutsTheNewItemInTheSlotAndStoresTheDisplacedOne)
	{
		InitSwap();
		URockItemDefinition* Held = Sword("Held");
		URockItemDefinition* Incoming = Sword("Incoming");
		Fixture.PlaceAt(Held, 1, PrimarySlot());

		const FRockLootResult Result = LootAndCompare(Incoming, EquipParams(true));

		ASSERT_THAT(IsTrue(Result.bSwapped));
		ASSERT_THAT(IsTrue(Result.IsFullyPlaced()));
		ASSERT_THAT(AreEqual(PrimarySlot(), Result.DisplacedSlot.GetSlotHandle()));
		ASSERT_THAT(IsTrue(DefinitionIn(PrimarySlot()) == Incoming));
		ASSERT_THAT(IsTrue(DefinitionIn(BackpackSlot(0)) == Held));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Swap_DisplacedStackMergesIntoAPartialStackInStorage)
	{
		InitSwap();
		URockItemDefinition* Arrow = Fixture.MakeTaggedDefinition("Arrow", Tags(RockInventoryTestTags::Weapon), 10);
		Fixture.PlaceAt(Arrow, 6, PrimarySlot());
		Fixture.PlaceAt(Arrow, 5, BackpackSlot(1));
		// A different weapon takes the slot; the arrows (6) top up the stack of 5 to 10 and 1 goes to the first empty backpack slot
		const FRockLootResult Result = LootAndCompare(Sword("Sword"), EquipParams(true));

		ASSERT_THAT(IsTrue(Result.bSwapped));
		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemBySlotHandle(BackpackSlot(1)).GetStackCount()));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetItemBySlotHandle(BackpackSlot(0)).GetStackCount()));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Swap_WithNoRoomForTheDisplacedItem_IsRefusedAndNothingChanges)
	{
		InitSwap(1);
		URockItemDefinition* Held = Sword("Held");
		URockItemDefinition* Bag = Fixture.MakeDefinition("Bag");
		Fixture.PlaceAt(Held, 1, PrimarySlot());
		Fixture.PlaceAt(Bag, 1, BackpackSlot(0));
		const TArray<FString> Before = URockInventoryLibrary::GetInventoryContentsDebug(Fixture.Inventory);

		const FRockLootResult Result = LootAndCompare(Sword("Incoming"), EquipParams(true));

		ASSERT_THAT(IsFalse(Result.bSwapped));
		ASSERT_THAT(IsTrue(Result.Placements.IsEmpty()));
		ASSERT_THAT(IsTrue(Result.DisplacedPlacements.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Result.Excess));
		ASSERT_THAT(IsTrue(DefinitionIn(PrimarySlot()) == Held));
		ASSERT_THAT(IsTrue(DefinitionIn(BackpackSlot(0)) == Bag));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetNumItemStacks()));
		ASSERT_THAT(IsTrue(Before == URockInventoryLibrary::GetInventoryContentsDebug(Fixture.Inventory)));
	}

	TEST_METHOD(Swap_ItemNoEquipmentSectionAccepts_IsRefusedAndNotStored)
	{
		InitSwap();
		Fixture.PlaceAt(Sword("Held"), 1, PrimarySlot());
		URockItemDefinition* Apple = Fixture.MakeTaggedDefinition("Apple", Tags(RockInventoryTestTags::Food));

		const FRockLootResult Result = LootAndCompare(Apple, EquipParams(true));

		ASSERT_THAT(IsFalse(Result.bSwapped));
		ASSERT_THAT(IsTrue(Result.Placements.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
		ASSERT_THAT(IsNull(DefinitionIn(BackpackSlot(0))));
	}

	TEST_METHOD(EquipWithoutAllowSwap_NeverSwaps)
	{
		InitSwap();
		URockItemDefinition* Held = Sword("Held");
		Fixture.PlaceAt(Held, 1, PrimarySlot());

		const FRockLootResult Result = LootAndCompare(Sword("Incoming"), EquipParams(false));

		ASSERT_THAT(IsFalse(Result.bSwapped));
		ASSERT_THAT(IsTrue(Result.Placements.IsEmpty()));
		ASSERT_THAT(IsTrue(DefinitionIn(PrimarySlot()) == Held));
	}

	TEST_METHOD(StoreAndEquip_NeverSwaps_ItFallsBackToStorage)
	{
		InitSwap();
		URockItemDefinition* Held = Sword("Held");
		URockItemDefinition* Incoming = Sword("Incoming");
		Fixture.PlaceAt(Held, 1, PrimarySlot());

		const FRockLootResult Result = LootAndCompare(Incoming, EquipParams(true, ERockLootIntent::Store | ERockLootIntent::Equip));

		ASSERT_THAT(IsFalse(Result.bSwapped));
		ASSERT_THAT(IsTrue(DefinitionIn(PrimarySlot()) == Held));
		ASSERT_THAT(IsTrue(DefinitionIn(BackpackSlot(0)) == Incoming));
	}

	TEST_METHOD(StoreOnly_WithAllowSwap_DoesNotSwap)
	{
		InitSwap();
		URockItemDefinition* Held = Sword("Held");
		Fixture.PlaceAt(Held, 1, PrimarySlot());

		const FRockLootResult Result = LootAndCompare(Sword("Incoming"), EquipParams(true, ERockLootIntent::Store));

		ASSERT_THAT(IsFalse(Result.bSwapped));
		ASSERT_THAT(IsTrue(DefinitionIn(PrimarySlot()) == Held));
	}

	TEST_METHOD(Swap_ItemThatFitsNoOccupiedSlot_IsRefused)
	{
		// Primary is a row of two 1x1 slots, both held. A 2x1 item needs both cells, and a swap frees only one stack's footprint.
		InitSwap(2, 2);
		URockItemDefinition* Left = Sword("Left");
		URockItemDefinition* Right = Sword("Right");
		URockItemDefinition* Greatsword = Fixture.MakeTaggedDefinition("Greatsword", Tags(RockInventoryTestTags::Weapon), 1, FIntPoint(2, 1));
		Fixture.PlaceAt(Left, 1, PrimarySlot(0));
		Fixture.PlaceAt(Right, 1, PrimarySlot(1));

		const FRockLootResult Result = LootAndCompare(Greatsword, EquipParams(true));

		ASSERT_THAT(IsFalse(Result.bSwapped));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetNumItemStacks()));
		ASSERT_THAT(IsTrue(DefinitionIn(PrimarySlot(0)) == Left));
		ASSERT_THAT(IsTrue(DefinitionIn(PrimarySlot(1)) == Right));
	}
};
#endif // WITH_DEV_AUTOMATION_TESTS
