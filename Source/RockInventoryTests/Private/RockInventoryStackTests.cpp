// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "RockInventoryTestFragments.h"
#include "StructUtils/InstancedStruct.h"
#include "Library/RockItemStackLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// Value-level rules for FRockItemStack and the helpers around it. No inventory involved.
TEST_CLASS(RockInventoryStackTests, "BRS.RockInventory.Stack")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(Stack_WithDefinitionAndCount_IsValid)
	{
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);
		const FRockItemStack Stack(Apple, 3);

		ASSERT_THAT(IsTrue(Stack.IsValid()));
		ASSERT_THAT(AreEqual(3, Stack.GetStackCount()));
		ASSERT_THAT(AreEqual(5, Stack.GetMaxStackCount()));
		ASSERT_THAT(AreEqual(FName("Apple"), Stack.GetItemId()));
	}

	TEST_METHOD(Stack_Invalid_HasNoIdentityOrCapacity)
	{
		const FRockItemStack& Stack = FRockItemStack::Invalid();

		ASSERT_THAT(IsFalse(Stack.IsValid()));
		ASSERT_THAT(IsTrue(Stack.IsEmpty()));
		ASSERT_THAT(AreEqual(0, Stack.GetMaxStackCount()));
		ASSERT_THAT(AreEqual(FName(NAME_None), Stack.GetItemId()));
	}

	TEST_METHOD(CanStackWith_SameDefinitionAndValues_IsTrue)
	{
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);

		ASSERT_THAT(IsTrue(FRockItemStack(Apple, 1).CanStackWith(FRockItemStack(Apple, 2))));
	}

	TEST_METHOD(CanStackWith_DifferentDefinitions_IsFalse)
	{
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);
		URockItemDefinition* Pear = Fixture.MakeDefinition("Pear", 5);

		ASSERT_THAT(IsFalse(FRockItemStack(Apple, 1).CanStackWith(FRockItemStack(Pear, 1))));
	}

	TEST_METHOD(CanStackWith_EmptyStack_IsFalse)
	{
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);

		ASSERT_THAT(IsFalse(FRockItemStack(Apple, 1).CanStackWith(FRockItemStack::Invalid())));
		ASSERT_THAT(IsFalse(FRockItemStack::Invalid().CanStackWith(FRockItemStack(Apple, 1))));
	}

	TEST_METHOD(CanStackWith_DifferentCustomValues_IsFalse)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Battery = Fixture.MakeDefinition("Battery", 5);
		const FRockItemStackHandle Charged = Fixture.PlaceAt(Battery, 1, Fixture.SlotAt(0, 0));
		const FRockItemStackHandle Empty = Fixture.PlaceAt(Battery, 1, Fixture.SlotAt(1, 0));
		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, Charged, 100);

		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemByHandle(Charged).CanStackWith(Fixture.Inventory->GetItemByHandle(Empty))));
		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, Empty, 100);
		ASSERT_THAT(IsTrue(Fixture.Inventory->GetItemByHandle(Charged).CanStackWith(Fixture.Inventory->GetItemByHandle(Empty))));
		URockInventoryLibrary::SetCustomValue2(Fixture.Inventory, Empty, 1);
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemByHandle(Charged).CanStackWith(Fixture.Inventory->GetItemByHandle(Empty))));
	}

	TEST_METHOD(CanStackWith_AFragmentThatVetoes_IsFalse)
	{
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);
		Apple->Fragments.Add(FInstancedStruct::Make(FRockTestFragment_NeverCombine()));

		ASSERT_THAT(IsFalse(FRockItemStack(Apple, 1).CanStackWith(FRockItemStack(Apple, 1))));
	}

	TEST_METHOD(CanStackWith_AFragmentThatAllows_IsTrue)
	{
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);
		Apple->Fragments.Add(FInstancedStruct::Make(FRockItemFragment()));

		ASSERT_THAT(IsTrue(FRockItemStack(Apple, 1).CanStackWith(FRockItemStack(Apple, 1))));
	}

	TEST_METHOD(CanStackWith_OneVetoAmongSeveralFragments_IsFalse)
	{
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);
		Apple->Fragments.Add(FInstancedStruct::Make(FRockItemFragment()));
		Apple->Fragments.Add(FInstancedStruct::Make(FRockTestFragment_NeverCombine()));

		ASSERT_THAT(IsFalse(FRockItemStack(Apple, 1).CanStackWith(FRockItemStack(Apple, 1))));
	}

	TEST_METHOD(CanStackWith_AFragmentThatLooksAtTheStacks_DecidesPerPair)
	{
		Fixture.InitGrid(4, 1);
		URockItemDefinition* Battery = Fixture.MakeDefinition("Battery", 5);
		FRockTestFragment_CombineLimit Limit;
		Limit.Limit = 50;
		Battery->Fragments.Add(FInstancedStruct::Make(Limit));
		const FRockItemStackHandle Low = Fixture.PlaceAt(Battery, 1, Fixture.SlotAt(0, 0));
		const FRockItemStackHandle AlsoLow = Fixture.PlaceAt(Battery, 1, Fixture.SlotAt(1, 0));
		const FRockItemStackHandle High = Fixture.PlaceAt(Battery, 1, Fixture.SlotAt(2, 0));
		const FRockItemStackHandle AlsoHigh = Fixture.PlaceAt(Battery, 1, Fixture.SlotAt(3, 0));
		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, Low, 10);
		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, AlsoLow, 10);
		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, High, 90);
		URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, AlsoHigh, 90);

		// Equal custom values stack by the base rule; only the fragment tells these two pairs apart
		ASSERT_THAT(IsTrue(Fixture.Inventory->GetItemByHandle(Low).CanStackWith(Fixture.Inventory->GetItemByHandle(AlsoLow))));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemByHandle(High).CanStackWith(Fixture.Inventory->GetItemByHandle(AlsoHigh))));
	}

	TEST_METHOD(CanStackWith_DifferentRuntimeInstances_IsFalse)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Pouch = Fixture.MakeDefinition("Pouch", 5);
		Pouch->RuntimeInstanceClass = URockItemInstance::StaticClass();
		const FRockItemStackHandle First = Fixture.PlaceAt(Pouch, 1, Fixture.SlotAt(0, 0));
		const FRockItemStackHandle Second = Fixture.PlaceAt(Pouch, 1, Fixture.SlotAt(1, 0));
		const FRockItemStack A = Fixture.Inventory->GetItemByHandle(First);
		const FRockItemStack B = Fixture.Inventory->GetItemByHandle(Second);

		ASSERT_THAT(IsTrue(A.GetRuntimeInstance() != nullptr));
		ASSERT_THAT(IsTrue(A.GetRuntimeInstance() != B.GetRuntimeInstance()));
		ASSERT_THAT(IsFalse(A.CanStackWith(B)));
		// The same instance is the same stack
		ASSERT_THAT(IsTrue(A.CanStackWith(A)));
	}

	TEST_METHOD(CanStackWith_AnInstancedStackAndOneWithoutAnInstanceYet_IsTrue)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Pouch = Fixture.MakeDefinition("Pouch", 5);
		Pouch->RuntimeInstanceClass = URockItemInstance::StaticClass();
		const FRockItemStack Placed = Fixture.Inventory->GetItemByHandle(Fixture.PlaceAt(Pouch, 1, Fixture.SlotAt(0, 0)));

		ASSERT_THAT(IsTrue(Placed.CanStackWith(FRockItemStack(Pouch, 1))));
	}

	TEST_METHOD(Loot_DoesNotMergeIntoAStackWhoseFragmentVetoes)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 5);
		Apple->Fragments.Add(FInstancedStruct::Make(FRockTestFragment_NeverCombine()));
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		FRockInventorySlotHandle Slot;
		int32 Excess = 0;

		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));

		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Slot));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Library_CanStackWith_AlsoRequiresRoomForBothStacks)
	{
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		ASSERT_THAT(IsTrue(URockItemStackLibrary::CanStackWith(FRockItemStack(Arrow, 6), FRockItemStack(Arrow, 4))));
		ASSERT_THAT(IsFalse(URockItemStackLibrary::CanStackWith(FRockItemStack(Arrow, 6), FRockItemStack(Arrow, 5))));
	}

	TEST_METHOD(Library_IsFull_TrueAtMax)
	{
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		ASSERT_THAT(IsFalse(URockItemStackLibrary::IsFull(FRockItemStack(Arrow, 9))));
		ASSERT_THAT(IsTrue(URockItemStackLibrary::IsFull(FRockItemStack(Arrow, 10))));
	}

	TEST_METHOD(Library_GetItemSize_ComesFromTheDefinitionGrid)
	{
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(5, 2));

		ASSERT_THAT(AreEqual(FIntPoint(5, 2), URockItemStackLibrary::GetItemSize(FRockItemStack(Rifle, 1))));
	}

	TEST_METHOD(Library_GetItemSizeForOrientation_VerticalSwapsTheAxes)
	{
		URockItemDefinition* Rifle = Fixture.MakeDefinition("Rifle", 1, FIntPoint(5, 2));
		const FRockItemStack Stack(Rifle, 1);

		ASSERT_THAT(AreEqual(FIntPoint(5, 2), URockItemStackLibrary::GetItemSizeForOrientation(Stack, ERockItemOrientation::Horizontal)));
		ASSERT_THAT(AreEqual(FIntPoint(2, 5), URockItemStackLibrary::GetItemSizeForOrientation(Stack, ERockItemOrientation::Vertical)));
	}

	TEST_METHOD(MoveAmount_FullStack_IsTheWholeStack)
	{
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		ASSERT_THAT(AreEqual(7, URockItemStackLibrary::CalculateMoveAmount(FRockItemStack(Arrow, 7), ERockItemMoveMode::FullStack, 0)));
	}

	TEST_METHOD(MoveAmount_SingleItem_IsOne)
	{
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		ASSERT_THAT(AreEqual(1, URockItemStackLibrary::CalculateMoveAmount(FRockItemStack(Arrow, 7), ERockItemMoveMode::SingleItem, 0)));
	}

	TEST_METHOD(MoveAmount_HalfStack_RoundsUp)
	{
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);

		ASSERT_THAT(AreEqual(3, URockItemStackLibrary::CalculateMoveAmount(FRockItemStack(Arrow, 5), ERockItemMoveMode::HalfStack, 0)));
		ASSERT_THAT(AreEqual(2, URockItemStackLibrary::CalculateMoveAmount(FRockItemStack(Arrow, 4), ERockItemMoveMode::HalfStack, 0)));
		ASSERT_THAT(AreEqual(1, URockItemStackLibrary::CalculateMoveAmount(FRockItemStack(Arrow, 1), ERockItemMoveMode::HalfStack, 0)));
	}

	TEST_METHOD(MoveAmount_CustomAmount_ClampsToTheStack)
	{
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStack Stack(Arrow, 7);

		ASSERT_THAT(AreEqual(3, URockItemStackLibrary::CalculateMoveAmount(Stack, ERockItemMoveMode::CustomAmount, 3)));
		ASSERT_THAT(AreEqual(7, URockItemStackLibrary::CalculateMoveAmount(Stack, ERockItemMoveMode::CustomAmount, 99)));
		ASSERT_THAT(AreEqual(0, URockItemStackLibrary::CalculateMoveAmount(Stack, ERockItemMoveMode::CustomAmount, -4)));
	}

	TEST_METHOD(MoveAmount_InvalidStack_IsZero)
	{
		ASSERT_THAT(AreEqual(0, URockItemStackLibrary::CalculateMoveAmount(FRockItemStack::Invalid(), ERockItemMoveMode::FullStack, 0)));
	}

	TEST_METHOD(Section_WithoutAFilter_AcceptsAnyItem)
	{
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		FRockInventorySectionInfo Section(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 2);
		Section.Initialize(0, 0);

		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemBePlacedInSection(FRockItemStack(Apple, 1), Section)));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
