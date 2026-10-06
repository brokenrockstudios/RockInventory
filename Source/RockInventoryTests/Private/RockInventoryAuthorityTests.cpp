// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Library/RockInventoryLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// Replicated inventory state is written on the authority only. A client (an owner whose role is not Authority) is refused
// and the inventory stays as it was. The role is faked with AActor::SetRole on the inventory's owner.
// AddItemToInventory's own guard is an ensure, which a test cannot declare as expected, so it has no test here (DevNotes).
TEST_CLASS(RockInventoryAuthorityTests, "BRS.RockInventory.Authority")
{
	FRockInventoryFixture Fixture;
	URockItemDefinition* Apple = nullptr;

	BEFORE_EACH()
	{
		Fixture.InitGrid(4, 1);
		Apple = Fixture.MakeDefinition("Apple", 5);
	}

	void BecomeClient()
	{
		Fixture.Owner->SetRole(ROLE_SimulatedProxy);
		ASSERT_THAT(IsFalse(Fixture.Owner->HasAuthority()));
	}

	void ExpectRefusal(const TCHAR* Operation)
	{
		TestRunner->AddExpectedMessagePlain(FString::Printf(TEXT("%s - refused"), Operation), ELogVerbosity::Warning);
	}

	TEST_METHOD(Loot_OnAClient_IsRefused)
	{
		BecomeClient();
		ExpectRefusal(TEXT("LootItemToInventory"));
		FRockInventorySlotHandle Slot;
		int32 Excess = 0;

		const bool bPlaced = URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Apple, 2), Slot, Excess);

		ASSERT_THAT(IsFalse(bPlaced));
		ASSERT_THAT(AreEqual(2, Excess));
		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Move_OnAClient_IsRefused)
	{
		const FRockInventorySlotHandle From = Fixture.SlotAt(0, 0);
		const FRockInventorySlotHandle To = Fixture.SlotAt(1, 0);
		Fixture.PlaceAt(Apple, 1, From);
		BecomeClient();
		ExpectRefusal(TEXT("MoveItem"));

		const bool bMoved = URockInventoryLibrary::MoveItem(Fixture.Inventory, From, Fixture.Inventory, To);

		ASSERT_THAT(IsFalse(bMoved));
		ASSERT_THAT(IsTrue(Fixture.Inventory->GetSlotByHandle(From).ItemHandle.IsValid()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetSlotByHandle(To).ItemHandle.IsValid()));
	}

	TEST_METHOD(Split_OnAClient_IsRefused)
	{
		const FRockInventorySlotHandle From = Fixture.SlotAt(0, 0);
		Fixture.PlaceAt(Apple, 4, From);
		BecomeClient();
		ExpectRefusal(TEXT("SplitItemStackAtLocation"));

		const FRockItemStack Taken = URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, From, 1);

		ASSERT_THAT(IsFalse(Taken.IsValid()));
		ASSERT_THAT(AreEqual(4, Fixture.Inventory->GetItemBySlotHandle(From).GetStackCount()));
	}

	TEST_METHOD(Merge_OnAClient_IsRefused)
	{
		const FRockInventorySlotHandle Target = Fixture.SlotAt(0, 0);
		Fixture.PlaceAt(Apple, 2, Target);
		BecomeClient();
		ExpectRefusal(TEXT("MergeItemAtGridPosition"));

		const int32 Remaining = URockInventoryLibrary::MergeItemAtGridPosition(Fixture.Inventory, Target, FRockItemStack(Apple, 2));

		ASSERT_THAT(AreEqual(2, Remaining));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetItemBySlotHandle(Target).GetStackCount()));
	}

	TEST_METHOD(SetCustomValue_OnAClient_IsRefused)
	{
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		BecomeClient();
		ExpectRefusal(TEXT("SetCustomValue1"));
		ExpectRefusal(TEXT("SetCustomValue2"));

		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, Handle, 7);
		URockInventoryLibrary::SetCustomValue2(Fixture.Inventory, Handle, 9);

		const FRockItemStack Stack = Fixture.Inventory->GetItemByHandle(Handle);
		ASSERT_THAT(AreEqual(0, Stack.GetCustomValue1()));
		ASSERT_THAT(AreEqual(0, Stack.GetCustomValue2()));
	}

	TEST_METHOD(Loot_OnTheAuthority_StillWorks)
	{
		FRockInventorySlotHandle Slot;
		int32 Excess = 0;

		const bool bPlaced = URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Apple, 2), Slot, Excess);

		ASSERT_THAT(IsTrue(bPlaced));
		ASSERT_THAT(AreEqual(0, Excess));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
