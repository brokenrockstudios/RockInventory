// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

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

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack::Invalid(), Slot, Excess)));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Loot_NullInventory_FailsAndWarns)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid Parameters. Inventory"), ELogVerbosity::Warning);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");

		FRockInventorySlotHandle Slot;
		int32 Excess = -1;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::LootItemToInventory(nullptr, FRockItemStack(Apple, 2), Slot, Excess)));
		ASSERT_THAT(AreEqual(2, Excess));
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

#endif // WITH_DEV_AUTOMATION_TESTS
