// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Inventory/RockInventoryData.h"
#include "Item/Fragment/RockItemFragment_SetStats.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Misc/RockInventoryTags.h"
#include "RockInventoryTestTags.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace
{
	/** One plain-data inventory with a single grid section. No world, no URockInventory. */
	struct FAddRemoveScenario
	{
		FRockInventoryData Data;
		FRockInventoryChangeSet Changes;
		TArray<TStrongObjectPtr<UObject>> KeepAlive;

		void Init(int32 Columns, int32 Rows, ERockItemSizePolicy Policy = ERockItemSizePolicy::RespectSize)
		{
			Data.Init({FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, Columns, Rows, Policy)});
		}

		FRockInventorySlotHandle At(int32 Column, int32 Row) const
		{
			const FRockInventorySectionInfo& Section = Data.Sections[0];
			return FRockInventorySlotHandle(Section.GetFirstSlotIndex() + Row * Section.GetColumns() + Column);
		}

		URockItemDefinition* Def(FName Id, int32 MaxStack = 1, FIntPoint Size = FIntPoint(1, 1))
		{
			URockItemDefinition* Definition = NewObject<URockItemDefinition>(GetTransientPackage());
			Definition->ItemId = Id;
			Definition->MaxStackCount = MaxStack;
			Definition->GridSize = Size;
			KeepAlive.Emplace(Definition);
			return Definition;
		}

		int32 CountAt(const FRockInventorySlotHandle& Slot) const
		{
			const FRockItemStack* Stack = Data.GetSlotStack(Slot);
			return Stack ? Stack->GetStackCount() : 0;
		}

		int32 NumStacks() const
		{
			int32 Num = 0;
			for (const FRockItemStack& Stack : Data.Stacks)
			{
				Num += Stack.IsValid() ? 1 : 0;
			}
			return Num;
		}

		int32 Add(const FRockInventorySlotHandle& Slot, URockItemDefinition* Definition, int32 Count, ERockAddRefusal& OutRefusal,
			ERockItemOrientation Orientation = ERockItemOrientation::Horizontal)
		{
			int32 Added = -1;
			OutRefusal = Data.ApplyAdd(Slot, FRockItemStack(Definition, Count), Orientation, Changes, Added);
			return Added;
		}
	};

	TFunction<bool(const FRockItemStack&)> IsItem(FName Id)
	{
		return [Id](const FRockItemStack& Stack) { return Stack.GetItemId() == Id; };
	}
}

// FRockInventoryData::CanAdd / ApplyAdd / CanRemove / ApplyRemove / CountMatching / RemoveMatching with no inventory object, events or world.
TEST_CLASS(RockInventoryDataAddRemoveTests, "BRS.RockInventory.Data")
{
	FAddRemoveScenario S;
	ERockAddRefusal AddRefusal = ERockAddRefusal::None;

	TEST_METHOD(Add_ToAnEmptyCell_CreatesTheStackThenSetsTheSlot)
	{
		S.Init(4, 2);

		const int32 Added = S.Add(S.At(1, 1), S.Def("Apple", 5), 3, AddRefusal);

		ASSERT_THAT(AreEqual(ERockAddRefusal::None, AddRefusal));
		ASSERT_THAT(AreEqual(3, Added));
		ASSERT_THAT(AreEqual(3, S.CountAt(S.At(1, 1))));
		ASSERT_THAT(AreEqual(2, S.Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(S.Changes.Changes[0].Type == ERockDataChangeType::StackCreated));
		ASSERT_THAT(IsTrue(S.Changes.Changes[0].Side == ERockInventorySide::Target));
		ASSERT_THAT(IsTrue(S.Changes.Changes[1].Type == ERockDataChangeType::Slot));
		ASSERT_THAT(AreEqual(S.At(1, 1), S.Changes.Changes[1].Slot));
		ASSERT_THAT(AreEqual(S.Changes.Changes[0].Stack, S.Changes.Changes[1].SlotAfter.ItemHandle));
	}

	TEST_METHOD(Add_RecordsTheOrientation)
	{
		S.Init(4, 4);

		S.Add(S.At(0, 0), S.Def("Rifle", 1, FIntPoint(2, 1)), 1, AddRefusal, ERockItemOrientation::Vertical);

		ASSERT_THAT(AreEqual(ERockAddRefusal::None, AddRefusal));
		ASSERT_THAT(IsTrue(S.Data.GetSlot(S.At(0, 0))->Orientation == ERockItemOrientation::Vertical));
	}

	TEST_METHOD(Add_MoreThanTheMaxStack_AddsTheMaxAndReportsIt)
	{
		S.Init(2, 1);

		const int32 Added = S.Add(S.At(0, 0), S.Def("Arrow", 10), 25, AddRefusal);

		ASSERT_THAT(AreEqual(ERockAddRefusal::None, AddRefusal));
		ASSERT_THAT(AreEqual(10, Added));
		ASSERT_THAT(AreEqual(10, S.CountAt(S.At(0, 0))));
	}

	TEST_METHOD(Add_ToACellWhereTheFootprintDoesNotFit_IsRefusedAndChangesNothing)
	{
		S.Init(2, 1);

		const int32 Added = S.Add(S.At(1, 0), S.Def("Crate", 1, FIntPoint(2, 1)), 1, AddRefusal);

		ASSERT_THAT(AreEqual(ERockAddRefusal::NoRoom, AddRefusal));
		ASSERT_THAT(AreEqual(0, Added));
		ASSERT_THAT(AreEqual(0, S.NumStacks()));
		ASSERT_THAT(IsTrue(S.Changes.IsEmpty()));
	}

	TEST_METHOD(Add_ToACellCoveredByAnotherItemsFootprint_IsRefused)
	{
		S.Init(3, 1);
		S.Data.PlaceStack(FRockItemStack(S.Def("Crate", 1, FIntPoint(2, 1)), 1), S.At(0, 0));

		S.Add(S.At(1, 0), S.Def("Apple"), 1, AddRefusal);

		ASSERT_THAT(AreEqual(ERockAddRefusal::NoRoom, AddRefusal));
		ASSERT_THAT(AreEqual(1, S.NumStacks()));
	}

	TEST_METHOD(Add_OnAMatchingStack_TopsItUpToTheMax)
	{
		S.Init(2, 1);
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		S.Data.PlaceStack(FRockItemStack(Arrow, 8), S.At(0, 0));

		const int32 Added = S.Add(S.At(0, 0), Arrow, 5, AddRefusal);

		ASSERT_THAT(AreEqual(ERockAddRefusal::None, AddRefusal));
		ASSERT_THAT(AreEqual(2, Added));
		ASSERT_THAT(AreEqual(10, S.CountAt(S.At(0, 0))));
		ASSERT_THAT(AreEqual(1, S.NumStacks()));
		ASSERT_THAT(AreEqual(1, S.Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(S.Changes.Changes[0].Type == ERockDataChangeType::StackModified));
		ASSERT_THAT(AreEqual(8, S.Changes.Changes[0].StackBefore.GetStackCount()));
		ASSERT_THAT(AreEqual(10, S.Changes.Changes[0].StackAfter.GetStackCount()));
	}

	TEST_METHOD(Add_OnAFullStack_HasNothingToMerge)
	{
		S.Init(2, 1);
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		S.Data.PlaceStack(FRockItemStack(Arrow, 10), S.At(0, 0));

		S.Add(S.At(0, 0), Arrow, 1, AddRefusal);

		ASSERT_THAT(AreEqual(ERockAddRefusal::NothingToMerge, AddRefusal));
		ASSERT_THAT(AreEqual(10, S.CountAt(S.At(0, 0))));
	}

	TEST_METHOD(Add_OnADifferentItem_IsRefused)
	{
		S.Init(2, 1);
		S.Data.PlaceStack(FRockItemStack(S.Def("Apple", 5), 1), S.At(0, 0));

		S.Add(S.At(0, 0), S.Def("Pear", 5), 1, AddRefusal);

		ASSERT_THAT(AreEqual(ERockAddRefusal::NoRoom, AddRefusal));
		ASSERT_THAT(AreEqual(1, S.CountAt(S.At(0, 0))));
	}

	TEST_METHOD(Add_IntoASectionThatRejectsTheItem_IsRefused)
	{
		FRockInventorySectionInfo Weapons(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 1);
		Weapons.SetSectionFilter(FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(RockInventoryTestTags::Weapon)));
		S.Data.Init({Weapons});

		S.Add(S.At(0, 0), S.Def("Apple"), 1, AddRefusal);

		ASSERT_THAT(AreEqual(ERockAddRefusal::SectionRejectsItem, AddRefusal));
	}

	TEST_METHOD(Add_WithAnInvalidStackOrSlot_IsRefused)
	{
		S.Init(2, 1);
		int32 Added = 0;

		ASSERT_THAT(AreEqual(ERockAddRefusal::InvalidStack, S.Data.ApplyAdd(S.At(0, 0), FRockItemStack::Invalid(), ERockItemOrientation::Horizontal, S.Changes, Added)));
		ASSERT_THAT(AreEqual(ERockAddRefusal::InvalidSlot, S.Data.ApplyAdd(FRockInventorySlotHandle(99), FRockItemStack(S.Def("Apple"), 1), ERockItemOrientation::Horizontal, S.Changes, Added)));
	}

	TEST_METHOD(CanAdd_ChangesNothing)
	{
		S.Init(2, 1);

		ASSERT_THAT(AreEqual(ERockAddRefusal::None, S.Data.CanAdd(S.At(0, 0), FRockItemStack(S.Def("Apple"), 1))));

		ASSERT_THAT(AreEqual(0, S.NumStacks()));
	}

	TEST_METHOD(Remove_PartOfAStack_ShrinksIt)
	{
		S.Init(2, 1);
		S.Data.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 7), S.At(0, 0));

		ASSERT_THAT(AreEqual(ERockRemoveRefusal::None, S.Data.ApplyRemove(S.At(0, 0), 3, S.Changes)));

		ASSERT_THAT(AreEqual(4, S.CountAt(S.At(0, 0))));
		ASSERT_THAT(AreEqual(1, S.Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(S.Changes.Changes[0].Type == ERockDataChangeType::StackModified));
		ASSERT_THAT(IsTrue(S.Changes.Changes[0].Side == ERockInventorySide::Source));
	}

	TEST_METHOD(Remove_TheWholeStack_ReleasesItAndClearsTheSlot)
	{
		S.Init(2, 1);
		const FRockItemStackHandle Handle = S.Data.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 7), S.At(0, 0));

		ASSERT_THAT(AreEqual(ERockRemoveRefusal::None, S.Data.ApplyRemove(S.At(0, 0), 7, S.Changes)));

		ASSERT_THAT(IsFalse(S.Data.GetSlot(S.At(0, 0))->ItemHandle.IsValid()));
		ASSERT_THAT(IsTrue(S.Data.GetStack(Handle) == nullptr));
		ASSERT_THAT(AreEqual(2, S.Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(S.Changes.Changes[0].Type == ERockDataChangeType::Slot));
		ASSERT_THAT(IsTrue(S.Changes.Changes[1].Type == ERockDataChangeType::StackRemoved));
		ASSERT_THAT(AreEqual(7, S.Changes.Changes[1].StackBefore.GetStackCount()));
	}

	TEST_METHOD(Remove_WithACountOfZero_TakesTheWholeStack)
	{
		S.Init(2, 1);
		S.Data.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 7), S.At(0, 0));

		ASSERT_THAT(AreEqual(ERockRemoveRefusal::None, S.Data.ApplyRemove(S.At(0, 0), 0, S.Changes)));

		ASSERT_THAT(AreEqual(0, S.NumStacks()));
	}

	TEST_METHOD(Remove_MoreThanTheStackHolds_IsRefusedAndChangesNothing)
	{
		S.Init(2, 1);
		S.Data.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 7), S.At(0, 0));

		ASSERT_THAT(AreEqual(ERockRemoveRefusal::NotEnoughItems, S.Data.ApplyRemove(S.At(0, 0), 8, S.Changes)));

		ASSERT_THAT(AreEqual(7, S.CountAt(S.At(0, 0))));
		ASSERT_THAT(IsTrue(S.Changes.IsEmpty()));
	}

	TEST_METHOD(Remove_FromAnEmptyOrUnknownSlot_IsRefused)
	{
		S.Init(2, 1);

		ASSERT_THAT(AreEqual(ERockRemoveRefusal::EmptySlot, S.Data.ApplyRemove(S.At(0, 0), 1, S.Changes)));
		ASSERT_THAT(AreEqual(ERockRemoveRefusal::InvalidSlot, S.Data.ApplyRemove(FRockInventorySlotHandle(99), 1, S.Changes)));
	}

	TEST_METHOD(Remove_ThenAdd_ReusesTheReleasedStackIndexWithANewGeneration)
	{
		S.Init(2, 1);
		const FRockItemStackHandle First = S.Data.PlaceStack(FRockItemStack(S.Def("Apple", 5), 1), S.At(0, 0));
		S.Data.ApplyRemove(S.At(0, 0), 0, S.Changes);

		S.Add(S.At(0, 0), S.Def("Pear", 5), 1, AddRefusal);

		const FRockItemStackHandle Second = S.Data.GetSlot(S.At(0, 0))->ItemHandle;
		ASSERT_THAT(AreEqual(First.GetIndex(), Second.GetIndex()));
		ASSERT_THAT(IsTrue(First != Second));
		ASSERT_THAT(IsTrue(S.Data.GetStack(First) == nullptr));
	}

	TEST_METHOD(CountMatching_SumsEveryMatchingStack)
	{
		S.Init(4, 1);
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		S.Data.PlaceStack(FRockItemStack(Arrow, 10), S.At(0, 0));
		S.Data.PlaceStack(FRockItemStack(S.Def("Apple", 5), 4), S.At(1, 0));
		S.Data.PlaceStack(FRockItemStack(Arrow, 3), S.At(3, 0));

		ASSERT_THAT(AreEqual(13, S.Data.CountMatching(IsItem("Arrow"))));
		ASSERT_THAT(AreEqual(0, S.Data.CountMatching(IsItem("Rock"))));
	}

	TEST_METHOD(RemoveMatching_EmptiesTheLowestSlotFirst)
	{
		S.Init(4, 1);
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		S.Data.PlaceStack(FRockItemStack(Arrow, 3), S.At(0, 0));
		S.Data.PlaceStack(FRockItemStack(S.Def("Apple", 5), 4), S.At(1, 0));
		S.Data.PlaceStack(FRockItemStack(Arrow, 10), S.At(3, 0));

		const int32 Removed = S.Data.RemoveMatching(IsItem("Arrow"), 5, true, S.Changes);

		ASSERT_THAT(AreEqual(5, Removed));
		ASSERT_THAT(AreEqual(0, S.CountAt(S.At(0, 0))));
		ASSERT_THAT(AreEqual(4, S.CountAt(S.At(1, 0))));
		ASSERT_THAT(AreEqual(8, S.CountAt(S.At(3, 0))));
	}

	TEST_METHOD(RemoveMatching_AllOrNothing_WithTooFew_ChangesNothing)
	{
		S.Init(2, 1);
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		S.Data.PlaceStack(FRockItemStack(Arrow, 3), S.At(0, 0));
		S.Data.PlaceStack(FRockItemStack(Arrow, 4), S.At(1, 0));

		const int32 Removed = S.Data.RemoveMatching(IsItem("Arrow"), 8, true, S.Changes);

		ASSERT_THAT(AreEqual(0, Removed));
		ASSERT_THAT(AreEqual(3, S.CountAt(S.At(0, 0))));
		ASSERT_THAT(AreEqual(4, S.CountAt(S.At(1, 0))));
		ASSERT_THAT(IsTrue(S.Changes.IsEmpty()));
	}

	TEST_METHOD(RemoveMatching_WithoutAllOrNothing_TakesWhatThereIs)
	{
		S.Init(2, 1);
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		S.Data.PlaceStack(FRockItemStack(Arrow, 3), S.At(0, 0));
		S.Data.PlaceStack(FRockItemStack(Arrow, 4), S.At(1, 0));

		const int32 Removed = S.Data.RemoveMatching(IsItem("Arrow"), 20, false, S.Changes);

		ASSERT_THAT(AreEqual(7, Removed));
		ASSERT_THAT(AreEqual(0, S.NumStacks()));
	}

	TEST_METHOD(RemoveMatching_WithACountOfZeroOrLess_DoesNothing)
	{
		S.Init(2, 1);
		S.Data.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 3), S.At(0, 0));

		ASSERT_THAT(AreEqual(0, S.Data.RemoveMatching(IsItem("Arrow"), 0, false, S.Changes)));
		ASSERT_THAT(AreEqual(0, S.Data.RemoveMatching(IsItem("Arrow"), -2, false, S.Changes)));

		ASSERT_THAT(AreEqual(3, S.CountAt(S.At(0, 0))));
	}

	TEST_METHOD(RemoveMatching_ReportsEveryChangeInOrder)
	{
		S.Init(2, 1);
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		S.Data.PlaceStack(FRockItemStack(Arrow, 3), S.At(0, 0));
		S.Data.PlaceStack(FRockItemStack(Arrow, 4), S.At(1, 0));

		S.Data.RemoveMatching(IsItem("Arrow"), 5, true, S.Changes);

		// Slot 0 released (slot cleared, stack removed), slot 1 shrunk by 2
		ASSERT_THAT(AreEqual(3, S.Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(S.Changes.Changes[0].Type == ERockDataChangeType::Slot));
		ASSERT_THAT(IsTrue(S.Changes.Changes[1].Type == ERockDataChangeType::StackRemoved));
		ASSERT_THAT(IsTrue(S.Changes.Changes[2].Type == ERockDataChangeType::StackModified));
		ASSERT_THAT(AreEqual(2, S.Changes.Changes[2].StackAfter.GetStackCount()));
	}
};

// The library verbs on a live inventory: snapshot, decide on plain data, commit (so the instance and OnItemCreated hooks run).
TEST_CLASS(RockInventoryAddRemoveLibraryTests, "BRS.RockInventory.AddRemove")
{
	FRockInventoryFixture Fixture;

	TEST_METHOD(AddItemToSlot_IntoAnEmptyCell_StoresTheStack)
	{
		Fixture.InitGrid(3, 1);
		int32 Added = 0;

		const ERockAddRefusal Refusal = URockInventoryLibrary::AddItemToSlot(
			Fixture.Inventory, Fixture.SlotAt(2, 0), FRockItemStack(Fixture.MakeDefinition("Apple", 5), 4), ERockItemOrientation::Horizontal, Added);

		ASSERT_THAT(AreEqual(ERockAddRefusal::None, Refusal));
		ASSERT_THAT(AreEqual(4, Added));
		ASSERT_THAT(AreEqual(4, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(2, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(AddItemToSlot_OnAMatchingStack_TopsItUp)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Arrow, 8, Fixture.SlotAt(0, 0));
		int32 Added = 0;

		URockInventoryLibrary::AddItemToSlot(Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Arrow, 5), ERockItemOrientation::Horizontal, Added);

		ASSERT_THAT(AreEqual(2, Added));
		ASSERT_THAT(AreEqual(10, Fixture.Inventory->GetItemByHandle(Handle).GetStackCount()));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(AddItemToSlot_RunsTheOnItemCreatedHookAtCommit)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Battery = Fixture.MakeDefinition("Battery", 1);
		Battery->CustomValue1Tag = RockInventoryTestTags::Weapon;
		FRockItemFragment_SetStats SetStats;
		SetStats.CustomValue1 = 7;
		Battery->Fragments.Add(FInstancedStruct::Make(SetStats));
		int32 Added = 0;

		URockInventoryLibrary::AddItemToSlot(Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Battery, 1), ERockItemOrientation::Horizontal, Added);

		ASSERT_THAT(AreEqual(7, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetCustomValue1()));
	}

	TEST_METHOD(AddItemToSlot_CreatesTheRuntimeInstanceAtCommit)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Pack = Fixture.MakeDefinition("Pack", 1);
		Pack->RuntimeInstanceClass = URockItemInstance::StaticClass();
		int32 Added = 0;

		URockInventoryLibrary::AddItemToSlot(Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Pack, 1), ERockItemOrientation::Horizontal, Added);

		ASSERT_THAT(IsTrue(Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetRuntimeInstance() != nullptr));
	}

	TEST_METHOD(AddItemToSlot_WhenRefused_ChangesNothing)
	{
		Fixture.InitGrid(2, 1);
		Fixture.PlaceAt(Fixture.MakeDefinition("Apple", 5), 1, Fixture.SlotAt(0, 0));
		int32 Added = 0;

		const ERockAddRefusal Refusal = URockInventoryLibrary::AddItemToSlot(
			Fixture.Inventory, Fixture.SlotAt(0, 0), FRockItemStack(Fixture.MakeDefinition("Pear", 5), 1), ERockItemOrientation::Horizontal, Added);

		ASSERT_THAT(AreEqual(ERockAddRefusal::NoRoom, Refusal));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(RemoveItemsById_AcrossStacks_RemovesTheLowestSlotFirst)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Fixture.MakeDefinition("Apple", 5), 2, Fixture.SlotAt(1, 0));
		Fixture.PlaceAt(Arrow, 10, Fixture.SlotAt(2, 0));

		ASSERT_THAT(AreEqual(5, URockInventoryLibrary::RemoveItemsById(Fixture.Inventory, "Arrow", 5)));

		ASSERT_THAT(IsFalse(Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).IsValid()));
		ASSERT_THAT(IsFalse(Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle.IsValid()));
		ASSERT_THAT(AreEqual(2, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(1, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(8, Fixture.Inventory->GetItemBySlotHandle(Fixture.SlotAt(2, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(8, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Arrow")));
	}

	TEST_METHOD(RemoveItemsById_AllOrNothing_WithTooFew_ChangesNothing)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		Fixture.PlaceAt(Arrow, 3, Fixture.SlotAt(0, 0));

		ASSERT_THAT(AreEqual(0, URockInventoryLibrary::RemoveItemsById(Fixture.Inventory, "Arrow", 4, true)));

		ASSERT_THAT(AreEqual(3, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Arrow")));
	}

	TEST_METHOD(RemoveItemsById_WithoutAllOrNothing_TakesWhatThereIs)
	{
		Fixture.InitGrid(2, 1);
		Fixture.PlaceAt(Fixture.MakeDefinition("Arrow", 10), 3, Fixture.SlotAt(0, 0));

		ASSERT_THAT(AreEqual(3, URockInventoryLibrary::RemoveItemsById(Fixture.Inventory, "Arrow", 4, false)));

		ASSERT_THAT(AreEqual(0, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(RemoveMatching_ByPredicate_RemovesOnlyMatches)
	{
		Fixture.InitGrid(2, 1);
		Fixture.PlaceAt(Fixture.MakeDefinition("Arrow", 10), 3, Fixture.SlotAt(0, 0));
		Fixture.PlaceAt(Fixture.MakeDefinition("Apple", 5), 2, Fixture.SlotAt(1, 0));

		const int32 Removed = URockInventoryLibrary::RemoveMatching(Fixture.Inventory, [](const FRockItemStack& Stack) { return Stack.GetMaxStackCount() == 5; }, 2);

		ASSERT_THAT(AreEqual(2, Removed));
		ASSERT_THAT(AreEqual(3, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Arrow")));
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(RemoveItemsById_ReleasingAStack_InvalidatesItsHandle)
	{
		Fixture.InitGrid(2, 1);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Fixture.MakeDefinition("Arrow", 10), 3, Fixture.SlotAt(0, 0));
		URockInventoryLibrary::RemoveItemsById(Fixture.Inventory, "Arrow", 3);

		ASSERT_THAT(IsFalse(Fixture.Inventory->IsHandleValid(Handle)));
	}

	TEST_METHOD(RemoveItemsById_WithACountOfZero_DoesNothing)
	{
		Fixture.InitGrid(2, 1);
		Fixture.PlaceAt(Fixture.MakeDefinition("Arrow", 10), 3, Fixture.SlotAt(0, 0));

		ASSERT_THAT(AreEqual(0, URockInventoryLibrary::RemoveItemsById(Fixture.Inventory, "Arrow", 0)));
		ASSERT_THAT(AreEqual(3, URockInventoryLibrary::GetItemCount(Fixture.Inventory, "Arrow")));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
