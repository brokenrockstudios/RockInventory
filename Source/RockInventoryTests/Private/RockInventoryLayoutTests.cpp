// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Inventory/RockInventoryConfig.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// How Init turns a config into sections and slots.
TEST_CLASS(RockInventoryLayoutTests, "BRS.RockInventory.Layout")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(Init_TwoSections_PacksSlotsContiguously)
	{
		Fixture.Init({
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 3, 2),
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Pockets, 0, 2, 1),
		});

		const FRockInventorySectionInfo& Main = Fixture.Inventory->GetSectionInfo(RockInventoryTags::Inventory_Section_Backpack);
		const FRockInventorySectionInfo& Pocket = Fixture.Inventory->GetSectionInfo(RockInventoryTags::Inventory_Section_Pockets);
		ASSERT_THAT(AreEqual(0, Main.GetFirstSlotIndex()));
		ASSERT_THAT(AreEqual(6, Main.GetNumSlots()));
		// The authored FirstSlotIndex (0) is ignored; Init assigns it from the preceding sections.
		ASSERT_THAT(AreEqual(6, Pocket.GetFirstSlotIndex()));
		ASSERT_THAT(AreEqual(2, Pocket.GetNumSlots()));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetSectionIndex(RockInventoryTags::Inventory_Section_Backpack)));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetSectionIndex(RockInventoryTags::Inventory_Section_Pockets)));
	}

	TEST_METHOD(Init_SlotHandles_MatchTheirAbsoluteIndex)
	{
		Fixture.Init({
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 3, 2),
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Pockets, 0, 2, 1),
		});

		for (int32 Index = 0; Index < 8; ++Index)
		{
			const FRockInventorySlotEntry& Slot = Fixture.Inventory->GetSlotByAbsoluteIndex(Index);
			ASSERT_THAT(AreEqual(Index, Slot.SlotHandle.GetAbsoluteIndex()));
		}
	}

	TEST_METHOD(Init_AllSlotsStartEmpty)
	{
		Fixture.InitGrid(4, 3);

		int32 SlotCount = 0;
		bool bAllEmpty = true;
		Fixture.Inventory->ForEachSlotInSection([&](const FRockInventorySectionInfo&, const FRockInventorySlotEntry& Slot)
		{
			++SlotCount;
			bAllEmpty &= !Slot.ItemHandle.IsValid() && !Slot.bIsLocked;
			return true;
		});
		ASSERT_THAT(AreEqual(12, SlotCount));
		ASSERT_THAT(IsTrue(bAllEmpty));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(GetSectionInfoBySlotHandle_ResolvesEachSectionAtItsBoundary)
	{
		Fixture.Init({
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 3, 2),
			FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Pockets, 0, 2, 1),
		});

		ASSERT_THAT(IsTrue(Fixture.Inventory->GetSectionInfoBySlotHandle(FRockInventorySlotHandle(5)).GetSectionTag() == RockInventoryTags::Inventory_Section_Backpack));
		ASSERT_THAT(IsTrue(Fixture.Inventory->GetSectionInfoBySlotHandle(FRockInventorySlotHandle(6)).GetSectionTag() == RockInventoryTags::Inventory_Section_Pockets));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetSectionInfoBySlotHandle(FRockInventorySlotHandle(8)).IsValid()));
	}

	TEST_METHOD(GetSectionIndex_UnknownOrInvalidTag_ReturnsNone)
	{
		Fixture.InitGrid(2, 2);

		ASSERT_THAT(AreEqual(INDEX_NONE, Fixture.Inventory->GetSectionIndex(RockInventoryTags::Inventory_Section_Pockets)));
		ASSERT_THAT(AreEqual(INDEX_NONE, Fixture.Inventory->GetSectionIndex(FGameplayTag())));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetSectionInfo(RockInventoryTags::Inventory_Section_Pockets).IsValid()));
	}

	TEST_METHOD(GetSlotByHandle_OutOfRange_ReturnsInvalidEntryAndWarns)
	{
		Fixture.InitGrid(2, 2);
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid slot index"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);

		ASSERT_THAT(IsFalse(Fixture.Inventory->GetSlotByHandle(FRockInventorySlotHandle(4)).IsValid()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetSlotByHandle(FRockInventorySlotHandle::Invalid()).IsValid()));
	}

	TEST_METHOD(Init_NullConfig_LogsErrorAndLeavesInventoryEmpty)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid config object"), ELogVerbosity::Error);
		URockInventory* Inventory = NewObject<URockInventory>(GetTransientPackage());
		Inventory->Init(nullptr);

		ASSERT_THAT(AreEqual(0, Inventory->GetNumItemStacks()));
		ASSERT_THAT(AreEqual(INDEX_NONE, Inventory->GetSectionIndex(RockInventoryTags::Inventory_Section_Backpack)));
	}

	TEST_METHOD(Init_ConfigWithNoTabs_LogsError)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("No inventory tabs defined"), ELogVerbosity::Error);
		URockInventory* Inventory = NewObject<URockInventory>(GetTransientPackage());
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Inventory->Init(Config);

		ASSERT_THAT(AreEqual(INDEX_NONE, Inventory->GetSectionIndex(RockInventoryTags::Inventory_Section_Backpack)));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
