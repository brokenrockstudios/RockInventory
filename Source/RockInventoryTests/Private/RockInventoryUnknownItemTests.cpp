// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Library/RockInventoryLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace RockInventoryUnknownItemTestsPrivate
{
	const TCHAR* UnknownIdWarning = TEXT("shows the unknown-item placeholder");

	/** Adds a stack known only by its id (as a client holds it) and assigns it to the slot. */
	FRockItemStackHandle PlaceUnknownAt(FRockInventoryFixture& Fixture, FName ItemId, int32 Count, FRockInventorySlotHandle Slot)
	{
		const FRockItemStackHandle Handle = Fixture.Inventory->AddItemToInventory(FRockItemStack::FromItemId(ItemId, Count));
		FRockInventorySlotEntry Entry = Fixture.Inventory->GetSlotByHandle(Slot);
		Entry.ItemHandle = Handle;
		Fixture.Inventory->SetSlotByHandle(Slot, Entry);
		return Handle;
	}
}

using namespace RockInventoryUnknownItemTestsPrivate;

// T-1010: an ItemId the registry does not know resolves to a placeholder definition instead of an invalid stack.
TEST_CLASS(RockInventoryUnknownItemTests, "BRS.RockInventory.UnknownItem")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(UnknownId_IsAValidPlaceholderThatKeepsTheId)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

		const FRockItemStack Stack = FRockItemStack::FromItemId("Test.Gone.Item", 3);

		ASSERT_THAT(IsTrue(Stack.IsValid()));
		ASSERT_THAT(IsTrue(Stack.IsUnknownItem()));
		ASSERT_THAT(AreEqual(FName("Test.Gone.Item"), Stack.GetItemId()));
		ASSERT_THAT(AreEqual(3, Stack.GetStackCount()));
		ASSERT_THAT(AreEqual(1, Stack.GetMaxStackCount()));
		ASSERT_THAT(IsTrue(Stack.GetDefinition()->GridSize == FIntPoint(1, 1)));
	}

	TEST_METHOD(KnownId_IsNotAPlaceholder)
	{
		Fixture.InitGrid(2, 1);
		Fixture.MakeDefinition("Apple", 8);

		const FRockItemStack Stack = FRockItemStack::FromItemId("Apple", 2);

		ASSERT_THAT(IsTrue(Stack.IsValid()));
		ASSERT_THAT(IsFalse(Stack.IsUnknownItem()));
		ASSERT_THAT(AreEqual(8, Stack.GetMaxStackCount()));
	}

	TEST_METHOD(Placeholder_NeverStacksNotEvenWithTheSameId)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);

		const FRockItemStack First = FRockItemStack::FromItemId("Test.Gone.Item", 1);
		const FRockItemStack Second = FRockItemStack::FromItemId("Test.Gone.Item", 1);

		ASSERT_THAT(IsFalse(First.CanStackWith(Second)));
		ASSERT_THAT(IsFalse(Second.CanStackWith(First)));
	}

	TEST_METHOD(Placeholder_DoesNotStackWithARealItem)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 8);

		const FRockItemStack Unknown = FRockItemStack::FromItemId("Test.Gone.Item", 1);

		ASSERT_THAT(IsFalse(Unknown.CanStackWith(FRockItemStack(Apple, 1))));
		ASSERT_THAT(IsFalse(FRockItemStack(Apple, 1).CanStackWith(Unknown)));
	}

	TEST_METHOD(Placeholder_AddedToAnInventory_KeepsItsCountAndId)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Fixture.InitGrid(2, 1);

		const FRockItemStackHandle Handle = PlaceUnknownAt(Fixture, "Test.Gone.Item", 7, Fixture.SlotAt(0, 0));

		const FRockItemStack& Stored = Fixture.Inventory->GetItemByHandle(Handle);
		ASSERT_THAT(IsTrue(Stored.IsValid()));
		ASSERT_THAT(AreEqual(7, Stored.GetStackCount())); // over its max of 1, and not clamped
		ASSERT_THAT(AreEqual(FName("Test.Gone.Item"), Stored.GetItemId()));
	}

	TEST_METHOD(Placeholder_MovesWhole)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Fixture.InitGrid(3, 1);
		const FRockItemStackHandle Handle = PlaceUnknownAt(Fixture, "Test.Gone.Item", 4, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 0))));

		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(2, 0)).ItemHandle));
		ASSERT_THAT(AreEqual(4, Fixture.Inventory->GetItemByHandle(Handle).GetStackCount()));
	}

	TEST_METHOD(Placeholder_CannotBeSplit)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Item cannot be moved to target location"), ELogVerbosity::Warning);
		Fixture.InitGrid(3, 1);
		const FRockItemStackHandle Handle = PlaceUnknownAt(Fixture, "Test.Gone.Item", 4, Fixture.SlotAt(0, 0));

		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::HalfStack;
		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 0), Params)));

		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
		ASSERT_THAT(AreEqual(4, Fixture.Inventory->GetItemByHandle(Handle).GetStackCount()));
	}

	TEST_METHOD(RegistryGainsTheDefinition_PlaceholderStackResolvesAndKeepsItsCount)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Fixture.InitGrid(4, 1);
		const FRockItemStackHandle Handle = PlaceUnknownAt(Fixture, "Test.Late.Item", 2, Fixture.SlotAt(0, 0));
		ASSERT_THAT(IsTrue(Fixture.Inventory->GetItemByHandle(Handle).IsUnknownItem()));

		// Registering announces the new definition, as a registry refresh does.
		URockItemDefinition* Late = Fixture.MakeDefinition("Test.Late.Item", 5, FIntPoint(2, 1));

		const FRockItemStack& Stored = Fixture.Inventory->GetItemByHandle(Handle);
		ASSERT_THAT(IsFalse(Stored.IsUnknownItem()));
		ASSERT_THAT(AreEqual(Late, Stored.GetDefinition()));
		ASSERT_THAT(AreEqual(2, Stored.GetStackCount()));
	}

	TEST_METHOD(Footprints_OverlappingAfterTheRealSizeReturns_AreToleratedUntilOneMoves)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Fixture.InitGrid(5, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Unknown = PlaceUnknownAt(Fixture, "Test.Late.Item", 1, Fixture.SlotAt(0, 0));
		const FRockItemStackHandle AppleHandle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(1, 0));

		// The real definition is 2x1: it now covers the cell the apple sits in.
		Fixture.MakeDefinition("Test.Late.Item", 1, FIntPoint(2, 1));

		TArray<bool> Occupancy;
		URockInventoryLibrary::PrecomputeOccupancyGrids(Fixture.Inventory, Occupancy);
		ASSERT_THAT(AreEqual(5, Occupancy.Num()));
		ASSERT_THAT(IsTrue(Occupancy[0]));
		ASSERT_THAT(IsTrue(Occupancy[1]));
		ASSERT_THAT(IsFalse(Occupancy[2]));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Unknown)));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(AppleHandle)));

		// Moving the apple away leaves a clean layout.
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(1, 0), Fixture.Inventory, Fixture.SlotAt(4, 0))));
		URockInventoryLibrary::PrecomputeOccupancyGrids(Fixture.Inventory, Occupancy);
		ASSERT_THAT(IsTrue(Occupancy[0]));
		ASSERT_THAT(IsTrue(Occupancy[1]));
		ASSERT_THAT(IsFalse(Occupancy[2]));
		ASSERT_THAT(IsFalse(Occupancy[3]));
		ASSERT_THAT(IsTrue(Occupancy[4]));
	}

	TEST_METHOD(Footprints_OverlappingItemCanMoveAwayToo)
	{
		TestRunner->AddExpectedMessagePlain(UnknownIdWarning, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Fixture.InitGrid(5, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Unknown = PlaceUnknownAt(Fixture, "Test.Late.Item", 1, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(1, 0));
		Fixture.MakeDefinition("Test.Late.Item", 1, FIntPoint(2, 1));

		// The wide item itself moves to the free cells at the right end.
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(3, 0))));

		ASSERT_THAT(AreEqual(Unknown, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(3, 0)).ItemHandle));
		TArray<bool> Occupancy;
		URockInventoryLibrary::PrecomputeOccupancyGrids(Fixture.Inventory, Occupancy);
		ASSERT_THAT(IsFalse(Occupancy[0]));
		ASSERT_THAT(IsTrue(Occupancy[1])); // the apple
		ASSERT_THAT(IsTrue(Occupancy[3]));
		ASSERT_THAT(IsTrue(Occupancy[4]));
	}

	TEST_METHOD(Placeholder_StaysOutOfTheRegistryLookup)
	{
		URockItemRegistrySubsystem* Registry = URockItemRegistrySubsystem::GetInstance();
		URockItemDefinition* Placeholder = Registry->GetUnknownItemDefinition();
		ASSERT_THAT(IsNotNull(Placeholder));
		ASSERT_THAT(IsTrue(Registry->IsUnknownItemDefinition(Placeholder)));

		TArray<URockItemDefinition*> All;
		Registry->GetAllDefinitions(All);
		ASSERT_THAT(IsFalse(All.Contains(Placeholder)));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
