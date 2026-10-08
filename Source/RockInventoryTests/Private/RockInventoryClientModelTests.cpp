// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Client/RockInventoryClientModel.h"
#include "Inventory/RockInventoryData.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace
{
	FRockInventorySlotHandle ModelSlotAt(const FRockInventoryData& Data, int32 Column, int32 Row)
	{
		const FRockInventorySectionInfo& Section = Data.Sections[0];
		return FRockInventorySlotHandle(Section.GetFirstSlotIndex() + Row * Section.GetColumns() + Column);
	}

	FRockInventorySectionInfo ModelGrid(int32 Columns, int32 Rows)
	{
		return FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, Columns, Rows, ERockItemSizePolicy::RespectSize);
	}
}

// The pure part of the client model: what visibly changed between two states. Plain data, no inventory object, no world.
TEST_CLASS(RockInventoryPresentationDiffTests, "BRS.RockInventory.ClientModel.Diff")
{
	FRockInventoryData Before;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;

	URockItemDefinition* Def(FName Id, int32 MaxStack = 1)
	{
		URockItemDefinition* Definition = NewObject<URockItemDefinition>(GetTransientPackage());
		Definition->ItemId = Id;
		Definition->MaxStackCount = MaxStack;
		KeepAlive.Emplace(Definition);
		return Definition;
	}

	static FRockInventoryPresentationDiff Diff(const FRockInventoryData& Old, const FRockInventoryData& New, ERockInventorySyncState OldState = ERockInventorySyncState::Live, ERockInventorySyncState NewState = ERockInventorySyncState::Live)
	{
		return FRockInventoryPresentationDiff::Between(Old, OldState, New, NewState);
	}

	TEST_METHOD(IdenticalStates_AreAnEmptyDiff)
	{
		Before.Init({ModelGrid(3, 1)});
		Before.PlaceStack(FRockItemStack(Def("Apple"), 1), ModelSlotAt(Before, 0, 0));

		ASSERT_THAT(IsTrue(Diff(Before, FRockInventoryData(Before)).IsEmpty()));
	}

	TEST_METHOD(AMove_ListsTheTwoSlots_AndNoStack)
	{
		Before.Init({ModelGrid(3, 1)});
		Before.PlaceStack(FRockItemStack(Def("Apple"), 1), ModelSlotAt(Before, 0, 0));
		FRockInventoryData After = Before;
		FRockInventoryChangeSet Changes;
		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, FRockInventoryData::ApplyMove(After, ModelSlotAt(After, 0, 0), After, ModelSlotAt(After, 2, 0), FRockMoveItemParams(), Changes)));

		const FRockInventoryPresentationDiff Result = Diff(Before, After);

		ASSERT_THAT(AreEqual(2, Result.ChangedSlots.Num()));
		ASSERT_THAT(IsTrue(Result.ChangedSlots.Contains(ModelSlotAt(Before, 0, 0))));
		ASSERT_THAT(IsTrue(Result.ChangedSlots.Contains(ModelSlotAt(Before, 2, 0))));
		ASSERT_THAT(IsTrue(Result.StacksCreated.IsEmpty() && Result.StacksRemoved.IsEmpty() && Result.StacksModified.IsEmpty()));
		ASSERT_THAT(IsFalse(Result.bLayoutChanged));
	}

	TEST_METHOD(ACountChange_IsAModifiedStack_AndNoSlot)
	{
		Before.Init({ModelGrid(2, 1)});
		const FRockItemStackHandle Handle = Before.PlaceStack(FRockItemStack(Def("Arrow", 10), 4), ModelSlotAt(Before, 0, 0));
		FRockInventoryData After = Before;
		FRockInventoryChangeSet Changes;
		ASSERT_THAT(AreEqual(ERockRemoveRefusal::None, After.ApplyRemove(ModelSlotAt(After, 0, 0), 1, Changes)));

		const FRockInventoryPresentationDiff Result = Diff(Before, After);

		ASSERT_THAT(AreEqual(1, Result.StacksModified.Num()));
		ASSERT_THAT(AreEqual(Handle, Result.StacksModified[0]));
		ASSERT_THAT(IsTrue(Result.ChangedSlots.IsEmpty() && Result.StacksCreated.IsEmpty() && Result.StacksRemoved.IsEmpty()));
	}

	TEST_METHOD(AddingAndRemovingAStack_AreCreatedAndRemoved)
	{
		Before.Init({ModelGrid(2, 1)});
		FRockInventoryData WithApple = Before;
		const FRockItemStackHandle Handle = WithApple.PlaceStack(FRockItemStack(Def("Apple"), 1), ModelSlotAt(WithApple, 1, 0));

		const FRockInventoryPresentationDiff Added = Diff(Before, WithApple);
		const FRockInventoryPresentationDiff Removed = Diff(WithApple, Before);

		ASSERT_THAT(AreEqual(1, Added.StacksCreated.Num()));
		ASSERT_THAT(AreEqual(Handle, Added.StacksCreated[0]));
		ASSERT_THAT(AreEqual(1, Added.ChangedSlots.Num()));
		ASSERT_THAT(AreEqual(1, Removed.StacksRemoved.Num()));
		ASSERT_THAT(AreEqual(Handle, Removed.StacksRemoved[0]));
		ASSERT_THAT(AreEqual(1, Removed.ChangedSlots.Num()));
	}

	TEST_METHOD(ARecycledStackIndex_IsARemovalAndACreation_NotAModification)
	{
		Before.Init({ModelGrid(2, 1)});
		const FRockItemStackHandle First = Before.PlaceStack(FRockItemStack(Def("Apple"), 1), ModelSlotAt(Before, 0, 0));
		FRockInventoryData After = Before;
		FRockInventoryChangeSet Changes;
		After.ApplyRemove(ModelSlotAt(After, 0, 0), 0, Changes);
		const FRockItemStackHandle Second = After.PlaceStack(FRockItemStack(Def("Pear"), 1), ModelSlotAt(After, 0, 0));
		ASSERT_THAT(AreEqual(First.GetIndex(), Second.GetIndex()));

		const FRockInventoryPresentationDiff Result = Diff(Before, After);

		ASSERT_THAT(AreEqual(1, Result.StacksRemoved.Num()));
		ASSERT_THAT(AreEqual(1, Result.StacksCreated.Num()));
		ASSERT_THAT(IsTrue(Result.StacksModified.IsEmpty()));
		ASSERT_THAT(AreEqual(First, Result.StacksRemoved[0]));
		ASSERT_THAT(AreEqual(Second, Result.StacksCreated[0]));
	}

	TEST_METHOD(ASyncStateChange_IsADiffOnItsOwn)
	{
		Before.Init({ModelGrid(2, 1)});

		const FRockInventoryPresentationDiff Result = Diff(Before, FRockInventoryData(Before), ERockInventorySyncState::Syncing, ERockInventorySyncState::Live);

		ASSERT_THAT(IsFalse(Result.IsEmpty()));
		ASSERT_THAT(IsTrue(Result.bSyncStateChanged));
		ASSERT_THAT(AreEqual(ERockInventorySyncState::Syncing, Result.PreviousSyncState));
		ASSERT_THAT(IsTrue(Result.ChangedSlots.IsEmpty()));
	}

	TEST_METHOD(ADifferentLayout_SetsLayoutChanged)
	{
		Before.Init({ModelGrid(2, 1)});
		FRockInventoryData Wider;
		Wider.Init({ModelGrid(3, 1)});

		const FRockInventoryPresentationDiff Result = Diff(Before, Wider);

		ASSERT_THAT(IsTrue(Result.bLayoutChanged));
		ASSERT_THAT(IsTrue(Result.ChangedSlots.Contains(ModelSlotAt(Wider, 2, 0))));
	}
};

// The model on top of a real inventory: follows its change batches, announces diffs, reads like the inventory.
TEST_CLASS(RockInventoryClientModelTests, "BRS.RockInventory.ClientModel")
{
	FRockInventoryFixture Fixture;
	TArray<TStrongObjectPtr<URockInventoryClientModel>> Models;

	URockInventoryClientModel* MakeModel(URockInventory* Inventory)
	{
		URockInventoryClientModel* Model = NewObject<URockInventoryClientModel>(GetTransientPackage());
		Models.Emplace(Model);
		Model->Bind(Inventory);
		return Model;
	}

	TEST_METHOD(Bind_TakesTheCurrentState_WithoutAnnouncingIt)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 3, Fixture.SlotAt(0, 0));
		URockInventoryClientModel* Model = NewObject<URockInventoryClientModel>(GetTransientPackage());
		Models.Emplace(Model);
		int32 Announced = 0;
		Model->OnChanged.AddLambda([&Announced](URockInventoryClientModel&, const FRockInventoryPresentationDiff&) { ++Announced; });

		Model->Bind(Fixture.Inventory);

		ASSERT_THAT(AreEqual(0, Announced));
		ASSERT_THAT(AreEqual(3, Model->GetItemBySlotHandle(Fixture.SlotAt(0, 0)).GetStackCount()));
		ASSERT_THAT(AreEqual(Handle, Model->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle));
		ASSERT_THAT(AreEqual(static_cast<int32>(Fixture.Inventory->GetRevision()), Model->GetRevision()));
		ASSERT_THAT(AreEqual(ERockInventorySyncState::Live, Model->GetSyncState()));
	}

	TEST_METHOD(AMove_AnnouncesOneDiff_AndTheModelHoldsTheFinishedState)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryClientModel* Model = MakeModel(Fixture.Inventory);
		TArray<FRockInventoryPresentationDiff> Diffs;
		TArray<FRockItemStackHandle> SeenInSecondSlot;
		Model->OnChanged.AddLambda([&](URockInventoryClientModel& Changed, const FRockInventoryPresentationDiff& Diff)
		{
			Diffs.Add(Diff);
			SeenInSecondSlot.Add(Changed.GetSlotByHandle(Fixture.SlotAt(2, 0)).ItemHandle);
		});

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 0))));

		ASSERT_THAT(AreEqual(1, Diffs.Num()));
		ASSERT_THAT(AreEqual(2, Diffs[0].ChangedSlots.Num()));
		ASSERT_THAT(AreEqual(Handle, SeenInSecondSlot[0]));
		ASSERT_THAT(IsFalse(Model->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle.IsValid()));
		ASSERT_THAT(AreEqual(static_cast<int32>(Fixture.Inventory->GetRevision()), Model->GetRevision()));
	}

	TEST_METHOD(ACountChange_AnnouncesAModifiedStack)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Arrow, 4, Fixture.SlotAt(0, 0));
		URockInventoryClientModel* Model = MakeModel(Fixture.Inventory);
		FRockInventoryPresentationDiff Last;
		Model->OnChanged.AddLambda([&Last](URockInventoryClientModel&, const FRockInventoryPresentationDiff& Diff) { Last = Diff; });

		Fixture.Inventory->SetItemStackCount(Handle, 7);

		ASSERT_THAT(AreEqual(1, Last.StacksModified.Num()));
		ASSERT_THAT(AreEqual(7, Model->GetItemByHandle(Handle).GetStackCount()));
	}

	TEST_METHOD(ARemovedItem_ClearsTheSlotInTheModel)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryClientModel* Model = MakeModel(Fixture.Inventory);
		FRockInventoryPresentationDiff Last;
		Model->OnChanged.AddLambda([&Last](URockInventoryClientModel&, const FRockInventoryPresentationDiff& Diff) { Last = Diff; });

		ASSERT_THAT(AreEqual(1, URockInventoryLibrary::RemoveItemsById(Fixture.Inventory, "Apple", 1)));

		ASSERT_THAT(AreEqual(1, Last.StacksRemoved.Num()));
		ASSERT_THAT(IsFalse(Model->GetItemByHandle(Handle).IsValid()));
		ASSERT_THAT(IsFalse(Model->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle.IsValid()));
	}

	TEST_METHOD(AfterUnbind_TheModelStopsFollowing)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryClientModel* Model = MakeModel(Fixture.Inventory);
		int32 Announced = 0;
		Model->OnChanged.AddLambda([&Announced](URockInventoryClientModel&, const FRockInventoryPresentationDiff&) { ++Announced; });
		Model->Unbind();

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(0, Announced));
		ASSERT_THAT(IsTrue(Model->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle.IsValid()));
		ASSERT_THAT(IsTrue(Model->GetInventory() == nullptr));
	}

	TEST_METHOD(SetState_AnnouncesOnlyWhatChanged_AndNeedsNoInventory)
	{
		URockInventoryClientModel* Model = MakeModel(nullptr);
		FRockInventoryData Empty;
		Empty.Init({ModelGrid(2, 1)});
		int32 Announced = 0;
		FRockInventoryPresentationDiff Last;
		Model->OnChanged.AddLambda([&](URockInventoryClientModel&, const FRockInventoryPresentationDiff& Diff) { ++Announced; Last = Diff; });

		Model->SetState(FRockInventoryData(Empty), 1, ERockInventorySyncState::Syncing);
		Model->SetState(FRockInventoryData(Empty), 2, ERockInventorySyncState::Syncing);
		Model->SetState(FRockInventoryData(Empty), 3, ERockInventorySyncState::Live);

		ASSERT_THAT(AreEqual(2, Announced));
		ASSERT_THAT(IsTrue(Last.bSyncStateChanged));
		ASSERT_THAT(AreEqual(ERockInventorySyncState::Syncing, Last.PreviousSyncState));
		ASSERT_THAT(AreEqual(3, Model->GetRevision()));
		ASSERT_THAT(AreEqual(ERockInventorySyncState::Live, Model->GetSyncState()));
	}

	TEST_METHOD(Reads_OfInvalidOrStaleHandles_GiveInvalidValues)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryClientModel* Model = MakeModel(Fixture.Inventory);
		Fixture.Inventory->RemoveItemFromInventory(Handle);

		ASSERT_THAT(IsFalse(Model->GetItemByHandle(Handle).IsValid()));
		ASSERT_THAT(IsFalse(Model->GetItemByHandle(FRockItemStackHandle::Invalid()).IsValid()));
		ASSERT_THAT(IsFalse(Model->GetSlotByHandle(FRockInventorySlotHandle(99)).ItemHandle.IsValid()));
		ASSERT_THAT(IsFalse(Model->GetSlotByAbsoluteIndex(-1).ItemHandle.IsValid()));
		ASSERT_THAT(IsFalse(Model->GetItemBySlotHandle(FRockInventorySlotHandle(99)).IsValid()));
		ASSERT_THAT(AreEqual(INDEX_NONE, Model->GetSectionIndex(FGameplayTag())));
		ASSERT_THAT(IsFalse(Model->GetSectionInfoBySlotHandle(FRockInventorySlotHandle(99)).IsValid()));
	}

	TEST_METHOD(Sections_AreFoundByTagAndBySlot)
	{
		Fixture.InitGrid(3, 2);
		URockInventoryClientModel* Model = MakeModel(Fixture.Inventory);

		ASSERT_THAT(AreEqual(0, Model->GetSectionIndex(RockInventoryTags::Inventory_Section_Backpack)));
		ASSERT_THAT(AreEqual(3, Model->GetSectionInfo(RockInventoryTags::Inventory_Section_Backpack).GetColumns()));
		ASSERT_THAT(AreEqual(0, Model->GetSectionInfoBySlotHandle(Fixture.SlotAt(2, 1)).GetSectionIndex()));
	}

	TEST_METHOD(SlotOfAnItem_IsFoundThroughTheModel)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(1, 0));
		URockInventoryClientModel* Model = MakeModel(Fixture.Inventory);

		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Model->GetSlotByItemHandle(Handle).SlotHandle));
		ASSERT_THAT(IsFalse(Model->GetSlotByItemHandle(FRockItemStackHandle::Invalid()).ItemHandle.IsValid()));
	}

	TEST_METHOD(TheSubsystem_SharesOneModelPerInventory)
	{
		Fixture.InitGrid(2, 1);

		URockInventoryClientModel* First = URockInventoryClientModelSubsystem::GetModel(Fixture.Inventory);
		URockInventoryClientModel* Second = URockInventoryClientModelSubsystem::GetModel(Fixture.Inventory);

		ASSERT_THAT(IsNotNull(First));
		ASSERT_THAT(IsTrue(First == Second));
		ASSERT_THAT(IsTrue(First->GetInventory() == Fixture.Inventory));
		ASSERT_THAT(IsTrue(URockInventoryClientModelSubsystem::GetModel(nullptr) == nullptr));
	}
};
#endif
