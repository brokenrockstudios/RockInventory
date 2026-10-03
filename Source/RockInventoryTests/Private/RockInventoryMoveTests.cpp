// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Library/RockInventoryLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace
{
	bool IsSlotEmpty(const FRockInventoryFixture& Fixture, const FRockInventorySlotHandle& Slot)
	{
		return !Fixture.Inventory->GetSlotByHandle(Slot).ItemHandle.IsValid();
	}
}

// URockInventoryLibrary::MoveItem: relocation, merging and partial moves.
TEST_CLASS(RockInventoryMoveTests, "BRS.RockInventory.Move")
{
	FRockInventoryFixture Fixture;
	// A second, separate inventory for cross-inventory moves.
	FRockInventoryFixture Other;

	TEST_METHOD(Move_ToAnEmptySlot_KeepsTheHandleAndClearsTheSource)
	{
		Fixture.InitGrid(4, 2);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 1))));

		ASSERT_THAT(IsTrue(IsSlotEmpty(Fixture, Fixture.SlotAt(0, 0))));
		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(2, 1)).ItemHandle));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Handle)));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Move_ToTheSameSlot_IsANoOp)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(0, 0))));

		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
	}

	TEST_METHOD(Move_FromAnEmptySlot_FailsAndWarns)
	{
		Fixture.InitGrid(2, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Source Slot is empty"), ELogVerbosity::Warning);

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));
	}

	TEST_METHOD(Move_WithANullInventory_FailsAndWarns)
	{
		Fixture.InitGrid(2, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid Source or Target Inventory"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(nullptr, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), nullptr, Fixture.SlotAt(1, 0))));
	}

	TEST_METHOD(Move_ToAnOutOfRangeSlot_FailsWithoutLosingTheItem)
	{
		Fixture.InitGrid(2, 1);
		// GetSlotByHandle also warns about the bad index before MoveItem reports it.
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid slot index"), ELogVerbosity::Warning);
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid Target Slot Handle"), ELogVerbosity::Warning);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, FRockInventorySlotHandle(50))));

		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
	}

	TEST_METHOD(Move_OntoADifferentItem_FailsAndChangesNothing)
	{
		Fixture.InitGrid(2, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Item cannot be moved to target location"), ELogVerbosity::Warning);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		URockItemDefinition* Pear = Fixture.MakeDefinition("Pear");
		const FRockItemStackHandle AppleHandle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		const FRockItemStackHandle PearHandle = Fixture.PlaceAt(Pear, 1, Fixture.SlotAt(1, 0));

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(AppleHandle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
		ASSERT_THAT(AreEqual(PearHandle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(1, 0)).ItemHandle));
	}

	TEST_METHOD(Move_MultiCellItemOverlappingItsOwnFootprint_Succeeds)
	{
		// The item's current cells don't count as blocking itself, so sliding a 2x2 one column over is legal.
		Fixture.InitGrid(4, 2);
		URockItemDefinition* Crate = Fixture.MakeDefinition("Crate", 1, FIntPoint(2, 2));
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Crate, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(IsTrue(IsSlotEmpty(Fixture, Fixture.SlotAt(0, 0))));
		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(1, 0)).ItemHandle));
	}

	TEST_METHOD(Move_MultiCellItemPastTheEdge_FailsAndStaysPut)
	{
		Fixture.InitGrid(4, 4);
		TestRunner->AddExpectedMessagePlain(TEXT("Item cannot be moved to target location"), ELogVerbosity::Warning);
		URockItemDefinition* Crate = Fixture.MakeDefinition("Crate", 1, FIntPoint(2, 2));
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Crate, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(3, 3))));

		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
		ASSERT_THAT(IsTrue(IsSlotEmpty(Fixture, Fixture.SlotAt(3, 3))));
	}

	TEST_METHOD(Move_MultiCellItemOverAnotherItem_FailsAndStaysPut)
	{
		Fixture.InitGrid(4, 2);
		TestRunner->AddExpectedMessagePlain(TEXT("Item cannot be moved to target location"), ELogVerbosity::Warning);
		URockItemDefinition* Crate = Fixture.MakeDefinition("Crate", 1, FIntPoint(2, 2));
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle CrateHandle = Fixture.PlaceAt(Crate, 1, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(2, 0));

		// Landing at column 1 would cover columns 1-2, and the apple sits in column 2.
		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(CrateHandle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
	}

	TEST_METHOD(Move_HalfStack_SplitsRoundingUp)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStackHandle Source = Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::HalfStack;

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 0), Params)));

		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetItemByHandle(Source).GetStackCount()));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(2, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(5, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Arrow")));
	}

	TEST_METHOD(Move_SingleItem_TakesExactlyOne)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 4, Fixture.SlotAt(0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::SingleItem;

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0), Params)));

		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(1, 0)).GetStackCount()));
	}

	TEST_METHOD(Move_CustomAmount_TakesThatManyAndClampsToTheStack)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 7, Fixture.SlotAt(0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::CustomAmount;
		Params.MoveCount = 3;

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0), Params)));
		ASSERT_THAT(AreEqual(4, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(1, 0)).GetStackCount()));

		// Asking for more than the stack holds moves the whole remaining stack.
		Params.MoveCount = 50;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 0), Params)));
		ASSERT_THAT(IsTrue(IsSlotEmpty(Fixture, Fixture.SlotAt(0, 0))));
		ASSERT_THAT(AreEqual(4, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(2, 0)).GetStackCount()));
	}

	TEST_METHOD(Move_IntoAPartialStack_MergesUpToTheMaxAndKeepsTheRest)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStackHandle Source = Fixture.PlaceAt(Arrow, 6, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Arrow, 7, Fixture.SlotAt(1, 0));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(1, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemByHandle(Source).GetStackCount()));
		ASSERT_THAT(AreEqual(13, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Arrow")));
	}

	TEST_METHOD(Move_IntoAPartialStack_RemovesTheSourceWhenItFitsEntirely)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStackHandle Source = Fixture.PlaceAt(Arrow, 2, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(1, 0));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(7, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(1, 0)).GetStackCount()));
		ASSERT_THAT(IsTrue(IsSlotEmpty(Fixture, Fixture.SlotAt(0, 0))));
		ASSERT_THAT(IsFalse(Fixture.Inventory->IsHandleValid(Source)));
	}

	TEST_METHOD(Move_IntoAFullStack_FailsAndChangesNothing)
	{
		Fixture.InitGrid(2, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Item cannot be moved to target location"), ELogVerbosity::Warning);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 4, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Arrow, 10, Fixture.SlotAt(1, 0));

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(4, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(1, 0)).GetStackCount()));
	}

	TEST_METHOD(Move_StoresTheDesiredOrientationOnTheTargetSlot)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0), Params)));

		ASSERT_THAT(AreEqual(ERockItemOrientation::Vertical, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(1, 0)).Orientation));
		// The vacated slot goes back to the default orientation.
		ASSERT_THAT(AreEqual(ERockItemOrientation::Horizontal, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).Orientation));
	}

	void SetOrientation(FRockInventorySlotHandle Slot, ERockItemOrientation Orientation)
	{
		FRockInventorySlotEntry Entry = Fixture.Inventory->GetSlotByHandle(Slot);
		Entry.Orientation = Orientation;
		Fixture.Inventory->SetSlotByHandle(Slot, Entry);
	}

	TEST_METHOD(Move_RotatedOnArrival_FitsUsingTheSwappedFootprint)
	{
		Fixture.InitGrid(4, 3);
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(3, 1));
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Rifle, 1, Fixture.SlotAt(0, 0));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		// Horizontal, a 3x1 at column 3 would overflow the 4 columns. Rotated it is 1x3 and fits.
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(3, 0), Params)));

		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(3, 0)).ItemHandle));
	}

	TEST_METHOD(Move_RotatedPastTheBottomEdge_FailsAndStaysPut)
	{
		Fixture.InitGrid(4, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Item cannot be moved to target location"), ELogVerbosity::Warning);
		URockItemDefinition* Plank = Fixture.MakeDefinition("Plank", 1, FIntPoint(2, 1));
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Plank, 1, Fixture.SlotAt(0, 0));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 0), Params)));

		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
	}

	TEST_METHOD(Move_OntoARotatedItemsSwappedCells_Fails)
	{
		Fixture.InitGrid(4, 4);
		TestRunner->AddExpectedMessagePlain(TEXT("Item cannot be moved to target location"), ELogVerbosity::Warning);
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(3, 1));
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Rifle, 1, Fixture.SlotAt(0, 0));
		SetOrientation(Fixture.SlotAt(0, 0), ERockItemOrientation::Vertical);
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(3, 3));

		// Rotated, the rifle covers (0,0)-(0,2), so (0,2) is taken and (1,0) is free.
		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(3, 3), Fixture.Inventory, Fixture.SlotAt(0, 2))));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(3, 3), Fixture.Inventory, Fixture.SlotAt(1, 0))));
	}

	TEST_METHOD(Move_RotateInPlace_ChangesOrientationWhenItFits)
	{
		Fixture.InitGrid(4, 4);
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(3, 1));
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Rifle, 1, Fixture.SlotAt(0, 0));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(0, 0), Params)));

		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
		ASSERT_THAT(AreEqual(ERockItemOrientation::Vertical, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).Orientation));
	}

	TEST_METHOD(Move_RotateInPlace_IsRefusedWhenTheRotatedFootprintIsBlocked)
	{
		Fixture.InitGrid(4, 4);
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(3, 1));
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Rifle, 1, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 1));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(0, 0), Params)));

		ASSERT_THAT(AreEqual(ERockItemOrientation::Horizontal, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).Orientation));
	}

	TEST_METHOD(Move_AcrossInventories_TransfersTheItemAndStalesTheOldHandle)
	{
		Fixture.InitGrid(2, 1);
		Other.InitGrid(2, 2);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle OldHandle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Other.Inventory, Other.SlotAt(1, 1))));

		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
		ASSERT_THAT(IsTrue(IsSlotEmpty(Fixture, Fixture.SlotAt(0, 0))));
		ASSERT_THAT(IsFalse(Fixture.Inventory->IsHandleValid(OldHandle)));
		ASSERT_THAT(AreEqual(1, Other.Inventory->GetNumItemStacks()));
		const FRockItemStack Arrived = Other.Inventory->GetItemBySlotHandle(Other.SlotAt(1, 1));
		ASSERT_THAT(IsTrue(Arrived.GetDefinition() == Apple));
		ASSERT_THAT(AreEqual(1, Arrived.GetStackCount()));
	}

	TEST_METHOD(Move_AcrossInventories_PartialMoveLeavesTheRestBehind)
	{
		Fixture.InitGrid(1, 1);
		Other.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 8, Fixture.SlotAt(0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::CustomAmount;
		Params.MoveCount = 5;

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Other.Inventory, Other.SlotAt(0, 0), Params)));

		ASSERT_THAT(AreEqual(3, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(5, Other.Inventory->GetItemBySlotHandle(Other.SlotAt(0, 0)).GetStackCount()));
	}

	TEST_METHOD(Move_AcrossInventories_IgnoresHandleIndexCollisionsInTheTarget)
	{
		// Both stacks get ItemData index 0. The source's handle must not be mistaken for the target's occupant.
		Fixture.InitGrid(1, 1);
		Other.InitGrid(2, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Item cannot be moved to target location"), ELogVerbosity::Warning);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		URockItemDefinition* Pear = Fixture.MakeDefinition("Pear");
		const FRockItemStackHandle ApplesHandle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		const FRockItemStackHandle PearsHandle = Other.PlaceAt(Pear, 1, Other.SlotAt(0, 0));
		ASSERT_THAT(AreEqual(ApplesHandle, PearsHandle));

		ASSERT_THAT(IsFalse(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Other.Inventory, Other.SlotAt(0, 0))));

		ASSERT_THAT(IsTrue(Other.Inventory->GetItemBySlotHandle(Other.SlotAt(0, 0)).GetDefinition() == Pear));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
