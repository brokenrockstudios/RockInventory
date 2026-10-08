// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Inventory/RockInventoryConfig.h"
#include "Inventory/RockInventoryData.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace
{
	FRockInventorySlotHandle ItemIndexSlot(const FRockInventoryData& Data, int32 Column)
	{
		return FRockInventorySlotHandle(Data.Sections[0].GetFirstSlotIndex() + Column);
	}
}

// Item index allocation (T-79): a new stack takes the lowest free index, so the client's plain-data prediction and the server's
// inventory pick the same handle from the same data, whatever order stacks were freed in.
TEST_CLASS(RockInventoryItemIndexTests, "BRS.RockInventory.ItemIndex")
{
	FRockInventoryFixture Fixture;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;
	URockItemDefinition* Apple = nullptr;

	BEFORE_EACH()
	{
		Fixture.InitGrid(4, 1);
		Apple = Fixture.MakeDefinition("Apple");
	}

	URockInventory* MakeSecondInventory()
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 4, 1, ERockItemSizePolicy::RespectSize)};
		URockInventory* Inventory = NewObject<URockInventory>(Fixture.Owner);
		Inventory->Owner = Fixture.Owner;
		Inventory->Init(Config);
		KeepAlive.Emplace(Config);
		KeepAlive.Emplace(Inventory);
		return Inventory;
	}

	TEST_METHOD(PlainData_NewStackTakesTheLowestFreeIndex_WhateverOrderStacksWereFreed)
	{
		FRockInventoryData Data;
		Data.Init({FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 4, 1, ERockItemSizePolicy::RespectSize)});
		for (int32 Column = 0; Column < 4; ++Column)
		{
			Data.PlaceStack(FRockItemStack(Apple, 1), ItemIndexSlot(Data, Column));
		}
		FRockInventoryChangeSet Changes;
		Data.ApplyRemove(ItemIndexSlot(Data, 0), 0, Changes);
		Data.ApplyRemove(ItemIndexSlot(Data, 2), 0, Changes);

		const FRockItemStackHandle First = Data.PlaceStack(FRockItemStack(Apple, 1), ItemIndexSlot(Data, 0));
		const FRockItemStackHandle Second = Data.PlaceStack(FRockItemStack(Apple, 1), ItemIndexSlot(Data, 2));

		ASSERT_THAT(AreEqual(0, First.GetIndex()));
		ASSERT_THAT(AreEqual(2, Second.GetIndex()));
	}

	TEST_METHOD(LiveInventory_NewStackTakesTheLowestFreeIndex_WhateverOrderStacksWereFreed)
	{
		URockInventory* Inventory = Fixture.Inventory;
		FRockItemStackHandle Handles[4];
		for (FRockItemStackHandle& Handle : Handles)
		{
			Handle = Inventory->AddItemToInventory(FRockItemStack(Apple, 1));
		}
		Inventory->RemoveItemFromInventory(Handles[0]);
		Inventory->RemoveItemFromInventory(Handles[2]);

		const FRockItemStackHandle First = Inventory->AddItemToInventory(FRockItemStack(Apple, 1));
		const FRockItemStackHandle Second = Inventory->AddItemToInventory(FRockItemStack(Apple, 1));

		ASSERT_THAT(AreEqual(0, First.GetIndex()));
		ASSERT_THAT(AreEqual(2, Second.GetIndex()));
		// A reused index carries the next generation, so the freed handle stays stale
		ASSERT_THAT(IsFalse(Inventory->IsHandleValid(Handles[0])));
	}

	TEST_METHOD(APredictedCrossInventoryMove_GetsTheHandleTheServerAllocates)
	{
		URockInventory* Source = MakeSecondInventory();
		URockInventory* Target = Fixture.Inventory;

		// Source holds the item to move
		const FRockItemStackHandle Moving = Source->AddItemToInventory(FRockItemStack(Apple, 1));
		const FRockInventorySlotHandle SourceSlot(Source->GetSectionInfo(RockInventoryTags::Inventory_Section_Backpack).GetFirstSlotIndex());
		FRockInventorySlotEntry SourceEntry = Source->GetSlotByHandle(SourceSlot);
		SourceEntry.ItemHandle = Moving;
		Source->SetSlotByHandle(SourceSlot, SourceEntry);

		// Target has churn: stacks freed in descending order, so a free list in free order would not hand out the lowest index
		FRockItemStackHandle Churn[4];
		for (FRockItemStackHandle& Handle : Churn)
		{
			Handle = Target->AddItemToInventory(FRockItemStack(Apple, 1));
		}
		Target->RemoveItemFromInventory(Churn[3]);
		Target->RemoveItemFromInventory(Churn[1]);
		const FRockInventorySlotHandle TargetSlot(Target->GetSectionInfo(RockInventoryTags::Inventory_Section_Backpack).GetFirstSlotIndex() + 2);

		// What the client predicts on copies of the replicated data
		FRockInventoryData PredictedSource = FRockInventoryData::FromInventory(Source);
		FRockInventoryData PredictedTarget = FRockInventoryData::FromInventory(Target);
		FRockInventoryChangeSet Changes;
		ASSERT_THAT(AreEqual(ERockMoveRefusal::None,
			FRockInventoryData::ApplyMove(PredictedSource, SourceSlot, PredictedTarget, TargetSlot, FRockMoveItemParams(), Changes)));
		const FRockItemStackHandle Predicted = PredictedTarget.GetSlot(TargetSlot)->ItemHandle;

		// What the server does
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Source, SourceSlot, Target, TargetSlot, FRockMoveItemParams())));
		const FRockItemStackHandle Actual = Target->GetSlotByHandle(TargetSlot).ItemHandle;

		ASSERT_THAT(IsTrue(Predicted.IsValid()));
		ASSERT_THAT(IsTrue(Predicted == Actual));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
