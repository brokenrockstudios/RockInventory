// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"
#include "RockInventoryTestListener.h"

#include "Inventory/RockInventoryData.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

/** Stands in for the replication system: applies a server inventory's replicated arrays to a client inventory and runs the callbacks. */
struct FRockInventoryTestAccess
{
	/** Copies the replicated state and calls the array callbacks. The caller decides when PostNetReceive runs. */
	static void ApplyReplicated(URockInventory* Client, const URockInventory* Server)
	{
		const TArray<FRockItemStack> OldItems = Client->ItemData.AllSlots;
		const TArray<FRockInventorySlotEntry> OldSlots = Client->SlotData.AllSlots;

		Client->Revision = Server->Revision;
		Client->ItemData.AllSlots = Server->ItemData.AllSlots;
		for (int32 Index = 0; Index < OldSlots.Num(); ++Index)
		{
			// LastKnownItemHandle is not replicated: it stays what this client saw last
			FRockInventorySlotEntry Entry = Server->SlotData.AllSlots[Index];
			Entry.LastKnownItemHandle = OldSlots[Index].LastKnownItemHandle;
			Client->SlotData.AllSlots[Index] = Entry;
		}

		TArray<int32> ItemsAdded;
		TArray<int32> ItemsChanged;
		for (int32 Index = 0; Index < Client->ItemData.AllSlots.Num(); ++Index)
		{
			if (Index >= OldItems.Num())
			{
				ItemsAdded.Add(Index);
			}
			else if (OldItems[Index] != Client->ItemData.AllSlots[Index])
			{
				ItemsChanged.Add(Index);
			}
		}
		TArray<int32> SlotsChanged;
		for (int32 Index = 0; Index < OldSlots.Num(); ++Index)
		{
			if (OldSlots[Index] != Client->SlotData.AllSlots[Index])
			{
				SlotsChanged.Add(Index);
			}
		}
		Client->ItemData.PostReplicatedAdd(ItemsAdded, Client->ItemData.AllSlots.Num());
		Client->ItemData.PostReplicatedChange(ItemsChanged, Client->ItemData.AllSlots.Num());
		Client->SlotData.PostReplicatedChange(SlotsChanged, Client->SlotData.AllSlots.Num());
	}

	static void Receive(URockInventory* Client, const URockInventory* Server)
	{
		ApplyReplicated(Client, Server);
		Client->PostNetReceive();
	}
};

// Change sets (batches), the replicated Revision and the item-to-slot index.
TEST_CLASS(RockInventoryChangeSetTests, "BRS.RockInventory.ChangeSet")
{
	FRockInventoryFixture Fixture;
	FRockInventoryFixture Other;
	TArray<TStrongObjectPtr<URockInventoryTestListener>> Listeners;

	URockInventoryTestListener* Listen(URockInventory* Inventory)
	{
		URockInventoryTestListener* Listener = NewObject<URockInventoryTestListener>(GetTransientPackage());
		Listeners.Emplace(Listener);
		Inventory->OnChangeBatch.AddDynamic(Listener, &URockInventoryTestListener::OnChangeBatch);
		Inventory->OnSlotChanged.AddDynamic(Listener, &URockInventoryTestListener::OnSlotChanged);
		Inventory->OnItemChanged.AddDynamic(Listener, &URockInventoryTestListener::OnItemChanged);
		return Listener;
	}

	/** The slot the old way: a scan over every slot. */
	static const FRockInventorySlotEntry* FindSlotByScan(const URockInventory* Inventory, const FRockItemStackHandle& Handle)
	{
		const FRockInventorySlotEntry* Found = nullptr;
		Inventory->ForEachSlotInSection([&](const FRockInventorySectionInfo&, const FRockInventorySlotEntry& Slot)
		{
			if (Slot.ItemHandle == Handle)
			{
				Found = &Slot;
				return false;
			}
			return true;
		});
		return Found;
	}

	static bool LookupMatchesScan(const URockInventory* Inventory, const TArray<FRockItemStackHandle>& Handles)
	{
		for (const FRockItemStackHandle& Handle : Handles)
		{
			const FRockInventorySlotEntry* Indexed = Inventory->GetSlotByItemHandlePtr(Handle);
			const FRockInventorySlotEntry* Scanned = FindSlotByScan(Inventory, Handle);
			if ((Indexed == nullptr) != (Scanned == nullptr) || (Indexed && Indexed->SlotHandle != Scanned->SlotHandle))
			{
				return false;
			}
		}
		return true;
	}

	TEST_METHOD(Move_WithinOneInventory_IsOneBatchHoldingBothSlots)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture.Inventory);

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(1, Listener->Batches.Num()));
		const FRockInventoryChangeBatch& Batch = Listener->Batches[0];
		ASSERT_THAT(IsTrue(Batch.Inventory == Fixture.Inventory));
		ASSERT_THAT(AreEqual(2, Batch.SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Batch.SlotDeltas[0].SlotHandle));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemRemoved, Batch.SlotDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Batch.SlotDeltas[1].SlotHandle));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemAdded, Batch.SlotDeltas[1].ChangeType));
		ASSERT_THAT(AreEqual(0, Batch.ItemDeltas.Num()));
		ASSERT_THAT(IsTrue(Fixture.Inventory->IsHandleValid(Handle)));
	}

	TEST_METHOD(Move_RaisesTheRevisionOnce_AndTheBatchCarriesIt)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		const uint32 Before = Fixture.Inventory->GetRevision();
		URockInventoryTestListener* Listener = Listen(Fixture.Inventory);

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		ASSERT_THAT(AreEqual(Before + 1, Fixture.Inventory->GetRevision()));
		ASSERT_THAT(AreEqual(1, Listener->Batches.Num()));
		ASSERT_THAT(AreEqual(static_cast<int32>(Before + 1), Listener->Batches[0].Revision));
	}

	TEST_METHOD(EachOperation_RaisesTheRevisionByOne)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Arrow = Fixture.MakeDefinition("Arrow", 10);
		const uint32 Start = Fixture.Inventory->GetRevision();

		FRockInventorySlotHandle Slot;
		int32 Excess = 0;
		ASSERT_THAT(IsTrue(Fixture.Loot(Arrow, 4, Slot, Excess)));
		ASSERT_THAT(AreEqual(Start + 1, Fixture.Inventory->GetRevision()));

		const FRockItemStackHandle Handle = Fixture.Inventory->GetSlotByHandle(Slot).ItemHandle;
		Fixture.Inventory->SetItemStackCount(Handle, 7);
		ASSERT_THAT(AreEqual(Start + 2, Fixture.Inventory->GetRevision()));

		URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Slot, 3);
		ASSERT_THAT(AreEqual(Start + 3, Fixture.Inventory->GetRevision()));

		URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Slot);
		ASSERT_THAT(AreEqual(Start + 4, Fixture.Inventory->GetRevision()));
	}

	TEST_METHOD(Loot_NewStack_IsOneBatchWithTheItemAndTheSlot)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		URockInventoryTestListener* Listener = Listen(Fixture.Inventory);

		FRockInventorySlotHandle Slot;
		int32 Excess = 0;
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));

		ASSERT_THAT(AreEqual(1, Listener->Batches.Num()));
		ASSERT_THAT(AreEqual(1, Listener->Batches[0].ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Added, Listener->Batches[0].ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(1, Listener->Batches[0].SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemAdded, Listener->Batches[0].SlotDeltas[0].ChangeType));
	}

	TEST_METHOD(SplitWholeStack_IsOneBatchWithTheRemovalAndTheEmptiedSlot)
	{
		Fixture.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture.Inventory);

		URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Fixture.SlotAt(0, 0));

		ASSERT_THAT(AreEqual(1, Listener->Batches.Num()));
		ASSERT_THAT(AreEqual(1, Listener->Batches[0].ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Removed, Listener->Batches[0].ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(1, Listener->Batches[0].SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockSlotChangeType::ItemRemoved, Listener->Batches[0].SlotDeltas[0].ChangeType));
	}

	TEST_METHOD(OperationThatChangesNothing_BroadcastsNoBatchAndKeepsTheRevision)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		const uint32 Before = Fixture.Inventory->GetRevision();
		URockInventoryTestListener* Listener = Listen(Fixture.Inventory);

		// Onto its own slot with the same orientation, and a slot rewritten with what it already holds
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(0, 0))));
		Fixture.Inventory->SetSlotByHandle(Fixture.SlotAt(0, 0), Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)));

		ASSERT_THAT(AreEqual(0, Listener->Batches.Num()));
		ASSERT_THAT(AreEqual(Before, Fixture.Inventory->GetRevision()));
	}

	TEST_METHOD(Move_AcrossInventories_IsOneBatchAndOneRevisionStepPerInventory)
	{
		Fixture.InitGrid(1, 1);
		Other.InitGrid(1, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		const uint32 SourceBefore = Fixture.Inventory->GetRevision();
		const uint32 TargetBefore = Other.Inventory->GetRevision();
		URockInventoryTestListener* SourceListener = Listen(Fixture.Inventory);
		URockInventoryTestListener* TargetListener = Listen(Other.Inventory);

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Other.Inventory, Other.SlotAt(0, 0))));

		ASSERT_THAT(AreEqual(1, SourceListener->Batches.Num()));
		ASSERT_THAT(AreEqual(1, SourceListener->Batches[0].ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Removed, SourceListener->Batches[0].ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(1, TargetListener->Batches.Num()));
		ASSERT_THAT(AreEqual(ERockItemChangeType::Added, TargetListener->Batches[0].ItemDeltas[0].ChangeType));
		ASSERT_THAT(AreEqual(SourceBefore + 1, Fixture.Inventory->GetRevision()));
		ASSERT_THAT(AreEqual(TargetBefore + 1, Other.Inventory->GetRevision()));
	}

	TEST_METHOD(OperationScope_GroupsSeveralCallsIntoOneBatchAndOneRevisionStep)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		const uint32 Before = Fixture.Inventory->GetRevision();
		URockInventoryTestListener* Listener = Listen(Fixture.Inventory);

		{
			FRockInventoryOperationScope Outer(Fixture.Inventory);
			{
				FRockInventoryOperationScope Inner(Fixture.Inventory);
				Fixture.Inventory->SetItemStackCount(Handle, 1);
				URockInventoryLibrary::SetCustomValue1(Fixture.Inventory, Handle, 5);
			}
			// The inner scope ending does not flush
			ASSERT_THAT(AreEqual(0, Listener->Batches.Num()));
			ASSERT_THAT(AreEqual(0, Listener->ItemDeltas.Num()));
		}

		ASSERT_THAT(AreEqual(1, Listener->Batches.Num()));
		ASSERT_THAT(AreEqual(2, Listener->Batches[0].ItemDeltas.Num()));
		ASSERT_THAT(AreEqual(Before + 1, Fixture.Inventory->GetRevision()));
	}

	TEST_METHOD(Listeners_AreToldAfterTheOperationIsComplete)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryTestListener* Listener = Listen(Fixture.Inventory);
		int32 Callbacks = 0;
		int32 SawFinishedState = 0;
		Listener->ProbeFunction = [&]()
		{
			++Callbacks;
			const bool bSourceEmpty = !Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(0, 0)).ItemHandle.IsValid();
			const bool bTargetFilled = Fixture.Inventory->GetSlotByHandle(Fixture.SlotAt(1, 0)).ItemHandle.IsValid();
			SawFinishedState += (bSourceEmpty && bTargetFilled) ? 1 : 0;
		};

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));

		// One batch and the two legacy slot deltas, every one of them after both slots were updated
		ASSERT_THAT(AreEqual(3, Callbacks));
		ASSERT_THAT(AreEqual(3, SawFinishedState));
	}

	TEST_METHOD(LegacyDelegates_AreReplayedFromTheBatch_InTheOrderTheChangesWereMade)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		URockInventoryTestListener* Listener = Listen(Fixture.Inventory);

		FRockInventorySlotHandle Slot;
		int32 Excess = 0;
		ASSERT_THAT(IsTrue(Fixture.Loot(Apple, 1, Slot, Excess)));

		// The batch first, then the new stack's item delta, then its slot delta
		ASSERT_THAT(AreEqual(FString(TEXT("BIS")), Listener->CallOrder));
	}

	TEST_METHOD(Client_ReplicatedChanges_AreOneBatchFlushedInPostNetReceive)
	{
		Fixture.InitGrid(3, 1);
		Other.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		FRockInventoryTestAccess::Receive(Other.Inventory, Fixture.Inventory);
		URockInventoryTestListener* Listener = Listen(Other.Inventory);

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 0))));
		FRockInventoryTestAccess::ApplyReplicated(Other.Inventory, Fixture.Inventory);

		// The arrays have been applied but the update is not over: nothing is broadcast yet
		ASSERT_THAT(AreEqual(0, Listener->Batches.Num()));
		ASSERT_THAT(AreEqual(0, Listener->SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(0, Listener->ItemDeltas.Num()));

		Other.Inventory->PostNetReceive();

		ASSERT_THAT(AreEqual(1, Listener->Batches.Num()));
		ASSERT_THAT(AreEqual(2, Listener->Batches[0].SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("BSS")), Listener->CallOrder));
		// The client never raises the revision itself, it reports the replicated one
		ASSERT_THAT(AreEqual(Fixture.Inventory->GetRevision(), Other.Inventory->GetRevision()));
		ASSERT_THAT(AreEqual(static_cast<int32>(Fixture.Inventory->GetRevision()), Listener->Batches[0].Revision));
	}

	TEST_METHOD(Client_SeveralServerOperationsInOneUpdate_AreOneBatchWithTheLatestRevision)
	{
		Fixture.InitGrid(3, 1);
		Other.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		FRockInventoryTestAccess::Receive(Other.Inventory, Fixture.Inventory);
		URockInventoryTestListener* Listener = Listen(Other.Inventory);
		const uint32 Before = Fixture.Inventory->GetRevision();

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(1, 0))));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(1, 0), Fixture.Inventory, Fixture.SlotAt(2, 0))));
		FRockInventoryTestAccess::Receive(Other.Inventory, Fixture.Inventory);

		ASSERT_THAT(AreEqual(Before + 2, Fixture.Inventory->GetRevision()));
		ASSERT_THAT(AreEqual(1, Listener->Batches.Num()));
		ASSERT_THAT(AreEqual(static_cast<int32>(Before + 2), Listener->Batches[0].Revision));
	}

	TEST_METHOD(ItemToSlotLookup_FollowsAMove)
	{
		Fixture.InitGrid(3, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Handle = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		ASSERT_THAT(IsNotNull(Fixture.Inventory->GetSlotByItemHandlePtr(Handle)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(0, 0), Fixture.Inventory->GetSlotByItemHandlePtr(Handle)->SlotHandle));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Fixture.Inventory, Fixture.SlotAt(2, 0))));

		ASSERT_THAT(IsNotNull(Fixture.Inventory->GetSlotByItemHandlePtr(Handle)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(2, 0), Fixture.Inventory->GetSlotByItemHandlePtr(Handle)->SlotHandle));
	}

	TEST_METHOD(ItemToSlotLookup_IsNullForAStaleHandleAndForAnItemInNoSlot)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle Removed = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Fixture.SlotAt(0, 0));
		// A live stack that no slot points at yet
		const FRockItemStackHandle Unplaced = Fixture.Inventory->AddItemToInventory(FRockItemStack(Apple, 1));

		ASSERT_THAT(IsNull(Fixture.Inventory->GetSlotByItemHandlePtr(Removed)));
		ASSERT_THAT(IsNull(Fixture.Inventory->GetSlotByItemHandlePtr(Unplaced)));
		ASSERT_THAT(IsNull(Fixture.Inventory->GetSlotByItemHandlePtr(FRockItemStackHandle::Invalid())));
	}

	TEST_METHOD(ItemToSlotLookup_AfterTheIndexIsReused_ReturnsTheNewOwnerOnly)
	{
		Fixture.InitGrid(2, 1);
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		const FRockItemStackHandle First = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(0, 0));
		URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, Fixture.SlotAt(0, 0));
		// Reuses the freed item index with a newer generation, in the other slot
		const FRockItemStackHandle Second = Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(1, 0));

		ASSERT_THAT(AreEqual(First.GetIndex(), Second.GetIndex()));
		ASSERT_THAT(IsNull(Fixture.Inventory->GetSlotByItemHandlePtr(First)));
		ASSERT_THAT(IsNotNull(Fixture.Inventory->GetSlotByItemHandlePtr(Second)));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(1, 0), Fixture.Inventory->GetSlotByItemHandlePtr(Second)->SlotHandle));
	}

	TEST_METHOD(ItemToSlotLookup_MatchesAFullScan_OnTheServerAndOnAClientCopy)
	{
		Fixture.InitGrid(6, 4);
		Other.InitGrid(6, 4);
		URockItemDefinition* Definitions[] = {
			Fixture.MakeDefinition("Sword"),
			Fixture.MakeDefinition("Arrow", 5),
			Fixture.MakeDefinition("Rifle", 1, FIntPoint(2, 1)),
		};
		const int32 NumSlots = 6 * 4;
		FRandomStream Random(20261006);
		TArray<FRockItemStackHandle> Seen;

		for (int32 Step = 0; Step < 250; ++Step)
		{
			switch (Random.RandRange(0, 3))
			{
			case 0:
			{
				FRockInventorySlotHandle Slot;
				int32 Excess = 0;
				Fixture.Loot(Definitions[Random.RandRange(0, 2)], Random.RandRange(1, 3), Slot, Excess);
				break;
			}
			case 1:
			{
				const FRockInventorySlotHandle From(Random.RandRange(0, NumSlots - 1));
				const FRockInventorySlotHandle To(Random.RandRange(0, NumSlots - 1));
				const FRockInventoryData Data = FRockInventoryData::FromInventory(Fixture.Inventory);
				if (FRockInventoryData::CanMove(Data, From, Data, To, FRockMoveItemParams()) == ERockMoveRefusal::None)
				{
					URockInventoryLibrary::MoveItem(Fixture.Inventory, From, Fixture.Inventory, To);
				}
				break;
			}
			case 2:
			{
				const FRockInventorySlotHandle From(Random.RandRange(0, NumSlots - 1));
				if (Fixture.Inventory->GetItemBySlotHandle(From).IsValid())
				{
					URockInventoryLibrary::SplitItemStackAtLocation(Fixture.Inventory, From, Random.RandRange(1, 2));
				}
				break;
			}
			default:
				URockInventoryLibrary::RemoveItemsById(Fixture.Inventory, Definitions[Random.RandRange(0, 2)]->ItemId, 1, false);
				break;
			}
			FRockInventoryTestAccess::Receive(Other.Inventory, Fixture.Inventory);

			Fixture.Inventory->ForEachItemStack([&](const FRockItemStack& Stack)
			{
				if (Stack.IsValid()) { Seen.AddUnique(Stack.ItemHandle); }
				return true;
			});
			ASSERT_THAT(IsTrue(LookupMatchesScan(Fixture.Inventory, Seen)));
			ASSERT_THAT(IsTrue(LookupMatchesScan(Other.Inventory, Seen)));
		}
		// The run had to place things for the comparison to mean something
		ASSERT_THAT(IsTrue(Seen.Num() > 10));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
