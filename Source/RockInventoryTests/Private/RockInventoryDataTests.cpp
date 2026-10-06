// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventoryData.h"
#include "Item/RockItemDefinition.h"
#include "Misc/RockInventoryTags.h"
#include "RockInventoryTestTags.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace
{
	/** A plain-data inventory and a second one for cross-inventory moves. No world, no URockInventory. */
	struct FDataScenario
	{
		FRockInventoryData A;
		FRockInventoryData B;
		TArray<TStrongObjectPtr<UObject>> KeepAlive;

		static FRockInventorySectionInfo Grid(int32 Columns, int32 Rows, ERockItemSizePolicy Policy = ERockItemSizePolicy::RespectSize)
		{
			return FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, Columns, Rows, Policy);
		}

		static FRockInventorySlotHandle At(const FRockInventoryData& Data, int32 Column, int32 Row)
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

		static bool IsEmpty(const FRockInventoryData& Data, const FRockInventorySlotHandle& Slot)
		{
			return !Data.GetSlot(Slot)->ItemHandle.IsValid();
		}

		static int32 Count(const FRockInventoryData& Data, const FRockInventorySlotHandle& Slot)
		{
			const FRockItemStack* Stack = Data.GetSlotStack(Slot);
			return Stack ? Stack->GetStackCount() : 0;
		}

		static int32 NumStacks(const FRockInventoryData& Data)
		{
			int32 Num = 0;
			for (const FRockItemStack& Stack : Data.Stacks)
			{
				Num += Stack.IsValid() ? 1 : 0;
			}
			return Num;
		}
	};
}

// The Move scenarios of BRS.RockInventory.Move, on FRockInventoryData: CanMove / ApplyMove with no inventory object, events or world.
TEST_CLASS(RockInventoryDataMoveTests, "BRS.RockInventory.Data")
{
	FDataScenario S;
	FRockInventoryChangeSet Changes;

	ERockMoveRefusal Apply(const FRockInventorySlotHandle& From, const FRockInventorySlotHandle& To, const FRockMoveItemParams& Params = FRockMoveItemParams())
	{
		return FRockInventoryData::ApplyMove(S.A, From, S.A, To, Params, Changes);
	}

	ERockMoveRefusal ApplyAcross(const FRockInventorySlotHandle& From, const FRockInventorySlotHandle& To, const FRockMoveItemParams& Params = FRockMoveItemParams())
	{
		return FRockInventoryData::ApplyMove(S.A, From, S.B, To, Params, Changes);
	}

	TEST_METHOD(Move_ToAnEmptySlot_KeepsTheHandleAndClearsTheSource)
	{
		S.A.Init({FDataScenario::Grid(4, 2)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 2, 1))));

		ASSERT_THAT(IsTrue(FDataScenario::IsEmpty(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(AreEqual(Handle, S.A.GetSlot(FDataScenario::At(S.A, 2, 1))->ItemHandle));
		ASSERT_THAT(AreEqual(1, FDataScenario::NumStacks(S.A)));
	}

	TEST_METHOD(Move_ToAnEmptySlot_ReportsTheSourceClearThenTheTargetSet)
	{
		S.A.Init({FDataScenario::Grid(4, 2)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 0));

		Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 2, 1));

		ASSERT_THAT(AreEqual(2, Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(Changes.Changes[0].Type == ERockDataChangeType::Slot));
		ASSERT_THAT(AreEqual(FDataScenario::At(S.A, 0, 0), Changes.Changes[0].Slot));
		ASSERT_THAT(AreEqual(Handle, Changes.Changes[0].SlotBefore.ItemHandle));
		ASSERT_THAT(IsFalse(Changes.Changes[0].SlotAfter.ItemHandle.IsValid()));
		ASSERT_THAT(AreEqual(FDataScenario::At(S.A, 2, 1), Changes.Changes[1].Slot));
		ASSERT_THAT(IsFalse(Changes.Changes[1].SlotBefore.ItemHandle.IsValid()));
		ASSERT_THAT(AreEqual(Handle, Changes.Changes[1].SlotAfter.ItemHandle));
	}

	TEST_METHOD(CanMove_ChangesNothing)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		S.A.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 4), FDataScenario::At(S.A, 0, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, FRockInventoryData::CanMove(S.A, FDataScenario::At(S.A, 0, 0), S.A, FDataScenario::At(S.A, 1, 0), FRockMoveItemParams())));

		ASSERT_THAT(AreEqual(4, FDataScenario::Count(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(IsTrue(FDataScenario::IsEmpty(S.A, FDataScenario::At(S.A, 1, 0))));
	}

	TEST_METHOD(Move_ToTheSameSlot_IsANoOpWithNoChanges)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 0, 0))));

		ASSERT_THAT(IsTrue(Changes.IsEmpty()));
		ASSERT_THAT(AreEqual(Handle, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->ItemHandle));
	}

	TEST_METHOD(Move_FromAnEmptySlot_IsRefused)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});

		ASSERT_THAT(AreEqual(ERockMoveRefusal::EmptySource, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0))));
	}

	TEST_METHOD(Move_FromAnOutOfRangeSlot_IsRefused)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});

		ASSERT_THAT(AreEqual(ERockMoveRefusal::InvalidSourceSlot, Apply(FRockInventorySlotHandle(50), FDataScenario::At(S.A, 1, 0))));
		ASSERT_THAT(AreEqual(ERockMoveRefusal::InvalidSourceSlot, Apply(FRockInventorySlotHandle(), FDataScenario::At(S.A, 1, 0))));
	}

	TEST_METHOD(Move_ToAnOutOfRangeSlot_IsRefusedWithoutLosingTheItem)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::InvalidTargetSlot, Apply(FDataScenario::At(S.A, 0, 0), FRockInventorySlotHandle(50))));

		ASSERT_THAT(AreEqual(Handle, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->ItemHandle));
		ASSERT_THAT(IsTrue(Changes.IsEmpty()));
	}

	TEST_METHOD(Move_OntoADifferentItem_IsRefusedAndChangesNothing)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		const FRockItemStackHandle Apple = S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 0));
		const FRockItemStackHandle Pear = S.A.PlaceStack(FRockItemStack(S.Def("Pear"), 1), FDataScenario::At(S.A, 1, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::NoRoom, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0))));

		ASSERT_THAT(AreEqual(Apple, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->ItemHandle));
		ASSERT_THAT(AreEqual(Pear, S.A.GetSlot(FDataScenario::At(S.A, 1, 0))->ItemHandle));
		ASSERT_THAT(IsTrue(Changes.IsEmpty()));
	}

	TEST_METHOD(Move_MultiCellItemOverlappingItsOwnFootprint_Succeeds)
	{
		S.A.Init({FDataScenario::Grid(4, 2)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Crate", 1, FIntPoint(2, 2)), 1), FDataScenario::At(S.A, 0, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0))));

		ASSERT_THAT(IsTrue(FDataScenario::IsEmpty(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(AreEqual(Handle, S.A.GetSlot(FDataScenario::At(S.A, 1, 0))->ItemHandle));
	}

	TEST_METHOD(Move_MultiCellItemPastTheEdge_IsRefusedAndStaysPut)
	{
		S.A.Init({FDataScenario::Grid(4, 4)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Crate", 1, FIntPoint(2, 2)), 1), FDataScenario::At(S.A, 0, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::NoRoom, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 3, 3))));

		ASSERT_THAT(AreEqual(Handle, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->ItemHandle));
		ASSERT_THAT(IsTrue(FDataScenario::IsEmpty(S.A, FDataScenario::At(S.A, 3, 3))));
	}

	TEST_METHOD(Move_MultiCellItemOverAnotherItem_IsRefusedAndStaysPut)
	{
		S.A.Init({FDataScenario::Grid(4, 2)});
		const FRockItemStackHandle Crate = S.A.PlaceStack(FRockItemStack(S.Def("Crate", 1, FIntPoint(2, 2)), 1), FDataScenario::At(S.A, 0, 0));
		S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 2, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::NoRoom, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0))));

		ASSERT_THAT(AreEqual(Crate, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->ItemHandle));
	}

	TEST_METHOD(Move_HalfStack_SplitsRoundingUp)
	{
		S.A.Init({FDataScenario::Grid(3, 1)});
		const FRockItemStackHandle Source = S.A.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 5), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::HalfStack;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 2, 0), Params)));

		ASSERT_THAT(AreEqual(2, S.A.GetStack(Source)->GetStackCount()));
		ASSERT_THAT(AreEqual(3, FDataScenario::Count(S.A, FDataScenario::At(S.A, 2, 0))));
		ASSERT_THAT(AreEqual(2, FDataScenario::NumStacks(S.A)));
	}

	TEST_METHOD(Move_Split_ReportsTheShrunkSourceThenTheCreatedStackThenTheSlot)
	{
		S.A.Init({FDataScenario::Grid(3, 1)});
		const FRockItemStackHandle Source = S.A.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 5), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::SingleItem;

		Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 2, 0), Params);

		ASSERT_THAT(AreEqual(3, Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(Changes.Changes[0].Type == ERockDataChangeType::StackModified));
		ASSERT_THAT(AreEqual(Source, Changes.Changes[0].Stack));
		ASSERT_THAT(AreEqual(5, Changes.Changes[0].StackBefore.GetStackCount()));
		ASSERT_THAT(AreEqual(4, Changes.Changes[0].StackAfter.GetStackCount()));
		ASSERT_THAT(IsTrue(Changes.Changes[1].Type == ERockDataChangeType::StackCreated));
		ASSERT_THAT(AreEqual(1, Changes.Changes[1].StackAfter.GetStackCount()));
		ASSERT_THAT(IsTrue(Changes.Changes[2].Type == ERockDataChangeType::Slot));
		ASSERT_THAT(AreEqual(Changes.Changes[1].Stack, Changes.Changes[2].SlotAfter.ItemHandle));
	}

	TEST_METHOD(Move_CustomAmount_TakesThatManyAndClampsToTheStack)
	{
		S.A.Init({FDataScenario::Grid(3, 1)});
		S.A.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 7), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::CustomAmount;
		Params.MoveCount = 3;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0), Params)));
		ASSERT_THAT(AreEqual(4, FDataScenario::Count(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(AreEqual(3, FDataScenario::Count(S.A, FDataScenario::At(S.A, 1, 0))));

		Params.MoveCount = 50;
		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 2, 0), Params)));
		ASSERT_THAT(IsTrue(FDataScenario::IsEmpty(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(AreEqual(4, FDataScenario::Count(S.A, FDataScenario::At(S.A, 2, 0))));
	}

	TEST_METHOD(Move_ZeroCustomAmount_IsRefused)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		S.A.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 4), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::CustomAmount;
		Params.MoveCount = 0;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::InvalidAmount, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0), Params)));
	}

	TEST_METHOD(Move_IntoAPartialStack_MergesUpToTheMaxAndKeepsTheRest)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		const FRockItemStackHandle Moving = S.A.PlaceStack(FRockItemStack(Arrow, 6), FDataScenario::At(S.A, 0, 0));
		S.A.PlaceStack(FRockItemStack(Arrow, 7), FDataScenario::At(S.A, 1, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0))));

		ASSERT_THAT(AreEqual(10, FDataScenario::Count(S.A, FDataScenario::At(S.A, 1, 0))));
		ASSERT_THAT(AreEqual(3, S.A.GetStack(Moving)->GetStackCount()));
	}

	TEST_METHOD(Move_IntoAPartialStack_RemovesTheSourceWhenItFitsEntirely)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		const FRockItemStackHandle Source = S.A.PlaceStack(FRockItemStack(Arrow, 2), FDataScenario::At(S.A, 0, 0));
		S.A.PlaceStack(FRockItemStack(Arrow, 5), FDataScenario::At(S.A, 1, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0))));

		ASSERT_THAT(AreEqual(7, FDataScenario::Count(S.A, FDataScenario::At(S.A, 1, 0))));
		ASSERT_THAT(IsTrue(FDataScenario::IsEmpty(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(IsTrue(S.A.GetStack(Source) == nullptr));
		// Merge order: target count, source released, source slot cleared.
		ASSERT_THAT(AreEqual(3, Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(Changes.Changes[0].Type == ERockDataChangeType::StackModified));
		ASSERT_THAT(IsTrue(Changes.Changes[1].Type == ERockDataChangeType::StackRemoved));
		ASSERT_THAT(IsTrue(Changes.Changes[2].Type == ERockDataChangeType::Slot));
	}

	TEST_METHOD(Move_IntoAFullStack_IsRefusedAndChangesNothing)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		URockItemDefinition* Arrow = S.Def("Arrow", 10);
		S.A.PlaceStack(FRockItemStack(Arrow, 4), FDataScenario::At(S.A, 0, 0));
		S.A.PlaceStack(FRockItemStack(Arrow, 10), FDataScenario::At(S.A, 1, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::NoRoom, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0))));

		ASSERT_THAT(AreEqual(4, FDataScenario::Count(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(AreEqual(10, FDataScenario::Count(S.A, FDataScenario::At(S.A, 1, 0))));
	}

	TEST_METHOD(Move_StoresTheDesiredOrientationOnTheTargetSlot)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 1, 0), Params)));

		ASSERT_THAT(AreEqual(ERockItemOrientation::Vertical, S.A.GetSlot(FDataScenario::At(S.A, 1, 0))->Orientation));
		ASSERT_THAT(AreEqual(ERockItemOrientation::Horizontal, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->Orientation));
	}

	TEST_METHOD(Move_RotatedOnArrival_FitsUsingTheSwappedFootprint)
	{
		S.A.Init({FDataScenario::Grid(4, 3)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Rifle", 1, FIntPoint(3, 1)), 1), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 3, 0), Params)));

		ASSERT_THAT(AreEqual(Handle, S.A.GetSlot(FDataScenario::At(S.A, 3, 0))->ItemHandle));
	}

	TEST_METHOD(Move_RotatedPastTheBottomEdge_IsRefusedAndStaysPut)
	{
		S.A.Init({FDataScenario::Grid(4, 1)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Plank", 1, FIntPoint(2, 1)), 1), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::NoRoom, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 2, 0), Params)));

		ASSERT_THAT(AreEqual(Handle, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->ItemHandle));
	}

	TEST_METHOD(Move_OntoARotatedItemsSwappedCells_IsRefused)
	{
		S.A.Init({FDataScenario::Grid(4, 4)});
		S.A.PlaceStack(FRockItemStack(S.Def("Rifle", 1, FIntPoint(3, 1)), 1), FDataScenario::At(S.A, 0, 0), ERockItemOrientation::Vertical);
		S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 3, 3));

		// Rotated, the rifle covers (0,0)-(0,2), so (0,2) is taken and (1,0) is free.
		ASSERT_THAT(AreEqual(ERockMoveRefusal::NoRoom, Apply(FDataScenario::At(S.A, 3, 3), FDataScenario::At(S.A, 0, 2))));
		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 3, 3), FDataScenario::At(S.A, 1, 0))));
	}

	TEST_METHOD(Move_RotateInPlace_ChangesOrientationWhenItFits)
	{
		S.A.Init({FDataScenario::Grid(4, 4)});
		const FRockItemStackHandle Handle = S.A.PlaceStack(FRockItemStack(S.Def("Rifle", 1, FIntPoint(3, 1)), 1), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 0, 0), Params)));

		ASSERT_THAT(AreEqual(Handle, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->ItemHandle));
		ASSERT_THAT(AreEqual(ERockItemOrientation::Vertical, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->Orientation));
		ASSERT_THAT(AreEqual(1, Changes.Changes.Num()));
	}

	TEST_METHOD(Move_RotateInPlace_IsRefusedWhenTheRotatedFootprintIsBlocked)
	{
		S.A.Init({FDataScenario::Grid(4, 4)});
		S.A.PlaceStack(FRockItemStack(S.Def("Rifle", 1, FIntPoint(3, 1)), 1), FDataScenario::At(S.A, 0, 0));
		S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 1));
		FRockMoveItemParams Params;
		Params.DesiredOrientation = ERockItemOrientation::Vertical;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::NoRoomToRotate, Apply(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.A, 0, 0), Params)));

		ASSERT_THAT(AreEqual(ERockItemOrientation::Horizontal, S.A.GetSlot(FDataScenario::At(S.A, 0, 0))->Orientation));
	}

	TEST_METHOD(Move_AcrossData_TransfersTheStackAndStalesTheOldHandle)
	{
		S.A.Init({FDataScenario::Grid(2, 1)});
		S.B.Init({FDataScenario::Grid(2, 2)});
		URockItemDefinition* Apple = S.Def("Apple");
		const FRockItemStackHandle OldHandle = S.A.PlaceStack(FRockItemStack(Apple, 1), FDataScenario::At(S.A, 0, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, ApplyAcross(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.B, 1, 1))));

		ASSERT_THAT(AreEqual(0, FDataScenario::NumStacks(S.A)));
		ASSERT_THAT(IsTrue(FDataScenario::IsEmpty(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(IsTrue(S.A.GetStack(OldHandle) == nullptr));
		ASSERT_THAT(AreEqual(1, FDataScenario::NumStacks(S.B)));
		const FRockItemStack* Arrived = S.B.GetSlotStack(FDataScenario::At(S.B, 1, 1));
		ASSERT_THAT(IsTrue(Arrived && Arrived->GetDefinition() == Apple));
		// Source clear, source released, target created, target set; the sides say which data each belongs to.
		ASSERT_THAT(AreEqual(4, Changes.Changes.Num()));
		ASSERT_THAT(IsTrue(Changes.Changes[1].Type == ERockDataChangeType::StackRemoved && Changes.Changes[1].Side == ERockInventorySide::Source));
		ASSERT_THAT(IsTrue(Changes.Changes[2].Type == ERockDataChangeType::StackCreated && Changes.Changes[2].Side == ERockInventorySide::Target));
		ASSERT_THAT(IsTrue(Changes.Changes[3].Side == ERockInventorySide::Target));
	}

	TEST_METHOD(Move_AcrossData_PartialMoveLeavesTheRestBehind)
	{
		S.A.Init({FDataScenario::Grid(1, 1)});
		S.B.Init({FDataScenario::Grid(1, 1)});
		S.A.PlaceStack(FRockItemStack(S.Def("Arrow", 10), 8), FDataScenario::At(S.A, 0, 0));
		FRockMoveItemParams Params;
		Params.MoveMode = ERockItemMoveMode::CustomAmount;
		Params.MoveCount = 5;

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, ApplyAcross(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.B, 0, 0), Params)));

		ASSERT_THAT(AreEqual(3, FDataScenario::Count(S.A, FDataScenario::At(S.A, 0, 0))));
		ASSERT_THAT(AreEqual(5, FDataScenario::Count(S.B, FDataScenario::At(S.B, 0, 0))));
	}

	TEST_METHOD(Move_AcrossData_IgnoresHandleIndexCollisionsInTheTarget)
	{
		S.A.Init({FDataScenario::Grid(1, 1)});
		S.B.Init({FDataScenario::Grid(2, 1)});
		const FRockItemStackHandle Apples = S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 0));
		URockItemDefinition* Pear = S.Def("Pear");
		const FRockItemStackHandle Pears = S.B.PlaceStack(FRockItemStack(Pear, 1), FDataScenario::At(S.B, 0, 0));
		ASSERT_THAT(AreEqual(Apples, Pears));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::NoRoom, ApplyAcross(FDataScenario::At(S.A, 0, 0), FDataScenario::At(S.B, 0, 0))));

		ASSERT_THAT(IsTrue(S.B.GetSlotStack(FDataScenario::At(S.B, 0, 0))->GetDefinition() == Pear));
		ASSERT_THAT(AreEqual(1, FDataScenario::NumStacks(S.A)));
	}

	TEST_METHOD(Move_IntoASectionThatRejectsTheItem_IsRefused)
	{
		FRockInventorySectionInfo Hats = FDataScenario::Grid(1, 1);
		// An untagged apple does not carry the weapon tag the section asks for.
		Hats.SetSectionFilter(FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(RockInventoryTestTags::Weapon)));
		S.A.Init({FDataScenario::Grid(2, 1), Hats});
		S.A.PlaceStack(FRockItemStack(S.Def("Apple"), 1), FDataScenario::At(S.A, 0, 0));

		ASSERT_THAT(AreEqual(ERockMoveRefusal::SectionRejectsItem, Apply(FDataScenario::At(S.A, 0, 0), FRockInventorySlotHandle(2))));
	}

	TEST_METHOD(Move_IntoAnIgnoreSizeSection_OnlyNeedsTheCellToBeFree)
	{
		S.A.Init({FDataScenario::Grid(2, 1), FDataScenario::Grid(2, 1, ERockItemSizePolicy::IgnoreSize)});
		S.A.PlaceStack(FRockItemStack(S.Def("Crate", 1, FIntPoint(2, 1)), 1), FDataScenario::At(S.A, 0, 0));
		const FRockInventorySlotHandle Quick(2);

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Apply(FDataScenario::At(S.A, 0, 0), Quick)));

		ASSERT_THAT(IsTrue(S.A.GetSlot(Quick)->ItemHandle.IsValid()));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
