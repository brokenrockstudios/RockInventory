// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"
#include "RockInventoryTestManager.h"

#include "Client/RockInventoryClientModel.h"
#include "Client/RockInventoryPrediction.h"
#include "Components/RockInventoryManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/RockInventoryConfig.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace
{
	FRockInventorySlotHandle PredSlot(const FRockInventoryData& Data, int32 Column, int32 Row = 0)
	{
		const FRockInventorySectionInfo& Section = Data.Sections[0];
		return FRockInventorySlotHandle(Section.GetFirstSlotIndex() + Row * Section.GetColumns() + Column);
	}

	FRockInventorySectionInfo PredGrid(int32 Columns, int32 Rows)
	{
		return FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, Columns, Rows, ERockItemSizePolicy::RespectSize);
	}

	bool SameState(const FRockInventoryData& A, const FRockInventoryData& B)
	{
		return FRockInventoryPresentationDiff::Between(A, ERockInventorySyncState::Live, B, ERockInventorySyncState::Live).IsEmpty();
	}
}

// The pure part of prediction (T-78): the queue of pending moves, its preconditions, re-application on fresh replicated data, the
// settle rule and the in-flight limits. Plain data, no inventory object, no world: what the server runs is the same ApplyMove.
TEST_CLASS(RockInventoryPredictionQueueTests, "BRS.RockInventory.Prediction.Queue")
{
	static constexpr uint32 KeyA = 1;
	static constexpr uint32 KeyB = 2;

	TArray<TStrongObjectPtr<UObject>> KeepAlive;
	FRockInventoryData Base;
	FRockPredictionQueue Queue;
	URockItemDefinition* Apple = nullptr;

	URockItemDefinition* Def(FName Id, int32 MaxStack = 1, FIntPoint GridSize = FIntPoint(1, 1))
	{
		URockItemDefinition* Definition = NewObject<URockItemDefinition>(GetTransientPackage());
		Definition->ItemId = Id;
		Definition->MaxStackCount = MaxStack;
		Definition->GridSize = GridSize;
		KeepAlive.Emplace(Definition);
		return Definition;
	}

	BEFORE_EACH()
	{
		Apple = Def("Apple");
		Base.Init({PredGrid(4, 1)});
		Base.PlaceStack(FRockItemStack(Apple, 1), PredSlot(Base, 0));
	}

	FRockPredictedMove Move(int32 Sequence, uint32 FromKey, int32 FromColumn, uint32 ToKey, int32 ToColumn) const
	{
		FRockPredictedMove Result;
		Result.Sequence = Sequence;
		Result.SourceKey = FromKey;
		Result.SourceSlot = PredSlot(Base, FromColumn);
		Result.TargetKey = ToKey;
		Result.TargetSlot = PredSlot(Base, ToColumn);
		return Result;
	}

	/** What the server does with the same move on the same data. */
	FRockInventoryData ServerResult(const FRockInventoryData& From, int32 FromColumn, int32 ToColumn, const FRockMoveItemParams& Params = FRockMoveItemParams()) const
	{
		FRockInventoryData Server = From;
		FRockInventoryChangeSet Changes;
		FRockInventoryData::ApplyMove(Server, PredSlot(Server, FromColumn), Server, PredSlot(Server, ToColumn), Params, Changes);
		return Server;
	}

	TEST_METHOD(APredictedMove_EqualsTheServersResult)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);

		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0)));

		ASSERT_THAT(IsTrue(SameState(State[KeyA], ServerResult(Base, 0, 2))));
		ASSERT_THAT(AreEqual(1, Queue.Num()));
	}

	TEST_METHOD(APredictedMove_CapturesWhatTheSlotsHeld)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0);

		const FRockPredictedMove& Queued = Queue.GetPending()[0];

		ASSERT_THAT(IsTrue(Queued.ExpectedSource.bCheck));
		ASSERT_THAT(IsFalse(Queued.ExpectedSource.bEmpty));
		ASSERT_THAT(IsTrue(Queued.ExpectedSource.Definition == Apple));
		ASSERT_THAT(AreEqual(1, Queued.ExpectedSource.Count));
		ASSERT_THAT(IsTrue(Queued.ExpectedTarget.bCheck));
		ASSERT_THAT(IsTrue(Queued.ExpectedTarget.bEmpty));
	}

	TEST_METHOD(AMoveTheStateRefuses_QueuesNothingAndChangesNothing)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);

		// Slot 1 is empty: nothing to move
		const ERockMoveRefusal Refusal = Queue.Predict(State, Move(1, KeyA, 1, KeyA, 2), 0.0);

		ASSERT_THAT(AreEqual(ERockMoveRefusal::EmptySource, Refusal));
		ASSERT_THAT(IsTrue(Queue.IsEmpty()));
		ASSERT_THAT(IsTrue(SameState(State[KeyA], Base)));
	}

	TEST_METHOD(AnInventoryMissingFromTheState_IsRefused)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);

		ASSERT_THAT(AreEqual(ERockMoveRefusal::InvalidSourceSlot, Queue.Predict(State, Move(1, KeyA, 0, KeyB, 2), 0.0)));
		ASSERT_THAT(IsTrue(Queue.IsEmpty()));
	}

	TEST_METHOD(TwoChainedMoves_ReapplyInOrderOnFreshData)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 1), 0.0);
		// The second move starts where the first one ends: only the shown state has the item at slot 1
		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Queue.Predict(State, Move(2, KeyA, 1, KeyA, 3), 0.0)));

		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Base);
		const int32 Skipped = Queue.Rebase(Fresh);

		ASSERT_THAT(AreEqual(0, Skipped));
		ASSERT_THAT(IsTrue(SameState(Fresh[KeyA], State[KeyA])));
		ASSERT_THAT(IsTrue(SameState(Fresh[KeyA], ServerResult(ServerResult(Base, 0, 1), 1, 3))));
	}

	TEST_METHOD(AMoveBetweenTwoInventories_ReappliesOnBoth)
	{
		FRockInventoryData Other;
		Other.Init({PredGrid(4, 1)});
		FRockPredictionState State;
		State.Add(KeyA, Base);
		State.Add(KeyB, Other);
		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Queue.Predict(State, Move(1, KeyA, 0, KeyB, 2), 0.0)));

		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Base);
		Fresh.Add(KeyB, Other);
		Queue.Rebase(Fresh);

		ASSERT_THAT(IsFalse(Fresh[KeyA].GetSlot(PredSlot(Base, 0))->ItemHandle.IsValid()));
		ASSERT_THAT(IsTrue(Fresh[KeyB].GetSlotStack(PredSlot(Other, 2)) != nullptr));
		ASSERT_THAT(IsTrue(Fresh[KeyB].GetSlotStack(PredSlot(Other, 2))->GetDefinition() == Apple));
	}

	TEST_METHOD(AMoveWhoseSourceChanged_IsSkippedAndNeverHalfApplied)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0);
		// Someone else took the apple out of slot 0 and put a pear into slot 3
		FRockInventoryData Changed;
		Changed.Init({PredGrid(4, 1)});
		Changed.PlaceStack(FRockItemStack(Def("Pear"), 1), PredSlot(Changed, 3));
		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Changed);

		const int32 Skipped = Queue.Rebase(Fresh);

		ASSERT_THAT(AreEqual(1, Skipped));
		ASSERT_THAT(IsTrue(SameState(Fresh[KeyA], Changed)));
	}

	TEST_METHOD(AMoveWhoseTargetChanged_IsSkipped)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0);
		FRockInventoryData Changed = Base;
		Changed.PlaceStack(FRockItemStack(Def("Pear"), 1), PredSlot(Changed, 2));
		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Changed);

		ASSERT_THAT(AreEqual(1, Queue.Rebase(Fresh)));
		ASSERT_THAT(IsTrue(SameState(Fresh[KeyA], Changed)));
	}

	TEST_METHOD(AMoveWhoseSourceNowHoldsAnotherItem_IsSkipped_NotAppliedToTheOtherItem)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0);
		// The apple is gone and a pear sits in its slot: the move would otherwise carry the pear
		FRockInventoryData Changed;
		Changed.Init({PredGrid(4, 1)});
		Changed.PlaceStack(FRockItemStack(Def("Pear"), 1), PredSlot(Changed, 0));
		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Changed);

		ASSERT_THAT(AreEqual(1, Queue.Rebase(Fresh)));

		ASSERT_THAT(IsTrue(Fresh[KeyA].GetSlotStack(PredSlot(Changed, 0)) != nullptr));
		ASSERT_THAT(IsTrue(Fresh[KeyA].GetSlotStack(PredSlot(Changed, 2)) == nullptr));
	}

	TEST_METHOD(AMergeWhoseTargetStackChanged_IsSkipped_NotMergedIntoTheNewCount)
	{
		URockItemDefinition* Arrow = Def("Arrow", 10);
		FRockInventoryData Arrows;
		Arrows.Init({PredGrid(4, 1)});
		Arrows.PlaceStack(FRockItemStack(Arrow, 3), PredSlot(Arrows, 0));
		Arrows.PlaceStack(FRockItemStack(Arrow, 2), PredSlot(Arrows, 1));
		FRockPredictionState State;
		State.Add(KeyA, Arrows);
		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Queue.Predict(State, Move(1, KeyA, 0, KeyA, 1), 0.0)));
		ASSERT_THAT(AreEqual(5, State[KeyA].GetSlotStack(PredSlot(Arrows, 1))->GetStackCount()));
		// Someone added two arrows to the target stack in the meantime
		FRockInventoryData Changed;
		Changed.Init({PredGrid(4, 1)});
		Changed.PlaceStack(FRockItemStack(Arrow, 3), PredSlot(Changed, 0));
		Changed.PlaceStack(FRockItemStack(Arrow, 4), PredSlot(Changed, 1));
		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Changed);

		ASSERT_THAT(AreEqual(1, Queue.Rebase(Fresh)));

		ASSERT_THAT(AreEqual(4, Fresh[KeyA].GetSlotStack(PredSlot(Changed, 1))->GetStackCount()));
		ASSERT_THAT(AreEqual(3, Fresh[KeyA].GetSlotStack(PredSlot(Changed, 0))->GetStackCount()));
	}

	TEST_METHOD(DataThatAlreadyHoldsTheMove_IsNotMovedAgain)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0);
		// The replicated data arrived before the ack: it already has the move
		const FRockInventoryData Replicated = ServerResult(Base, 0, 2);
		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Replicated);

		ASSERT_THAT(AreEqual(1, Queue.Rebase(Fresh)));
		ASSERT_THAT(IsTrue(SameState(Fresh[KeyA], Replicated)));
	}

	TEST_METHOD(ARotation_IsNotRotatedBackWhenTheDataAlreadyHasIt)
	{
		FRockInventoryData Grid;
		Grid.Init({PredGrid(3, 3)});
		Grid.PlaceStack(FRockItemStack(Def("Plank", 1, FIntPoint(2, 1)), 1), PredSlot(Grid, 0));
		FRockPredictedMove Rotate = Move(1, KeyA, 0, KeyA, 0);
		Rotate.Params.DesiredOrientation = ERockItemOrientation::Vertical;
		FRockPredictionState State;
		State.Add(KeyA, Grid);
		ASSERT_THAT(AreEqual(ERockMoveRefusal::None, Queue.Predict(State, Rotate, 0.0)));
		ASSERT_THAT(AreEqual(ERockItemOrientation::Vertical, State[KeyA].GetSlot(PredSlot(Grid, 0))->Orientation));
		const FRockInventoryData Replicated = State[KeyA];
		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Replicated);

		Queue.Rebase(Fresh);

		ASSERT_THAT(AreEqual(ERockItemOrientation::Vertical, Fresh[KeyA].GetSlot(PredSlot(Grid, 0))->Orientation));
	}

	TEST_METHOD(ARefusedMove_LeavesTheQueue_AndTheNextRebaseShowsTheDataAgain)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0);

		Queue.Ack(1, false, {});
		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Base);
		Queue.Rebase(Fresh);

		ASSERT_THAT(IsTrue(Queue.IsEmpty()));
		ASSERT_THAT(IsTrue(SameState(Fresh[KeyA], Base)));
	}

	TEST_METHOD(AnAckForAnUnknownMove_ChangesNothing)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0);

		Queue.Ack(99, false, {});

		ASSERT_THAT(AreEqual(1, Queue.Num()));
	}

	TEST_METHOD(AnAckedMove_StaysUntilTheReplicatedRevisionReachesTheAck)
	{
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 2), 0.0);
		Queue.Ack(1, true, {FRockKeyedRevision(KeyA, 5)});

		const int32 EarlySettled = Queue.Settle([](uint32, int32 Revision) { return 4 >= Revision; });
		ASSERT_THAT(AreEqual(0, EarlySettled));
		ASSERT_THAT(AreEqual(1, Queue.Num()));

		const int32 Settled = Queue.Settle([](uint32, int32 Revision) { return 5 >= Revision; });
		ASSERT_THAT(AreEqual(1, Settled));
		ASSERT_THAT(IsTrue(Queue.IsEmpty()));
	}

	TEST_METHOD(AMoveBetweenTwoInventories_SettlesOnlyWhenBothReachedTheirRevision)
	{
		FRockInventoryData Other;
		Other.Init({PredGrid(4, 1)});
		FRockPredictionState State;
		State.Add(KeyA, Base);
		State.Add(KeyB, Other);
		Queue.Predict(State, Move(1, KeyA, 0, KeyB, 2), 0.0);
		Queue.Ack(1, true, {FRockKeyedRevision(KeyA, 3), FRockKeyedRevision(KeyB, 7)});

		const int32 OnlyA = Queue.Settle([](uint32 Key, int32) { return Key == KeyA; });
		const int32 Both = Queue.Settle([](uint32, int32) { return true; });

		ASSERT_THAT(AreEqual(0, OnlyA));
		ASSERT_THAT(AreEqual(1, Both));
	}

	TEST_METHOD(RevisionCompare_SurvivesAWrap)
	{
		ASSERT_THAT(IsTrue(FRockPredictionQueue::RevisionReached(5, 5)));
		ASSERT_THAT(IsTrue(FRockPredictionQueue::RevisionReached(6, 5)));
		ASSERT_THAT(IsFalse(FRockPredictionQueue::RevisionReached(4, 5)));
		// The uint32 revision wrapped past INT32_MAX: still "after" the revision just before the wrap
		ASSERT_THAT(IsTrue(FRockPredictionQueue::RevisionReached(MIN_int32, MAX_int32)));
		ASSERT_THAT(IsFalse(FRockPredictionQueue::RevisionReached(MAX_int32, MIN_int32)));
	}

	TEST_METHOD(InFlightCap_StopsPredicting_UntilAnAnswerComes)
	{
		Queue.MaxInFlight = 2;
		FRockPredictionState State;
		State.Add(KeyA, Base);
		ASSERT_THAT(IsTrue(Queue.CanPredict(0.0)));
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 1), 0.0);
		ASSERT_THAT(IsTrue(Queue.CanPredict(0.0)));
		Queue.Predict(State, Move(2, KeyA, 1, KeyA, 2), 0.0);

		ASSERT_THAT(IsFalse(Queue.CanPredict(0.0)));
		ASSERT_THAT(AreEqual(2, Queue.NumInFlight()));

		// An acked move waits for replication, not for the server: it no longer counts against the cap
		Queue.Ack(1, true, {FRockKeyedRevision(KeyA, 9)});
		ASSERT_THAT(AreEqual(1, Queue.NumInFlight()));
		ASSERT_THAT(IsTrue(Queue.CanPredict(0.0)));
	}

	TEST_METHOD(PendingThreshold_HoldsWhileTheOldestMoveIsUnanswered)
	{
		Queue.PendingThresholdSeconds = 0.5;
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 1), 10.0);

		ASSERT_THAT(IsFalse(Queue.IsHolding(10.4)));
		ASSERT_THAT(IsTrue(Queue.CanPredict(10.4)));
		ASSERT_THAT(IsTrue(Queue.IsHolding(10.6)));
		ASSERT_THAT(IsFalse(Queue.CanPredict(10.6)));

		Queue.Ack(1, true, {FRockKeyedRevision(KeyA, 9)});
		ASSERT_THAT(IsFalse(Queue.IsHolding(10.6)));
	}

	TEST_METHOD(AnUnansweredMove_IsAbandonedAfterTheTimeout)
	{
		Queue.AbandonSeconds = 5.0;
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 1), 10.0);

		ASSERT_THAT(AreEqual(0, Queue.Expire(14.9)));
		ASSERT_THAT(AreEqual(1, Queue.Expire(15.1)));

		ASSERT_THAT(IsTrue(Queue.IsEmpty()));
		FRockPredictionState Fresh;
		Fresh.Add(KeyA, Base);
		Queue.Rebase(Fresh);
		ASSERT_THAT(IsTrue(SameState(Fresh[KeyA], Base)));
	}

	TEST_METHOD(AnAckedMoveThatNeverSettles_IsAbandonedCountedFromTheAck)
	{
		Queue.AbandonSeconds = 5.0;
		FRockPredictionState State;
		State.Add(KeyA, Base);
		Queue.Predict(State, Move(1, KeyA, 0, KeyA, 1), 10.0);
		// Answered late, then the inventory stops replicating (closed, out of reach): the revision is never reached
		Queue.Ack(1, true, {FRockKeyedRevision(KeyA, 99)}, 14.0);

		ASSERT_THAT(AreEqual(0, Queue.Expire(18.0)));
		ASSERT_THAT(AreEqual(1, Queue.Expire(19.5)));
	}

	TEST_METHOD(TouchedKeys_ListEachInventoryOnce)
	{
		FRockInventoryData Other;
		Other.Init({PredGrid(4, 1)});
		FRockPredictionState State;
		State.Add(KeyA, Base);
		State.Add(KeyB, Other);
		Queue.Predict(State, Move(1, KeyA, 0, KeyB, 2), 0.0);
		Queue.Predict(State, Move(2, KeyB, 2, KeyA, 1), 0.0);

		TArray<uint32> Keys;
		Queue.GetTouchedKeys(Keys);

		ASSERT_THAT(AreEqual(2, Keys.Num()));
		ASSERT_THAT(IsTrue(Keys.Contains(KeyA) && Keys.Contains(KeyB)));
	}
};

// The manager component with a real inventory and a real client model: a move shows at once, the server's agreement is not a visible
// correction, a refusal snaps back, chained moves apply in order, and input is held at the in-flight cap or the pending threshold.
// The test manager holds back every command it would send; the test plays the server by calling Server_MoveItem_Implementation.
TEST_CLASS(RockInventoryPredictionManagerTests, "BRS.RockInventory.Prediction.Manager")
{
	FRockInventoryFixture Fixture;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;

	APawn* Pawn = nullptr;
	AController* Controller = nullptr;
	URockInventoryTestManager* Manager = nullptr;
	URockInventory* Mine = nullptr;
	URockInventoryClientModel* Model = nullptr;
	URockItemDefinition* Apple = nullptr;
	TArray<FRockInventoryPresentationDiff> Announced;

	URockInventory* MakeInventory(AActor* InOwner)
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 4, 1, ERockItemSizePolicy::RespectSize)};
		URockInventory* Inventory = NewObject<URockInventory>(InOwner);
		Inventory->Owner = InOwner;
		Inventory->Init(Config);
		KeepAlive.Emplace(Config);
		KeepAlive.Emplace(Inventory);
		return Inventory;
	}

	BEFORE_EACH()
	{
		Pawn = &SpawnPawnAt(Fixture.Spawner, FVector::ZeroVector);
		Controller = &Fixture.Spawner.SpawnActor<APlayerController>();
		Controller->SetPawn(Pawn);
		Manager = NewObject<URockInventoryTestManager>(Controller);
		KeepAlive.Emplace(Manager);
		Mine = MakeInventory(Pawn);
		Apple = Fixture.MakeDefinition("Apple");
		Model = URockInventoryClientModelSubsystem::GetModel(Mine);
		ASSERT_THAT(IsNotNull(Model));
		Manager->BindPredictionToModels();
	}

	FRockInventorySlotHandle Slot(int32 Column) const
	{
		return FRockInventorySlotHandle(Mine->GetSectionInfo(RockInventoryTags::Inventory_Section_Backpack).GetFirstSlotIndex() + Column);
	}

	FRockItemStackHandle Place(URockItemDefinition* Definition, int32 Column)
	{
		const FRockItemStackHandle Handle = Mine->AddItemToInventory(FRockItemStack(Definition, 1));
		FRockInventorySlotEntry Entry = Mine->GetSlotByHandle(Slot(Column));
		Entry.ItemHandle = Handle;
		Mine->SetSlotByHandle(Slot(Column), Entry);
		return Handle;
	}

	FRockMoveItemTransaction MoveCommand(int32 From, int32 To) const
	{
		return FRockMoveItemTransaction(Controller, Mine, Slot(From), Mine, Slot(To));
	}

	/** The item the model shows in a column, invalid if none. */
	bool ModelHasItem(int32 Column) const { return Model->GetSlotByHandle(Slot(Column)).ItemHandle.IsValid(); }
	bool InventoryHasItem(int32 Column) const { return Mine->GetSlotByHandle(Slot(Column)).ItemHandle.IsValid(); }

	void ListenToTheModel()
	{
		Model->OnChanged.AddLambda([this](URockInventoryClientModel&, const FRockInventoryPresentationDiff& Diff) { Announced.Add(Diff); });
	}

	TEST_METHOD(AMove_ShowsOnTheModelAtOnce_AndTheInventoryWaitsForTheServer)
	{
		Place(Apple, 0);

		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(0, 1))));

		ASSERT_THAT(IsFalse(ModelHasItem(0)));
		ASSERT_THAT(IsTrue(ModelHasItem(1)));
		ASSERT_THAT(IsTrue(InventoryHasItem(0)));
		ASSERT_THAT(IsFalse(InventoryHasItem(1)));
		ASSERT_THAT(AreEqual(1, Manager->GetNumPendingCommands()));
	}

	TEST_METHOD(ASentMove_CarriesASequenceNumberAndThePreconditions)
	{
		Place(Apple, 0);

		Manager->MoveItem(MoveCommand(0, 1));
		Manager->MoveItem(MoveCommand(1, 2));

		ASSERT_THAT(AreEqual(2, Manager->Sent.Num()));
		ASSERT_THAT(IsTrue(Manager->Sent[0].TransactionID > 0));
		ASSERT_THAT(IsTrue(Manager->Sent[1].TransactionID > Manager->Sent[0].TransactionID));
		ASSERT_THAT(IsTrue(Manager->Sent[0].ExpectedSource.bCheck && !Manager->Sent[0].ExpectedSource.bEmpty));
		ASSERT_THAT(IsTrue(Manager->Sent[0].ExpectedTarget.bCheck && Manager->Sent[0].ExpectedTarget.bEmpty));
	}

	TEST_METHOD(TheServerAgreeing_IsNotAVisibleCorrection)
	{
		const FRockItemStackHandle Handle = Place(Apple, 0);
		ListenToTheModel();
		Manager->MoveItem(MoveCommand(0, 1));
		ASSERT_THAT(AreEqual(1, Announced.Num()));

		Manager->Server_MoveItem_Implementation(Manager->Sent[0]);

		// Replication brought the move and the ack settled it: the model had it already, so nothing more was announced
		ASSERT_THAT(AreEqual(1, Announced.Num()));
		ASSERT_THAT(IsTrue(InventoryHasItem(1)));
		ASSERT_THAT(IsFalse(InventoryHasItem(0)));
		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
		ASSERT_THAT(AreEqual(Handle, Model->GetSlotByHandle(Slot(1)).ItemHandle));
	}

	TEST_METHOD(ARefusedMove_SnapsTheModelBack)
	{
		Place(Apple, 0);
		Manager->MoveItem(MoveCommand(0, 1));
		ASSERT_THAT(IsTrue(ModelHasItem(1)));
		const int32 Sequence = Manager->Sent[0].TransactionID;

		Manager->Client_TransactionResult_Implementation(Sequence, false, {});

		ASSERT_THAT(IsTrue(ModelHasItem(0)));
		ASSERT_THAT(IsFalse(ModelHasItem(1)));
		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
	}

	TEST_METHOD(TwoChainedMoves_ShowAtOnce_AndTheServerRunningThemInOrderChangesNothingVisible)
	{
		Place(Apple, 0);
		ListenToTheModel();

		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(0, 1))));
		// The replicated inventory still has the apple at 0; the shown state has it at 1, and the second move starts there
		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(1, 3))));
		ASSERT_THAT(IsTrue(ModelHasItem(3)));
		ASSERT_THAT(IsFalse(ModelHasItem(1)));
		const int32 AnnouncedBeforeTheServer = Announced.Num();

		Manager->Server_MoveItem_Implementation(Manager->Sent[0]);
		Manager->Server_MoveItem_Implementation(Manager->Sent[1]);

		ASSERT_THAT(AreEqual(AnnouncedBeforeTheServer, Announced.Num()));
		ASSERT_THAT(IsTrue(InventoryHasItem(3)));
		ASSERT_THAT(IsTrue(ModelHasItem(3)));
		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
	}

	TEST_METHOD(AnAckThatArrivesBeforeTheReplicatedState_KeepsThePrediction)
	{
		Place(Apple, 0);
		Manager->MoveItem(MoveCommand(0, 1));
		FRockInventoryRevision Touched;
		Touched.Inventory = Mine;
		Touched.Revision = static_cast<int32>(Mine->GetRevision()) + 1;

		// The ack names the revision the server reached; the replicated inventory has not got there
		Manager->Client_TransactionResult_Implementation(Manager->Sent[0].TransactionID, true, {Touched});

		ASSERT_THAT(AreEqual(1, Manager->GetNumPendingCommands()));
		ASSERT_THAT(AreEqual(0, Manager->GetPredictionQueue().NumInFlight()));
		ASSERT_THAT(IsTrue(ModelHasItem(1)));

		// The replicated state arrives (here: the same move on the inventory itself): the ack's revision is reached and the prediction ends
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Mine, Slot(0), Mine, Slot(1))));

		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
		ASSERT_THAT(IsTrue(ModelHasItem(1)));
		ASSERT_THAT(IsFalse(ModelHasItem(0)));
	}

	TEST_METHOD(AnotherPlayerChangingTheTarget_RefusesTheMoveOnTheServer_AndTheModelShowsTheirChange)
	{
		Place(Apple, 0);
		Manager->MoveItem(MoveCommand(0, 1));
		// Someone else puts a pear into slot 1 before the command arrives
		Place(Fixture.MakeDefinition("Pear"), 1);

		Manager->Server_MoveItem_Implementation(Manager->Sent[0]);

		ASSERT_THAT(IsTrue(InventoryHasItem(0)));
		ASSERT_THAT(IsTrue(InventoryHasItem(1)));
		ASSERT_THAT(IsTrue(Mine->GetItemBySlotHandle(Slot(1)).GetDefinition()->ItemId == FName("Pear")));
		ASSERT_THAT(IsTrue(Model->GetItemBySlotHandle(Slot(1)).GetDefinition()->ItemId == FName("Pear")));
		ASSERT_THAT(IsTrue(Model->GetItemBySlotHandle(Slot(0)).GetDefinition() == Apple));
		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
	}

	TEST_METHOD(AReplayedSequenceNumber_IsRefusedByTheServer)
	{
		Place(Apple, 0);
		Manager->MoveItem(MoveCommand(0, 1));
		const FRockMoveItemTransaction Sent = Manager->Sent[0];
		Manager->Server_MoveItem_Implementation(Sent);
		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Mine, Slot(1), Mine, Slot(0))));
		TestRunner->AddExpectedMessagePlain(TEXT("sequence already seen"), ELogVerbosity::Warning);

		Manager->Server_MoveItem_Implementation(Sent);

		ASSERT_THAT(IsTrue(InventoryHasItem(0)));
		ASSERT_THAT(IsFalse(InventoryHasItem(1)));
	}

	TEST_METHOD(FourMovesInFlight_HoldInput_AndAnAnswerReleasesIt)
	{
		Place(Apple, 0);
		TArray<bool> HoldEvents;
		Manager->OnInputHoldChanged.AddLambda([&HoldEvents](bool bHeld) { HoldEvents.Add(bHeld); });
		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(0, 1))));
		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(1, 2))));
		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(2, 3))));
		ASSERT_THAT(IsFalse(Manager->IsInputHeld()));
		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(3, 0))));

		ASSERT_THAT(IsTrue(Manager->IsInputHeld()));
		ASSERT_THAT(IsFalse(Manager->MoveItem(MoveCommand(0, 1))));
		ASSERT_THAT(AreEqual(4, Manager->Sent.Num()));
		ASSERT_THAT(AreEqual(1, HoldEvents.Num()));
		ASSERT_THAT(IsTrue(HoldEvents[0]));

		Manager->Client_TransactionResult_Implementation(Manager->Sent[0].TransactionID, true, {FRockInventoryRevision{Mine, static_cast<int32>(Mine->GetRevision()) + 10}});

		ASSERT_THAT(IsFalse(Manager->IsInputHeld()));
		ASSERT_THAT(AreEqual(2, HoldEvents.Num()));
		ASSERT_THAT(IsFalse(HoldEvents[1]));
	}

	TEST_METHOD(AnUnansweredMove_HoldsInputAfterTheThreshold)
	{
		Place(Apple, 0);
		TArray<bool> HoldEvents;
		Manager->OnInputHoldChanged.AddLambda([&HoldEvents](bool bHeld) { HoldEvents.Add(bHeld); });
		Manager->Now = 100.0;
		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(0, 1))));
		Manager->Now = 100.2;
		ASSERT_THAT(IsFalse(Manager->IsInputHeld()));

		Manager->Now = 100.8;

		ASSERT_THAT(IsTrue(Manager->IsInputHeld()));
		ASSERT_THAT(IsFalse(Manager->MoveItem(MoveCommand(1, 2))));
		Manager->UpdateInputHold();
		ASSERT_THAT(AreEqual(1, HoldEvents.Num()));
		ASSERT_THAT(IsTrue(HoldEvents[0]));
		// The prediction of the move already sent stays: only new moves are held
		ASSERT_THAT(IsTrue(ModelHasItem(1)));
	}

	TEST_METHOD(AMoveThatNeverGetsAnAnswer_IsAbandoned_TheModelRecovers_AndInputIsFreeAgain)
	{
		Place(Apple, 0);
		TArray<bool> HoldEvents;
		Manager->OnInputHoldChanged.AddLambda([&HoldEvents](bool bHeld) { HoldEvents.Add(bHeld); });
		Manager->Now = 100.0;
		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(0, 1))));
		Manager->Now = 101.0;
		Manager->UpdateInputHold();
		ASSERT_THAT(IsTrue(Manager->IsInputHeld()));
		ASSERT_THAT(IsFalse(Manager->MoveItem(MoveCommand(1, 2))));

		// The answer is lost: past the abandon timeout the prediction is dropped and the replicated state shows again
		TestRunner->AddExpectedMessagePlain(TEXT("were abandoned"), ELogVerbosity::Warning);
		Manager->Now = 106.0;
		Manager->UpdateInputHold();

		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
		ASSERT_THAT(IsFalse(Manager->IsInputHeld()));
		ASSERT_THAT(IsTrue(ModelHasItem(0)));
		ASSERT_THAT(IsFalse(ModelHasItem(1)));
		ASSERT_THAT(AreEqual(2, HoldEvents.Num()));
		ASSERT_THAT(IsFalse(HoldEvents[1]));
		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(0, 2))));
		ASSERT_THAT(IsTrue(ModelHasItem(2)));
	}

	TEST_METHOD(AMoveAckedButNeverReplicated_IsAbandonedToo)
	{
		Place(Apple, 0);
		Manager->Now = 100.0;
		Manager->MoveItem(MoveCommand(0, 1));
		Manager->Client_TransactionResult_Implementation(Manager->Sent[0].TransactionID, true, {FRockInventoryRevision{Mine, static_cast<int32>(Mine->GetRevision()) + 5}});
		ASSERT_THAT(AreEqual(1, Manager->GetNumPendingCommands()));
		TestRunner->AddExpectedMessagePlain(TEXT("were abandoned"), ELogVerbosity::Warning);

		Manager->Now = 106.0;
		Manager->UpdateInputHold();

		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
		ASSERT_THAT(IsTrue(ModelHasItem(0)));
	}

	TEST_METHOD(AMoveTheShownStateRefuses_IsNotSent)
	{
		Place(Apple, 0);

		// Slot 2 is empty on the shown state: nothing to move
		ASSERT_THAT(IsFalse(Manager->MoveItem(MoveCommand(2, 3))));

		ASSERT_THAT(AreEqual(0, Manager->Sent.Num()));
		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
	}

	TEST_METHOD(WithoutPrediction_TheMoveIsSentAndTheModelWaitsForTheServer)
	{
		Place(Apple, 0);
		Manager->bPredict = false;

		ASSERT_THAT(IsTrue(Manager->MoveItem(MoveCommand(0, 1))));

		ASSERT_THAT(AreEqual(1, Manager->Sent.Num()));
		ASSERT_THAT(IsTrue(Manager->Sent[0].TransactionID > 0));
		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
		ASSERT_THAT(IsTrue(ModelHasItem(0)));
		ASSERT_THAT(IsFalse(ModelHasItem(1)));
	}

	TEST_METHOD(AMoveWithAStaleExpectation_ChangesNothingOnTheServer)
	{
		Place(Apple, 0);
		Manager->bPredict = false;
		FRockMoveItemTransaction Command = MoveCommand(0, 1);
		// The client believed slot 1 held a pear
		Command.ExpectedTarget.bCheck = true;
		Command.ExpectedTarget.bEmpty = false;
		Command.ExpectedTarget.Definition = Fixture.MakeDefinition("Pear");
		Command.ExpectedTarget.Count = 1;

		Manager->Server_MoveItem_Implementation(Command);

		ASSERT_THAT(IsTrue(InventoryHasItem(0)));
		ASSERT_THAT(IsFalse(InventoryHasItem(1)));
	}
};
#endif
