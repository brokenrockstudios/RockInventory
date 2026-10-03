// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"
#include "RockInventoryTestListener.h"

#include "Library/RockInventoryLibrary.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// What listeners (UI, equipment, quest tracking) are told when the inventory changes.
// Each test arranges its scenario first and starts listening afterwards, so only the action under test is recorded.
TEST_CLASS(RockInventoryEventTests, "BRS.RockInventory.Events")
{
	FRockInventoryFixture Fixture;
	FRockInventoryFixture Other;
	TStrongObjectPtr<URockInventoryTestListener> ListenerPtr;

	URockInventoryTestListener* Listen(const FRockInventoryFixture& Target)
	{
		URockInventoryTestListener* Listener = NewObject<URockInventoryTestListener>(GetTransientPackage());
		ListenerPtr.Reset(Listener);
		Target.Inventory->OnSlotChanged.AddDynamic(Listener, &URockInventoryTestListener::OnSlotChanged);
		Target.Inventory->OnItemChanged.AddDynamic(Listener, &URockInventoryTestListener::OnItemChanged);
		return Listener;
	}

	TEST_METHOD(Loot_NewItem_BroadcastsItemAddedThenSlotItemAdded)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		URockInventoryTestListener* Listener = Listen(Fixture);

		FRockInventorySlotHandle Slot;
		int32 Excess = 0;
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));

		ASSERT_THAT(AreEqual(1, Listener->ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Added, Listener->ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(Fixture.Inventory->GetSlotByHandle(Slot).ItemHandle, Listener->ItemDeltas[0].ItemHandle));
		ASSERT_THAT(AreEqual(1, Listener->SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemAdded, Listener->SlotDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(Slot, Listener->SlotDeltas[0].SlotHandle));
	}

	TEST_METHOD(Loot_MergingIntoAStack_BroadcastsOnlyAnItemChange)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture);

		FRockInventorySlotHandle Slot;
		int32 Excess = 0;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 2, Slot, Excess)));

		ASSERT_THAT(AreEqual(1, Listener->ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Changed, Listener->ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(0, Listener->SlotDeltas.Num()));
	}

	TEST_METHOD(SetItemStackCount_BroadcastsItemChanged)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture);

		Fixture.Inventory->SetItemStackCount(Handle, 8);

		ASSERT_THAT(AreEqual(1, Listener->ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Changed, Listener->ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(Handle, Listener->ItemDeltas[0].ItemHandle));
	}

	TEST_METHOD(Remove_BroadcastsItemRemovedWithTheOldHandle)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture);

		Fixture.Inventory->RemoveItemFromInventory(Handle);

		ASSERT_THAT(AreEqual(1, Listener->ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Removed, Listener->ItemDeltas[0].ChangeType));
		// Listeners need the handle the item had, not the already-bumped replacement.
		ASSERT_THAT(AreEqual(Handle, Listener->ItemDeltas[0].ItemHandle));
	}

	TEST_METHOD(SplitWholeStack_BroadcastsItemRemovedAndSlotItemRemoved)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture);

		URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Fixture.SlotAt(0, 0));

		ASSERT_THAT(AreEqual(1, Listener->ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Removed, Listener->ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(1, Listener->SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemRemoved, Listener->SlotDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(Handle, Listener->SlotDeltas[0].PreviousItemHandle));
	}

	TEST_METHOD(SetSlot_OrientationOnly_BroadcastsPropertiesChanged)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture);

		FRockInventorySlotEntry Entry = Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0));
		Entry.Orientation = ERockItemOrientation::Vertical;
		Fixture.Inventory->SetSlotByHandle(Fixture.SlotAt(0, 0), Entry);

		ASSERT_THAT(AreEqual(1, Listener->SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::PropertiesChanged, Listener->SlotDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(0, Listener->ItemDeltas.Num()));
	}

	TEST_METHOD(SetSlot_Unchanged_BroadcastsNothing)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture);

		Fixture.Inventory->SetSlotByHandle(Fixture.SlotAt(0, 0), Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)));

		ASSERT_THAT(AreEqual(0, Listener->SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(0, Listener->ItemDeltas.Num()));
	}

	TEST_METHOD(Move_BetweenSlots_BroadcastsRemovalFromSourceThenAdditionToTarget)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture);

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(2, Listener->SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Listener->SlotDeltas[0].SlotHandle));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemRemoved, Listener->SlotDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Listener->SlotDeltas[1].SlotHandle));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemAdded, Listener->SlotDeltas[1].ChangeType));
		// Same inventory, so the stack itself is untouched.
		ASSERT_THAT(AreEqual(0, Listener->ItemDeltas.Num()));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Handle)));
	}

	TEST_METHOD(Move_AcrossInventories_NotifiesEachInventoryOfItsOwnSide)
	{
		Fixture.InitGrid(1, 1);
		Other.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* SourceListener = Listen(Fixture);
		TStrongObjectPtr<URockInventoryTestListener> SourceListenerKeepAlive(SourceListener);
		URockInventoryTestListener* TargetListener = Listen(Other);

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Other.Inventory, Other.SlotAt(0, 0))));

		ASSERT_THAT(AreEqual(1, SourceListener->ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Removed, SourceListener->ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(1, TargetListener->ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Added, TargetListener->ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemRemoved, SourceListener->SlotDeltas.Last().ChangeType));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemAdded, TargetListener->SlotDeltas.Last().ChangeType));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
