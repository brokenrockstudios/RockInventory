// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Library/RockInventoryLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// Item stacks live in a fixed array and are addressed by generation-checked handles. Removing a stack must
// invalidate every handle that pointed at it, even after its array index is handed to a new stack.
TEST_CLASS(RockInventoryLifecycleTests, "BRS.RockInventory.Lifecycle")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(Remove_InvalidatesTheHandle)
	{
		Fixture.InitGrid(2, 2);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Handle)));

		Fixture.Inventory->RemoveItemFromInventory(Handle);

		ASSERT_THAT(IsFalse(Fixture.Inventory->IsHandleValid(Handle)));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemByHandle(Handle).IsValid()));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Remove_ThenAdd_ReusesTheIndexWithANewGeneration)
	{
		Fixture.InitGrid(2, 2);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle First = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		Fixture.Inventory->RemoveItemFromInventory(First);

		const FRockItemStackHandle Second = Fixture.Inventory->AddItemToInventory(FRockItemStack(Apple, 1));

		ASSERT_THAT(AreEqual(First.GetIndex(), Second.GetIndex()));
		ASSERT_THAT(AreEqual(First.GetGeneration() + 1, Second.GetGeneration()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->IsHandleValid(First)));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Second)));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemByHandle(First).IsValid()));
	}

	TEST_METHOD(Remove_FreedIndexesAreReusedBeforeTheArrayGrows)
	{
		Fixture.InitGrid(4, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle A = Fixture.Inventory->AddItemToInventory(FRockItemStack(Apple, 1));
		const FRockItemStackHandle B = Fixture.Inventory->AddItemToInventory(FRockItemStack(Apple, 1));
		ASSERT_THAT(AreEqual(0, A.GetIndex()));
		ASSERT_THAT(AreEqual(1, B.GetIndex()));

		Fixture.Inventory->RemoveItemFromInventory(A);
		const FRockItemStackHandle C = Fixture.Inventory->AddItemToInventory(FRockItemStack(Apple, 1));

		ASSERT_THAT(AreEqual(0, C.GetIndex()));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(B)));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Remove_WithAStaleHandle_WarnsAndLeavesTheNewItemAlone)
	{
		Fixture.InitGrid(2, 2);
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid item handle"), ELogVerbosity::Warning);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Stale = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		Fixture.Inventory->RemoveItemFromInventory(Stale);
		const FRockItemStackHandle Current = Fixture.Inventory->AddItemToInventory(FRockItemStack(Apple, 1));

		Fixture.Inventory->RemoveItemFromInventory(Stale);

		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Current)));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Remove_WithAnOutOfRangeHandle_Warns)
	{
		Fixture.InitGrid(2, 2);
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid index"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);

		Fixture.Inventory->RemoveItemFromInventory(FRockItemStackHandle::Create(500, 0));
		Fixture.Inventory->RemoveItemFromInventory(FRockItemStackHandle::Invalid());

		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(SetItemStackCount_Positive_UpdatesTheStack)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 20);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(0, 0));

		Fixture.Inventory->SetItemStackCount(Handle, 12);

		ASSERT_THAT(AreEqual(12, Fixture.Inventory->GetItemByHandle(Handle).GetStackCount()));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Handle)));
	}

	TEST_METHOD(SetItemStackCount_Zero_RemovesTheItem)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 20);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Arrow, 5, Fixture.SlotAt(0, 0));

		Fixture.Inventory->SetItemStackCount(Handle, 0);

		ASSERT_THAT(IsFalse(Fixture.Inventory->IsHandleValid(Handle)));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(SetItemByHandle_WithAStaleHandle_WarnsAndDoesNotOverwrite)
	{
		Fixture.InitGrid(2, 2);
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid or stale item handle"), ELogVerbosity::Warning);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 10);
		const FRockItemStackHandle Stale = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		Fixture.Inventory->RemoveItemFromInventory(Stale);
		const FRockItemStackHandle Current = Fixture.Inventory->AddItemToInventory(FRockItemStack(Apple, 2));

		Fixture.Inventory->SetItemByHandle(Stale, FRockItemStack(Apple, 9));

		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetItemByHandle(Current).GetStackCount()));
	}

	TEST_METHOD(CopyDataFrom_KeepsTheDestinationsInitializedFlag)
	{
		// The init flag is not part of the copied data. Overwriting a stored item must not reset it.
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 10);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		FRockItemStack Stored = Fixture.Inventory->GetItemByHandle(Handle);
		ASSERT_THAT(IsTrue(Stored.IsInitialized()));

		const FRockItemStack Fresh(Apple, 9);
		ASSERT_THAT(IsFalse(Fresh.IsInitialized()));
		Stored.CopyDataFrom(Fresh);

		ASSERT_THAT(AreEqual(9, Stored.GetStackCount()));
		ASSERT_THAT(IsTrue(Stored.IsInitialized()));
	}

	TEST_METHOD(CopyDataFrom_DoesNotMarkAnUninitializedDestinationInitialized)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 10);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		const FRockItemStack Stored = Fixture.Inventory->GetItemByHandle(Handle);

		FRockItemStack Fresh(Apple, 1);
		Fresh.CopyDataFrom(Stored);

		ASSERT_THAT(IsFalse(Fresh.IsInitialized()));
	}

	TEST_METHOD(SetItemByHandle_KeepsTheItemInitialized)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 10);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));

		Fixture.Inventory->SetItemStackCount(Handle, 5);
		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, Handle, 3);

		ASSERT_THAT(IsTrue(Fixture.Inventory->GetItemByHandle(Handle).IsInitialized()));
	}

	TEST_METHOD(SetItemCustomValueByTag_WritesTheValueSlotTheDefinitionDeclares)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Gun = Fixture.MakeDefinition("Gun");
		Gun->CustomValue1Tag = RockInventoryTags::Inventory_Section_Backpack;
		Gun->CustomValue2Tag = RockInventoryTags::Inventory_Section_Pockets;
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Gun, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsTrue(Fixture.Inventory->SetItemCustomValueByTag(Handle, RockInventoryTags::Inventory_Section_Backpack, 7)));
		ASSERT_THAT(IsTrue(Fixture.Inventory->SetItemCustomValueByTag(Handle, RockInventoryTags::Inventory_Section_Pockets, 9)));

		const FRockItemStack Stack = Fixture.Inventory->GetItemByHandle(Handle);
		ASSERT_THAT(AreEqual(7, Stack.GetCustomValue1()));
		ASSERT_THAT(AreEqual(9, Stack.GetCustomValue2()));
	}

	TEST_METHOD(SetItemCustomValueByTag_UndeclaredTag_ReturnsFalseAndChangesNothing)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Gun = Fixture.MakeDefinition("Gun");
		Gun->CustomValue1Tag = RockInventoryTags::Inventory_Section_Backpack;
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Gun, 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(IsFalse(Fixture.Inventory->SetItemCustomValueByTag(Handle, RockInventoryTags::Inventory_Section_Pockets, 7)));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetItemByHandle(Handle).GetCustomValue1()));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetItemByHandle(Handle).GetCustomValue2()));
	}

	TEST_METHOD(GetItemCount_SumsAcrossStacksOfTheSameItem)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 10);
		URockItemDefinition* Pear = Fixture.MakeDefinition("Pear", 10);
		Fixture.PlaceAt(Apple, 3, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Apple, 4, Fixture.SlotAt(1, 0));
		Fixture.PlaceAt(Pear, 2, Fixture.SlotAt(2, 0));

		ASSERT_THAT(AreEqual(7, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Apple")));
		ASSERT_THAT(AreEqual(2, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Pear")));
		ASSERT_THAT(AreEqual(0, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Nothing")));
	}

	TEST_METHOD(ItemCounts_QuantitySumsUnitsWhileNumItemStacksCountsStacks)
	{
		// Quantity is units across all stacks, NumItemStacks is the number of stacks.
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 10);
		Fixture.PlaceAt(Apple, 3, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Apple, 4, Fixture.SlotAt(1, 0));

		ASSERT_THAT(AreEqual(7, Fixture.Inventory->GetTotalItemQuantity()));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(SlotStatus_RegisterThenRelease_RoundTrips)
	{
		Fixture.InitGrid(2, 1);
		const FRockInventorySlotHandle Slot = Fixture.SlotAt(0, 0);
		ASSERT_THAT(AreEqual(ERockSlotStatus::Empty, Fixture.Inventory->GetSlotStatus(Slot)));

		Fixture.Inventory->RegisterSlotStatus(nullptr, Slot, ERockSlotStatus::Pending);
		ASSERT_THAT(AreEqual(ERockSlotStatus::Pending, Fixture.Inventory->GetSlotStatus(Slot)));
		ASSERT_THAT(AreEqual(ERockSlotStatus::Empty, Fixture.Inventory->GetSlotStatus(Fixture.SlotAt(1, 0))));

		Fixture.Inventory->ReleaseSlotStatus(nullptr, Slot);
		ASSERT_THAT(AreEqual(ERockSlotStatus::Empty, Fixture.Inventory->GetSlotStatus(Slot)));
	}

	TEST_METHOD(SlotStatus_ClaimingAnAlreadyPendingSlot_DoesNotStackClaims)
	{
		Fixture.InitGrid(2, 1);
		const FRockInventorySlotHandle Slot = Fixture.SlotAt(0, 0);

		Fixture.Inventory->RegisterSlotStatus(nullptr, Slot, ERockSlotStatus::Pending);
		Fixture.Inventory->RegisterSlotStatus(nullptr, Slot, ERockSlotStatus::Pending);
		// A duplicate claim would leave a second entry behind, and the slot would still read Pending here.
		Fixture.Inventory->ReleaseSlotStatus(nullptr, Slot);

		ASSERT_THAT(AreEqual(ERockSlotStatus::Empty, Fixture.Inventory->GetSlotStatus(Slot)));
	}

	TEST_METHOD(SlotStatus_RecordsTheItemThatWasInTheSlot)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Item = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));

		Fixture.Inventory->RegisterSlotStatus(nullptr, Fixture.SlotAt(0, 0), ERockSlotStatus::Pending);

		const FRockPendingSlotOperation Pending = Fixture.Inventory->GetPendingSlotState(Fixture.SlotAt(0, 0));
		ASSERT_THAT(AreEqual(Item, Pending.ItemHandle));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetPendingSlotState(Fixture.SlotAt(1, 0)).ItemHandle.IsValid()));
	}

	TEST_METHOD(SlotStatus_InvalidHandle_IsIgnored)
	{
		Fixture.InitGrid(2, 1);

		Fixture.Inventory->RegisterSlotStatus(nullptr, FRockInventorySlotHandle::Invalid(), ERockSlotStatus::Pending);

		ASSERT_THAT(AreEqual(ERockSlotStatus::Empty, Fixture.Inventory->GetSlotStatus(FRockInventorySlotHandle::Invalid())));
	}
};

// Splitting takes units out of a slot and hands back an unplaced stack.
TEST_CLASS(RockInventorySplitTests, "BRS.RockInventory.Split")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(Split_Partial_LeavesTheRemainderAndReturnsAnUnplacedStack)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Arrow, 10, Fixture.SlotAt(0, 0));

		const FRockItemStack Taken = URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Fixture.SlotAt(0, 0), 4);

		ASSERT_THAT(AreEqual(4, Taken.GetStackCount()));
		ASSERT_THAT(IsTrue(Taken.GetDefinition() == Arrow));
		// The split-off stack isn't in any inventory yet, so it must not alias the source's handle.
		ASSERT_THAT(IsFalse(Taken.ItemHandle.IsValid()));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Handle)));
		ASSERT_THAT(AreEqual(6, Fixture.Inventory->GetItemByHandle(Handle).GetStackCount()));
		ASSERT_THAT(AreEqual(Handle, Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
	}

	TEST_METHOD(Split_DefaultQuantity_TakesTheWholeStackAndEmptiesTheSlot)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Arrow, 10, Fixture.SlotAt(0, 0));

		const FRockItemStack Taken = URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Fixture.SlotAt(0, 0));

		ASSERT_THAT(AreEqual(10, Taken.GetStackCount()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle.IsValid()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->IsHandleValid(Handle)));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Split_QuantityAboveTheStack_TakesTheWholeStack)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));

		const FRockItemStack Taken = URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Fixture.SlotAt(0, 0), 99);

		ASSERT_THAT(AreEqual(3, Taken.GetStackCount()));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Split_FromAnEmptySlot_ReturnsInvalidAndWarns)
	{
		Fixture.InitGrid(2, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("No valid item at slot"), ELogVerbosity::Warning);

		const FRockItemStack Taken = URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Fixture.SlotAt(0, 0), 1);

		ASSERT_THAT(IsFalse(Taken.IsValid()));
	}

	TEST_METHOD(Split_FromAnOutOfRangeSlot_ReturnsInvalidAndWarns)
	{
		Fixture.InitGrid(2, 1);
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid SlotHandle"), ELogVerbosity::Warning);

		const FRockItemStack Taken = URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, FRockInventorySlotHandle(99), 1);

		ASSERT_THAT(IsFalse(Taken.IsValid()));
	}

	TEST_METHOD(Split_NullInventory_ReturnsInvalidAndWarns)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("Invalid Inventory"), ELogVerbosity::Warning);

		const FRockItemStack Taken = URockInventoryLibrary::SplitItemStackAtLocation(nullptr, FRockInventorySlotHandle(0), 1);

		ASSERT_THAT(IsFalse(Taken.IsValid()));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
