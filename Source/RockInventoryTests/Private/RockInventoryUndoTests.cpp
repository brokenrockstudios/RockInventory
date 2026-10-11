// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"
#include "RockInventoryTestManager.h"

#include "Client/RockInventoryClientModel.h"
#include "Client/RockInventoryUndo.h"
#include "Components/RockInventoryManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/RockInventoryConfig.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace
{
	FRockInventorySectionInfo UndoGrid(int32 Columns, int32 Rows)
	{
		return FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, Columns, Rows, ERockItemSizePolicy::RespectSize);
	}

	FRockInventorySlotHandle UndoSlot(const FRockInventoryData& Data, int32 Column, int32 Row = 0)
	{
		const FRockInventorySectionInfo& Section = Data.Sections[0];
		return FRockInventorySlotHandle(Section.GetFirstSlotIndex() + Row * Section.GetColumns() + Column);
	}

	/** Same items in every slot (definition, count, custom values, orientation); handles may differ. What an undo promises. */
	bool UndoSameContents(const FRockInventoryData& A, const FRockInventoryData& B)
	{
		if (A.Slots.Num() != B.Slots.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Slots.Num(); ++Index)
		{
			if (!FRockSlotExpectation::Capture(A, FRockInventorySlotHandle(Index)).Matches(B, FRockInventorySlotHandle(Index)))
			{
				return false;
			}
		}
		return true;
	}

	int32 UndoCountAt(const FRockInventoryData& Data, const FRockInventorySlotHandle& Slot)
	{
		const FRockItemStack* Stack = Data.GetSlotStack(Slot);
		return Stack ? Stack->GetStackCount() : 0;
	}
}

// One move and its inverse on plain data (T-80): the inverse puts back what both slots held, for every kind of move, and the
// preconditions both ways are the ones an undo and a redo check before they are sent.
TEST_CLASS(RockInventoryUndoStepTests, "BRS.RockInventory.Undo.Step")
{
	static constexpr uint32 KeyA = 1;
	static constexpr uint32 KeyB = 2;

	TArray<TStrongObjectPtr<UObject>> KeepAlive;
	URockItemDefinition* Apple = nullptr;
	URockItemDefinition* Arrow = nullptr;

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
		Arrow = Def("Arrow", 10);
	}

	static FRockInventoryData Grid(int32 Columns = 4, int32 Rows = 1)
	{
		FRockInventoryData Data;
		Data.Init({UndoGrid(Columns, Rows)});
		return Data;
	}

	static FRockUndoMove Move(const FRockInventoryData& Layout, uint32 FromKey, FIntPoint From, uint32 ToKey, FIntPoint To, const FRockMoveItemParams& Params = FRockMoveItemParams())
	{
		FRockUndoMove Result;
		Result.SourceKey = FromKey;
		Result.SourceSlot = UndoSlot(Layout, From.X, From.Y);
		Result.TargetKey = ToKey;
		Result.TargetSlot = UndoSlot(Layout, To.X, To.Y);
		Result.Params = Params;
		return Result;
	}

	/** Runs a recorded move the way undo and redo do: its preconditions must match, then it applies. */
	static bool Replay(FRockPredictionState& State, const FRockUndoMove& Recorded)
	{
		FRockUndoStep Ignored;
		const ERockUndoStepResult Result = FRockUndoStep::Make(State, Recorded, Ignored);
		return Result != ERockUndoStepResult::Stale && Result != ERockUndoStepResult::Refused;
	}

	TEST_METHOD(AMove_ItsInverseMovesTheItemBack)
	{
		FRockInventoryData Data = Grid();
		Data.PlaceStack(FRockItemStack(Apple, 1), UndoSlot(Data, 0));
		const FRockInventoryData Before = Data;
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Undoable, FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {2, 0}), Step)));
		ASSERT_THAT(AreEqual(1, UndoCountAt(State[KeyA], UndoSlot(Data, 2))));

		ASSERT_THAT(IsTrue(Replay(State, Step.Inverse)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Before)));
	}

	TEST_METHOD(ASplit_ItsInverseMergesThePartBack)
	{
		FRockInventoryData Data = Grid();
		Data.PlaceStack(FRockItemStack(Arrow, 5), UndoSlot(Data, 0));
		const FRockInventoryData Before = Data;
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockMoveItemParams Half;
		Half.MoveMode = ERockItemMoveMode::HalfStack;
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Undoable, FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {2, 0}, Half), Step)));
		ASSERT_THAT(AreEqual(2, UndoCountAt(State[KeyA], UndoSlot(Data, 0))));
		ASSERT_THAT(AreEqual(3, Step.Inverse.Params.MoveCount));

		ASSERT_THAT(IsTrue(Replay(State, Step.Inverse)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Before)));
	}

	TEST_METHOD(AWholeStackMerge_ItsInverseSplitsTheUnitsBackIntoTheEmptySlot)
	{
		FRockInventoryData Data = Grid();
		Data.PlaceStack(FRockItemStack(Arrow, 3), UndoSlot(Data, 0));
		Data.PlaceStack(FRockItemStack(Arrow, 4), UndoSlot(Data, 1));
		const FRockInventoryData Before = Data;
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Undoable, FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {1, 0}), Step)));
		ASSERT_THAT(AreEqual(0, UndoCountAt(State[KeyA], UndoSlot(Data, 0))));
		ASSERT_THAT(AreEqual(7, UndoCountAt(State[KeyA], UndoSlot(Data, 1))));

		ASSERT_THAT(IsTrue(Replay(State, Step.Inverse)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Before)));
	}

	TEST_METHOD(APartialMerge_ItsInverseTakesBackOnlyWhatMoved)
	{
		FRockInventoryData Data = Grid();
		Data.PlaceStack(FRockItemStack(Arrow, 6), UndoSlot(Data, 0));
		Data.PlaceStack(FRockItemStack(Arrow, 8), UndoSlot(Data, 1));
		const FRockInventoryData Before = Data;
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Undoable, FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {1, 0}), Step)));
		ASSERT_THAT(AreEqual(4, UndoCountAt(State[KeyA], UndoSlot(Data, 0))));
		ASSERT_THAT(AreEqual(10, UndoCountAt(State[KeyA], UndoSlot(Data, 1))));
		ASSERT_THAT(AreEqual(2, Step.Inverse.Params.MoveCount));

		ASSERT_THAT(IsTrue(Replay(State, Step.Inverse)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Before)));
	}

	TEST_METHOD(ARotationInPlace_ItsInverseRotatesBack)
	{
		FRockInventoryData Data = Grid(3, 3);
		Data.PlaceStack(FRockItemStack(Def("Plank", 1, FIntPoint(2, 1)), 1), UndoSlot(Data, 0));
		const FRockInventoryData Before = Data;
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockMoveItemParams Rotate;
		Rotate.DesiredOrientation = ERockItemOrientation::Vertical;
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Undoable, FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {0, 0}, Rotate), Step)));
		ASSERT_THAT(AreEqual(ERockItemOrientation::Vertical, State[KeyA].GetSlot(UndoSlot(Data, 0))->Orientation));

		ASSERT_THAT(IsTrue(Replay(State, Step.Inverse)));
		ASSERT_THAT(AreEqual(ERockItemOrientation::Horizontal, State[KeyA].GetSlot(UndoSlot(Data, 0))->Orientation));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Before)));
	}

	TEST_METHOD(AMoveThatAlsoRotates_ItsInverseRestoresTheOldOrientation)
	{
		FRockInventoryData Data = Grid(3, 3);
		Data.PlaceStack(FRockItemStack(Def("Plank", 1, FIntPoint(2, 1)), 1), UndoSlot(Data, 0));
		const FRockInventoryData Before = Data;
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockMoveItemParams Rotate;
		Rotate.DesiredOrientation = ERockItemOrientation::Vertical;
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Undoable, FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {2, 0}, Rotate), Step)));
		ASSERT_THAT(AreEqual(ERockItemOrientation::Horizontal, Step.Inverse.Params.DesiredOrientation));

		ASSERT_THAT(IsTrue(Replay(State, Step.Inverse)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Before)));
	}

	TEST_METHOD(AMoveBetweenTwoInventories_ItsInverseMovesTheItemBack)
	{
		FRockInventoryData Mine = Grid();
		Mine.PlaceStack(FRockItemStack(Arrow, 4), UndoSlot(Mine, 1));
		const FRockInventoryData Chest = Grid();
		FRockPredictionState State;
		State.Add(KeyA, Mine);
		State.Add(KeyB, Chest);
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Undoable, FRockUndoStep::Make(State, Move(Mine, KeyA, {1, 0}, KeyB, {3, 0}), Step)));
		ASSERT_THAT(AreEqual(4, UndoCountAt(State[KeyB], UndoSlot(Chest, 3))));

		ASSERT_THAT(IsTrue(Replay(State, Step.Inverse)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Mine)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyB], Chest)));
	}

	TEST_METHOD(ARedo_FindsTheForwardsPreconditionsInTheStateTheInverseRestored)
	{
		FRockInventoryData Data = Grid();
		Data.PlaceStack(FRockItemStack(Arrow, 6), UndoSlot(Data, 0));
		Data.PlaceStack(FRockItemStack(Arrow, 8), UndoSlot(Data, 1));
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockUndoStep Step;
		FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {1, 0}), Step);
		const FRockInventoryData After = State[KeyA];
		Replay(State, Step.Inverse);

		ASSERT_THAT(IsTrue(Replay(State, Step.Forward)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], After)));
	}

	TEST_METHOD(AMoveThatChangesNothing_IsNoChange)
	{
		FRockInventoryData Data = Grid();
		Data.PlaceStack(FRockItemStack(Apple, 1), UndoSlot(Data, 0));
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::NoChange, FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {0, 0}), Step)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Data)));
	}

	TEST_METHOD(AMoveWhoseSlotNoLongerHoldsWhatItExpects_IsStale_AndChangesNothing)
	{
		FRockInventoryData Data = Grid();
		Data.PlaceStack(FRockItemStack(Apple, 1), UndoSlot(Data, 0));
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockUndoMove Recorded = Move(Data, KeyA, {0, 0}, KeyA, {2, 0});
		// Recorded when slot 0 held a pear
		FRockInventoryData Earlier = Grid();
		Earlier.PlaceStack(FRockItemStack(Def("Pear"), 1), UndoSlot(Earlier, 0));
		Recorded.ExpectedSource = FRockSlotExpectation::Capture(Earlier, UndoSlot(Earlier, 0));
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Stale, FRockUndoStep::Make(State, Recorded, Step)));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Data)));
	}

	TEST_METHOD(AMoveTheDataRefuses_IsRefused_AndChangesNothing)
	{
		FRockInventoryData Data = Grid();
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockUndoStep Step;
		ERockMoveRefusal Refusal = ERockMoveRefusal::None;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::Refused, FRockUndoStep::Make(State, Move(Data, KeyA, {1, 0}, KeyA, {2, 0}), Step, &Refusal)));
		ASSERT_THAT(AreEqual(ERockMoveRefusal::EmptySource, Refusal));
		ASSERT_THAT(IsTrue(UndoSameContents(State[KeyA], Data)));
	}

	TEST_METHOD(AWholeStackMergeOfAnInstancedItem_IsNotInvertible)
	{
		// Its inverse would split a part off an instanced stack, which the data refuses (PartialMoveOfInstancedItem)
		URockItemDefinition* Charm = Def("Charm", 10);
		Charm->RuntimeInstanceClass = TSoftClassPtr<URockItemInstance>(URockItemInstance::StaticClass());
		FRockInventoryData Data = Grid();
		Data.PlaceStack(FRockItemStack(Charm, 2), UndoSlot(Data, 0));
		Data.PlaceStack(FRockItemStack(Charm, 3), UndoSlot(Data, 1));
		FRockPredictionState State;
		State.Add(KeyA, Data);
		FRockUndoStep Step;

		ASSERT_THAT(AreEqual(ERockUndoStepResult::NotInvertible, FRockUndoStep::Make(State, Move(Data, KeyA, {0, 0}, KeyA, {1, 0}), Step)));
		ASSERT_THAT(AreEqual(5, UndoCountAt(State[KeyA], UndoSlot(Data, 1))));
	}
};

// The two stacks of the history (T-80): recording, the cap, undo and redo moving entries, severing per container, refusals.
TEST_CLASS(RockInventoryUndoHistoryTests, "BRS.RockInventory.Undo.History")
{
	static constexpr uint32 Own = 1;
	static constexpr uint32 ChestKey = 2;

	FRockUndoHistory History;

	/** An entry tagged with Id (in the forward move's count) that moves within or between the given inventories. */
	static FRockUndoEntry Entry(int32 Id, uint32 SourceKey = Own, uint32 TargetKey = Own, int32 Sequence = 0)
	{
		FRockUndoEntry Result;
		FRockUndoStep& Step = Result.Steps.AddDefaulted_GetRef();
		Step.Forward.SourceKey = SourceKey;
		Step.Forward.TargetKey = TargetKey;
		Step.Forward.Params.MoveCount = Id;
		Step.Inverse.SourceKey = TargetKey;
		Step.Inverse.TargetKey = SourceKey;
		if (Sequence != 0)
		{
			Result.PendingSequences.Add(Sequence);
		}
		return Result;
	}

	static TArray<int32> Ids(const TArray<FRockUndoEntry>& Stack)
	{
		TArray<int32> Result;
		for (const FRockUndoEntry& Each : Stack)
		{
			Result.Add(Each.Steps[0].Forward.Params.MoveCount);
		}
		return Result;
	}

	TEST_METHOD(Record_PushesTheEntry_AndClearsRedo)
	{
		History.Record(Entry(1));
		History.Record(Entry(2));
		History.MarkUndone({});
		ASSERT_THAT(IsTrue(History.CanRedo()));

		History.Record(Entry(3));

		ASSERT_THAT(IsFalse(History.CanRedo()));
		ASSERT_THAT(IsTrue(Ids(History.GetUndoEntries()) == TArray<int32>{1, 3}));
	}

	TEST_METHOD(AnEntryWithoutSteps_IsNotRecorded)
	{
		History.Record(FRockUndoEntry());

		ASSERT_THAT(IsFalse(History.CanUndo()));
	}

	TEST_METHOD(TheCap_KeepsTheNewestEntries)
	{
		History.MaxEntries = 3;
		for (int32 Id = 1; Id <= 5; ++Id)
		{
			History.Record(Entry(Id));
		}

		ASSERT_THAT(IsTrue(Ids(History.GetUndoEntries()) == TArray<int32>{3, 4, 5}));
	}

	TEST_METHOD(ACapOfZero_KeepsNothing)
	{
		History.MaxEntries = 0;
		History.Record(Entry(1));

		ASSERT_THAT(IsFalse(History.CanUndo()));
	}

	TEST_METHOD(UndoAndRedo_MoveTheTopEntryBetweenTheStacks)
	{
		History.Record(Entry(1));
		History.Record(Entry(2));

		History.MarkUndone({});
		ASSERT_THAT(IsTrue(Ids(History.GetUndoEntries()) == TArray<int32>{1}));
		ASSERT_THAT(AreEqual(2, History.PeekRedo()->Steps[0].Forward.Params.MoveCount));
		History.MarkUndone({});
		ASSERT_THAT(AreEqual(1, History.PeekRedo()->Steps[0].Forward.Params.MoveCount));

		History.MarkRedone({});
		ASSERT_THAT(IsTrue(Ids(History.GetUndoEntries()) == TArray<int32>{1}));
		ASSERT_THAT(IsTrue(Ids(History.GetRedoEntries()) == TArray<int32>{2}));
	}

	TEST_METHOD(ClosingAContainer_CutsAtTheNewestEntryTouchingIt_AndKeepsNewerOnesOnOpenInventories)
	{
		History.Record(Entry(1));
		History.Record(Entry(2, Own, ChestKey));
		History.Record(Entry(3));
		History.Record(Entry(4, ChestKey, ChestKey));
		History.Record(Entry(5));

		const int32 Dropped = History.Sever(ChestKey);

		ASSERT_THAT(AreEqual(4, Dropped));
		ASSERT_THAT(IsTrue(Ids(History.GetUndoEntries()) == TArray<int32>{5}));
	}

	TEST_METHOD(ClosingAContainer_CutsRedoFromTheNextEntryTouchingIt)
	{
		History.Record(Entry(1));
		History.Record(Entry(2, ChestKey, Own));
		History.Record(Entry(3));
		History.MarkUndone({});
		History.MarkUndone({});
		History.MarkUndone({});
		// Redo would replay 1, then 2 (the chest), then 3

		History.Sever(ChestKey);

		ASSERT_THAT(IsTrue(Ids(History.GetRedoEntries()) == TArray<int32>{1}));
	}

	TEST_METHOD(ClosingAnInventoryNoEntryTouches_ChangesNothing)
	{
		History.Record(Entry(1));
		History.Record(Entry(2));

		ASSERT_THAT(AreEqual(0, History.Sever(ChestKey)));
		ASSERT_THAT(IsTrue(Ids(History.GetUndoEntries()) == TArray<int32>{1, 2}));
	}

	TEST_METHOD(ARefusedCommand_DropsOnlyItsEntry)
	{
		History.Record(Entry(1, Own, Own, 11));
		History.Record(Entry(2, Own, Own, 12));
		History.Record(Entry(3, Own, Own, 13));

		ASSERT_THAT(IsTrue(History.DropBySequence(12)));
		ASSERT_THAT(IsTrue(Ids(History.GetUndoEntries()) == TArray<int32>{1, 3}));
	}

	TEST_METHOD(AConfirmedCommand_IsNoLongerWaitedFor)
	{
		History.Record(Entry(1, Own, Own, 11));

		History.Confirm(11);

		ASSERT_THAT(IsFalse(History.DropBySequence(11)));
		ASSERT_THAT(IsTrue(History.CanUndo()));
	}

	TEST_METHOD(AnUndoneEntry_StillWaitsForItsOriginalCommand)
	{
		// Undone before the server answered the move: a refusal of the move drops the entry from redo too
		History.Record(Entry(1, Own, Own, 11));
		History.MarkUndone({12});

		ASSERT_THAT(IsTrue(History.DropBySequence(11)));
		ASSERT_THAT(IsTrue(History.IsEmpty()));
	}

	TEST_METHOD(Keys_ListEachInventoryOnce)
	{
		History.Record(Entry(1, Own, ChestKey));
		History.Record(Entry(2));
		History.MarkUndone({});
		TArray<uint32> Keys;

		History.GetKeys(Keys);

		ASSERT_THAT(AreEqual(2, Keys.Num()));
		ASSERT_THAT(IsTrue(Keys.Contains(Own) && Keys.Contains(ChestKey)));
	}
};

// The manager component end to end (T-80): a real inventory, the client model and the test manager, which holds every command so
// the test plays the server (Server_MoveItem_Implementation) and other players (moves on the inventory itself).
TEST_CLASS(RockInventoryUndoManagerTests, "BRS.RockInventory.Undo.Manager")
{
	FRockInventoryFixture Fixture;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;

	APawn* Pawn = nullptr;
	APlayerController* Controller = nullptr;
	URockInventoryTestManager* Manager = nullptr;
	URockInventory* Mine = nullptr;
	URockInventoryClientModel* Model = nullptr;
	URockItemDefinition* Apple = nullptr;
	URockItemDefinition* Pear = nullptr;
	int32 DroppedNotices = 0;

	URockInventory* MakeInventory(AActor* InOwner)
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {UndoGrid(4, 1)};
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
		Pear = Fixture.MakeDefinition("Pear");
		Model = URockInventoryClientModelSubsystem::GetModel(Mine);
		ASSERT_THAT(IsNotNull(Model));
		Manager->BindPredictionToModels();
		Manager->OnUndoEntryDropped.AddLambda([this]() { ++DroppedNotices; });
	}

	static FRockInventorySlotHandle SlotOf(const URockInventory* Inventory, int32 Column)
	{
		return FRockInventorySlotHandle(Inventory->GetSectionInfo(RockInventoryTags::Inventory_Section_Backpack).GetFirstSlotIndex() + Column);
	}

	FRockInventorySlotHandle Slot(int32 Column) const { return SlotOf(Mine, Column); }

	void Place(URockItemDefinition* Definition, int32 Column, URockInventory* Inventory = nullptr)
	{
		Inventory = Inventory ? Inventory : Mine;
		const FRockItemStackHandle Handle = Inventory->AddItemToInventory(FRockItemStack(Definition, 1));
		FRockInventorySlotEntry Entry = Inventory->GetSlotByHandle(SlotOf(Inventory, Column));
		Entry.ItemHandle = Handle;
		Inventory->SetSlotByHandle(SlotOf(Inventory, Column), Entry);
	}

	bool Move(int32 From, int32 To)
	{
		return Manager->MoveItem(FRockMoveItemTransaction(Controller, Mine, Slot(From), Mine, Slot(To)));
	}

	/** The server runs the held command with this index, as the RPC would. */
	void ServerRuns(int32 Index)
	{
		Manager->Server_MoveItem_Implementation(Manager->Sent[Index]);
	}

	/** Another player (or the server) changes the inventory directly. */
	void SomeoneElseMoves(int32 From, int32 To)
	{
		URockInventoryLibrary::MoveItem(Mine, Slot(From), Mine, Slot(To));
	}

	bool ModelHas(int32 Column) const { return Model->GetSlotByHandle(Slot(Column)).ItemHandle.IsValid(); }
	bool InventoryHas(int32 Column) const { return Mine->GetSlotByHandle(Slot(Column)).ItemHandle.IsValid(); }
	int32 NumUndo() const { return Manager->GetUndoHistory().GetUndoEntries().Num(); }

	TEST_METHOD(AMove_UndoAndRedo_RoundTripOnTheModelAndOnTheServer)
	{
		Place(Apple, 0);
		ASSERT_THAT(IsTrue(Move(0, 1)));
		ServerRuns(0);
		ASSERT_THAT(IsTrue(InventoryHas(1)));
		ASSERT_THAT(IsTrue(Manager->CanUndo()));

		ASSERT_THAT(IsTrue(Manager->Undo()));
		// Shown at once, the server has not seen it yet
		ASSERT_THAT(IsTrue(ModelHas(0)));
		ASSERT_THAT(IsFalse(ModelHas(1)));
		ASSERT_THAT(IsTrue(InventoryHas(1)));
		ASSERT_THAT(AreEqual(2, Manager->Sent.Num()));
		ServerRuns(1);
		ASSERT_THAT(IsTrue(InventoryHas(0)));
		ASSERT_THAT(IsFalse(InventoryHas(1)));
		ASSERT_THAT(IsFalse(Manager->CanUndo()));
		ASSERT_THAT(IsTrue(Manager->CanRedo()));

		ASSERT_THAT(IsTrue(Manager->Redo()));
		ASSERT_THAT(IsTrue(ModelHas(1)));
		ServerRuns(2);
		ASSERT_THAT(IsTrue(InventoryHas(1)));
		ASSERT_THAT(IsFalse(InventoryHas(0)));
		ASSERT_THAT(IsTrue(Manager->CanUndo()));
		ASSERT_THAT(IsFalse(Manager->CanRedo()));
		ASSERT_THAT(AreEqual(0, Manager->GetNumPendingCommands()));
		ASSERT_THAT(AreEqual(0, DroppedNotices));
	}

	TEST_METHOD(WithoutPrediction_UndoAndRedo_RoundTripOnTheServer)
	{
		Manager->bPredict = false;
		Place(Apple, 0);
		ASSERT_THAT(IsTrue(Move(0, 1)));
		ServerRuns(0);

		ASSERT_THAT(IsTrue(Manager->Undo()));
		ASSERT_THAT(IsTrue(Manager->Sent[1].ExpectedSource.bCheck));
		ServerRuns(1);
		ASSERT_THAT(IsTrue(InventoryHas(0)));
		ASSERT_THAT(IsFalse(InventoryHas(1)));

		ASSERT_THAT(IsTrue(Manager->Redo()));
		ServerRuns(2);
		ASSERT_THAT(IsTrue(InventoryHas(1)));
		ASSERT_THAT(IsFalse(InventoryHas(0)));
	}

	TEST_METHOD(AnUndoAfterSomeoneElseMovedTheItem_FailsAndDropsTheEntry)
	{
		Place(Apple, 0);
		Move(0, 1);
		ServerRuns(0);
		SomeoneElseMoves(1, 3);

		ASSERT_THAT(IsFalse(Manager->Undo()));

		ASSERT_THAT(AreEqual(1, Manager->Sent.Num()));
		ASSERT_THAT(IsFalse(Manager->CanUndo()));
		ASSERT_THAT(IsFalse(Manager->CanRedo()));
		ASSERT_THAT(AreEqual(1, DroppedNotices));
		ASSERT_THAT(IsTrue(InventoryHas(3)));
		ASSERT_THAT(IsTrue(ModelHas(3)));
	}

	TEST_METHOD(AnUndoTheServerRefuses_DropsTheEntry_AndTheModelShowsTheServersState)
	{
		Place(Apple, 0);
		Move(0, 1);
		ServerRuns(0);
		ASSERT_THAT(IsTrue(Manager->Undo()));
		ASSERT_THAT(IsTrue(ModelHas(0)));
		// Someone else moves the apple before the server sees the undo
		SomeoneElseMoves(1, 2);

		ServerRuns(1);

		ASSERT_THAT(IsFalse(Manager->CanUndo()));
		ASSERT_THAT(IsFalse(Manager->CanRedo()));
		ASSERT_THAT(AreEqual(1, DroppedNotices));
		ASSERT_THAT(IsTrue(InventoryHas(2)));
		ASSERT_THAT(IsTrue(ModelHas(2)));
		ASSERT_THAT(IsFalse(ModelHas(0)));
	}

	TEST_METHOD(ARefusedMove_DropsItsEntry_WithoutTheUndoNotice)
	{
		Place(Apple, 0);
		Move(0, 1);
		// Someone else fills the target first
		Place(Pear, 1);

		ServerRuns(0);

		ASSERT_THAT(IsFalse(Manager->CanUndo()));
		ASSERT_THAT(AreEqual(0, DroppedNotices));
	}

	TEST_METHOD(ADrop_IsABarrier_UndoStopsThere)
	{
		Place(Apple, 0);
		Place(Pear, 2);
		Move(0, 1);
		ServerRuns(0);
		ASSERT_THAT(IsTrue(Manager->CanUndo()));

		Manager->DropItem(FRockDropItemTransaction(Controller, Mine, Slot(2)));

		ASSERT_THAT(AreEqual(1, Manager->SentDrops.Num()));
		ASSERT_THAT(IsFalse(Manager->CanUndo()));
		ASSERT_THAT(IsFalse(Manager->Undo()));
		ASSERT_THAT(AreEqual(1, Manager->Sent.Num()));
	}

	TEST_METHOD(ADrop_ClearsRedo)
	{
		Place(Apple, 0);
		Place(Pear, 2);
		Move(0, 1);
		ServerRuns(0);
		Manager->Undo();
		ServerRuns(1);
		ASSERT_THAT(IsTrue(Manager->CanRedo()));

		Manager->DropItem(FRockDropItemTransaction(Controller, Mine, Slot(2)));

		ASSERT_THAT(IsFalse(Manager->CanRedo()));
	}

	TEST_METHOD(ANewMove_ClearsRedo)
	{
		Place(Apple, 0);
		Move(0, 1);
		ServerRuns(0);
		Manager->Undo();
		ServerRuns(1);
		ASSERT_THAT(IsTrue(Manager->CanRedo()));

		Move(0, 2);

		ASSERT_THAT(IsFalse(Manager->CanRedo()));
		ASSERT_THAT(AreEqual(1, NumUndo()));
	}

	TEST_METHOD(AMoveThatChangesNothing_IsNotRecorded)
	{
		Place(Apple, 0);

		ASSERT_THAT(IsTrue(Move(0, 0)));

		ASSERT_THAT(IsFalse(Manager->CanUndo()));
	}

	TEST_METHOD(ABatch_IsOneEntry_UndoneAndRedoneTogether)
	{
		Place(Apple, 0);
		Place(Pear, 1);
		Manager->BeginUndoBatch();
		Move(0, 2);
		Move(1, 3);
		ASSERT_THAT(IsFalse(Manager->CanUndo()));
		Manager->EndUndoBatch();
		ServerRuns(0);
		ServerRuns(1);
		ASSERT_THAT(AreEqual(1, NumUndo()));
		ASSERT_THAT(AreEqual(2, Manager->GetUndoHistory().GetUndoEntries()[0].Steps.Num()));

		ASSERT_THAT(IsTrue(Manager->Undo()));
		ASSERT_THAT(IsTrue(ModelHas(0) && ModelHas(1)));
		ASSERT_THAT(IsFalse(ModelHas(2) || ModelHas(3)));
		ASSERT_THAT(AreEqual(4, Manager->Sent.Num()));
		ServerRuns(2);
		ServerRuns(3);
		ASSERT_THAT(IsTrue(InventoryHas(0) && InventoryHas(1)));

		ASSERT_THAT(IsTrue(Manager->Redo()));
		ServerRuns(4);
		ServerRuns(5);
		ASSERT_THAT(IsTrue(InventoryHas(2) && InventoryHas(3)));
		ASSERT_THAT(IsFalse(InventoryHas(0) || InventoryHas(1)));
	}

	TEST_METHOD(ABatchPartOfWhichTheServerRefused_IsNotRecorded)
	{
		Place(Apple, 0);
		Place(Pear, 1);
		Manager->BeginUndoBatch();
		Move(0, 2);
		Move(1, 3);
		Place(Fixture.MakeDefinition("Plum"), 3);
		ServerRuns(0);
		ServerRuns(1);

		Manager->EndUndoBatch();

		ASSERT_THAT(IsFalse(Manager->CanUndo()));
	}

	TEST_METHOD(UndoAndRedo_WaitForAnOpenBatchToEnd)
	{
		Place(Apple, 0);
		Move(0, 1);
		Manager->BeginUndoBatch();

		ASSERT_THAT(IsFalse(Manager->Undo()));
		Manager->EndUndoBatch();
		ASSERT_THAT(IsTrue(Manager->Undo()));
	}

	TEST_METHOD(AnUndoWhileInputIsHeld_KeepsTheEntry)
	{
		Place(Apple, 0);
		Move(0, 1);
		Move(1, 2);
		Move(2, 3);
		Move(3, 0);
		ASSERT_THAT(IsTrue(Manager->IsInputHeld()));

		ASSERT_THAT(IsFalse(Manager->Undo()));

		ASSERT_THAT(AreEqual(4, NumUndo()));
		ASSERT_THAT(AreEqual(0, DroppedNotices));
	}

	TEST_METHOD(ClosingAContainer_SeversItsHistory_ButAMoveInsideMyInventoryAfterItStays)
	{
		AActor& ChestActor = Fixture.Spawner.SpawnActor<AActor>();
		URockInventory* Chest = MakeInventory(&ChestActor);
		Manager->AddInventoryView(Mine);
		Manager->AddInventoryView(Chest);
		Place(Apple, 0);
		Place(Pear, 1);
		Move(0, 2);
		ASSERT_THAT(IsTrue(Manager->MoveItem(FRockMoveItemTransaction(Controller, Mine, Slot(1), Chest, SlotOf(Chest, 0)))));
		Move(2, 3);
		ASSERT_THAT(AreEqual(3, NumUndo()));

		Manager->RemoveInventoryView(Chest);

		ASSERT_THAT(IsTrue(Manager->IsInventoryScreenOpen()));
		ASSERT_THAT(AreEqual(1, NumUndo()));
		ASSERT_THAT(IsTrue(Manager->Undo()));
		ASSERT_THAT(IsTrue(ModelHas(2)));
		ASSERT_THAT(IsFalse(ModelHas(3)));
		ASSERT_THAT(IsFalse(Manager->CanUndo()));
	}

	TEST_METHOD(ClosingTheInventoryScreen_ClearsTheHistory)
	{
		// Two widgets show my inventory (two sections)
		Manager->AddInventoryView(Mine);
		Manager->AddInventoryView(Mine);
		Place(Apple, 0);
		Move(0, 1);

		Manager->RemoveInventoryView(Mine);
		ASSERT_THAT(IsTrue(Manager->IsInventoryScreenOpen()));
		ASSERT_THAT(IsTrue(Manager->CanUndo()));

		Manager->RemoveInventoryView(Mine);
		ASSERT_THAT(IsFalse(Manager->IsInventoryScreenOpen()));
		ASSERT_THAT(IsFalse(Manager->CanUndo()));
	}

	TEST_METHOD(TheHistoryChangedEvent_FiresOnRecordUndoAndClear)
	{
		int32 Changes = 0;
		Manager->OnUndoHistoryChanged.AddLambda([&Changes]() { ++Changes; });
		Place(Apple, 0);

		Move(0, 1);
		ASSERT_THAT(AreEqual(1, Changes));
		Manager->Undo();
		ASSERT_THAT(AreEqual(2, Changes));
		Manager->ClearUndoHistory();
		ASSERT_THAT(AreEqual(3, Changes));
	}
};
#endif
