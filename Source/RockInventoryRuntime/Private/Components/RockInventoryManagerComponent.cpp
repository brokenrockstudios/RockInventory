// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Components/RockInventoryManagerComponent.h"

#include "Access/RockInventoryAccessSubsystem.h"
#include "Client/RockInventoryClientModel.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "RockInventoryLogging.h"
#include "HAL/PlatformTime.h"
#include "Inventory/RockInventory.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "Net/UnrealNetwork.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "Transactions/Implementations/RockMoveItemTransaction.h"

using RockInventoryManager::EMoveOrigin;
using RockInventoryManager::ESendResult;

URockInventoryManagerComponent::URockInventoryManagerComponent(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	// Ticks only while commands wait for the server (the pending threshold)
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void URockInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(URockInventoryManagerComponent, Observed, COND_OwnerOnly);
}

URockInventoryManagerComponent* URockInventoryManagerComponent::FindFor(const AController* Controller)
{
	if (!Controller)
	{
		return nullptr;
	}
	if (URockInventoryManagerComponent* Found = Controller->FindComponentByClass<URockInventoryManagerComponent>())
	{
		return Found;
	}
	if (const APlayerState* PlayerState = Controller->PlayerState)
	{
		if (URockInventoryManagerComponent* Found = PlayerState->FindComponentByClass<URockInventoryManagerComponent>())
		{
			return Found;
		}
	}
	if (const APawn* Pawn = Controller->GetPawn())
	{
		return Pawn->FindComponentByClass<URockInventoryManagerComponent>();
	}
	return nullptr;
}

URockInventoryManagerComponent* URockInventoryManagerComponent::FindLocal(const UWorld* World)
{
	return GEngine ? FindFor(GEngine->GetFirstLocalPlayerController(World)) : nullptr;
}

void URockInventoryManagerComponent::ServerSetObserved(URockInventory* Inventory, bool bObserved)
{
	if (!Inventory || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const int32 Index = Observed.IndexOfByPredicate([Inventory](const FRockObservedInventory& Entry) { return Entry.Inventory == Inventory; });
	if (bObserved)
	{
		FRockObservedInventory& Entry = Index == INDEX_NONE ? Observed.AddDefaulted_GetRef() : Observed[Index];
		Entry.Inventory = Inventory;
		Entry.Revision = Inventory->GetRevision();
	}
	else if (Index != INDEX_NONE)
	{
		Observed.RemoveAtSwap(Index);
	}
	else
	{
		return;
	}
	// A listen server's own client part: no replication happens, so the OnRep does not run
	if (GetNetMode() != NM_DedicatedServer)
	{
		Inventory->RefreshSyncStateWithFollowers();
	}
}

const FRockObservedInventory* URockInventoryManagerComponent::FindObserved(const URockInventory* Inventory) const
{
	return Observed.FindByPredicate([Inventory](const FRockObservedInventory& Entry) { return Entry.Inventory == Inventory; });
}

void URockInventoryManagerComponent::OnRep_Observed(const TArray<FRockObservedInventory>& OldObserved)
{
	// Entries that left (closed, out of reach) turn Stale; new ones turn Syncing. An entry whose inventory has not arrived yet is
	// picked up by the inventory's own PostNetReceive.
	for (const FRockObservedInventory& Old : OldObserved)
	{
		if (Old.Inventory && !FindObserved(Old.Inventory))
		{
			Old.Inventory->RefreshSyncStateWithFollowers();
			// The server closed it (closed, out of reach): undo cannot reach into it any more
			SeverUndoHistory(Old.Inventory);
		}
	}
	for (const FRockObservedInventory& Entry : Observed)
	{
		if (Entry.Inventory)
		{
			Entry.Inventory->RefreshSyncStateWithFollowers();
		}
	}
}

bool URockInventoryManagerComponent::CanAccess(const URockInventory* Inventory, const AController* Instigator, ERockInventoryRights Required) const
{
	const URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(this);
	return Access && Access->CanAccess(Instigator, Inventory, Required);
}

void URockInventoryManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindPredictionFromModels();
	InventoryViews.Reset();
	ClearUndoHistory();
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(this))
		{
			Access->CloseAll(GetOwningController());
		}
	}
	Super::EndPlay(EndPlayReason);
}

void URockInventoryManagerComponent::Server_OpenInventory_Implementation(URockInventory* Inventory)
{
	URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(this);
	AController* Controller = GetOwningController();
	if (!Access || !Controller || GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_OpenInventory - no access registry, owning controller or authority, open refused"));
		return;
	}
	const ERockOpenResult Result = Access->Open(Controller, Inventory);
	if (Result != ERockOpenResult::Opened && Result != ERockOpenResult::AlreadyOpen)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_OpenInventory - %s may not open inventory %s (result %d)"), *GetNameSafe(Controller), *GetNameSafe(Inventory), static_cast<int32>(Result));
	}
}

void URockInventoryManagerComponent::Server_CloseInventory_Implementation(URockInventory* Inventory)
{
	if (URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(this))
	{
		Access->Close(GetOwningController(), Inventory);
	}
}

AController* URockInventoryManagerComponent::GetOwningController() const
{
	AActor* Owner = GetOwner();
	if (AController* Controller = Cast<AController>(Owner))
	{
		return Controller;
	}
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		return Pawn->GetController();
	}
	if (const APlayerState* PlayerState = Cast<APlayerState>(Owner))
	{
		return PlayerState->GetOwningController();
	}
	return nullptr;
}

bool URockInventoryManagerComponent::AuthorizeServerCommand(FRockItemTransactionBase& Command, TConstArrayView<const URockInventory*> Inventories) const
{
	AController* Controller = GetOwningController();
	if (!Controller)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("AuthorizeServerCommand - %s has no owning controller, command refused"), *GetNameSafe(GetOwner()));
		return false;
	}
	// Never trust the instigator the client sent: it decides slot-lock ownership and where a drop spawns.
	Command.Instigator = Controller;
	for (const URockInventory* Inventory : Inventories)
	{
		if (!Inventory)
		{
			UE_LOG(LogRockInventory, Warning, TEXT("AuthorizeServerCommand - null inventory from %s, command refused"), *GetNameSafe(Controller));
			return false;
		}
		if (!CanAccess(Inventory, Controller))
		{
			UE_LOG(LogRockInventory, Warning, TEXT("AuthorizeServerCommand - %s may not access inventory %s, command refused"), *GetNameSafe(Controller), *GetNameSafe(Inventory));
			return false;
		}
	}
	return true;
}

// Prediction -----------------------------------------------------------------------------------------------------------------

bool URockInventoryManagerComponent::ShouldPredict() const
{
	return GetOwnerRole() != ROLE_Authority && GetDefault<URockInventoryDeveloperSettings>()->bPredictMoves;
}

double URockInventoryManagerComponent::GetPredictionTime() const
{
	return FPlatformTime::Seconds();
}

void URockInventoryManagerComponent::SendMove(const FRockMoveItemTransaction& Command)
{
	Server_MoveItem(Command);
}

void URockInventoryManagerComponent::SendDrop(const FRockDropItemTransaction& Command)
{
	Server_DropItem(Command);
}

void URockInventoryManagerComponent::SendLoot(const FRockLootWorldItemTransaction& Command)
{
	Server_LootWorldItem(Command);
}

void URockInventoryManagerComponent::ConfigureQueue()
{
	const URockInventoryDeveloperSettings* Settings = GetDefault<URockInventoryDeveloperSettings>();
	PredictionQueue.MaxInFlight = FMath::Max(1, Settings->PredictionMaxInFlight);
	PredictionQueue.PendingThresholdSeconds = Settings->PredictionPendingThresholdSeconds;
	PredictionQueue.AbandonSeconds = Settings->PredictionAbandonSeconds;
}

void URockInventoryManagerComponent::BindPredictionToModels()
{
	if (const UWorld* World = GetWorld())
	{
		if (URockInventoryClientModelSubsystem* Models = World->GetSubsystem<URockInventoryClientModelSubsystem>())
		{
			Models->SetPredictionSource(this);
			bBoundToModels = true;
		}
	}
}

void URockInventoryManagerComponent::UnbindPredictionFromModels()
{
	if (!bBoundToModels)
	{
		return;
	}
	bBoundToModels = false;
	TArray<uint32> Keys;
	PredictionQueue.GetTouchedKeys(Keys);
	if (const UWorld* World = GetWorld())
	{
		if (URockInventoryClientModelSubsystem* Models = World->GetSubsystem<URockInventoryClientModelSubsystem>())
		{
			if (Models->GetPredictionSource() == this)
			{
				Models->SetPredictionSource(nullptr);
				// Show what the server has again
				PredictionQueue.Reset();
				RefreshModels(Keys);
			}
		}
	}
	PredictionQueue.Reset();
	PredictionInventories.Reset();
}

URockInventory* URockInventoryManagerComponent::FindInventory(uint32 Key) const
{
	const TWeakObjectPtr<URockInventory>* Found = PredictionInventories.Find(Key);
	return Found ? Found->Get() : nullptr;
}

void URockInventoryManagerComponent::RefreshModels(TConstArrayView<uint32> Keys, uint32 ExcludeKey)
{
	const UWorld* World = GetWorld();
	const URockInventoryClientModelSubsystem* Models = World ? World->GetSubsystem<URockInventoryClientModelSubsystem>() : nullptr;
	if (!Models)
	{
		return;
	}
	for (const uint32 Key : Keys)
	{
		if (Key != ExcludeKey)
		{
			if (URockInventoryClientModel* Model = Models->FindModel(FindInventory(Key)))
			{
				Model->Rebuild();
			}
		}
	}
}

bool URockInventoryManagerComponent::SettlePending()
{
	return PredictionQueue.Settle([this](uint32 Key, int32 Revision)
	{
		const URockInventory* Inventory = FindInventory(Key);
		// An inventory that is gone has nothing left to wait for
		return !Inventory || FRockPredictionQueue::RevisionReached(static_cast<int32>(Inventory->GetRevision()), Revision);
	}) > 0;
}

void URockInventoryManagerComponent::ApplyPrediction(const URockInventory& Inventory, FRockInventoryData& Data)
{
	if (PredictionQueue.IsEmpty())
	{
		return;
	}
	const uint32 Key = Inventory.GetUniqueID();
	TArray<uint32> Keys;
	PredictionQueue.GetTouchedKeys(Keys);
	if (SettlePending())
	{
		// The moves that just settled also changed what the other inventories show
		RefreshModels(Keys, Key);
		Keys.Reset();
		PredictionQueue.GetTouchedKeys(Keys);
		UpdateInputHold();
	}
	if (!Keys.Contains(Key))
	{
		if (PredictionQueue.IsEmpty())
		{
			PredictionInventories.Reset();
		}
		return;
	}
	FRockPredictionState State;
	for (const uint32 Touched : Keys)
	{
		if (Touched == Key)
		{
			State.Add(Touched, Data);
		}
		else if (const URockInventory* Other = FindInventory(Touched))
		{
			State.Add(Touched, FRockInventoryData::FromInventory(Other));
		}
	}
	PredictionQueue.Rebase(State);
	Data = MoveTemp(State.FindChecked(Key));
}

void URockInventoryManagerComponent::ExpirePending()
{
	if (PredictionQueue.IsEmpty())
	{
		return;
	}
	ConfigureQueue();
	TArray<uint32> Keys;
	PredictionQueue.GetTouchedKeys(Keys);
	const int32 Abandoned = PredictionQueue.Expire(GetPredictionTime());
	if (Abandoned > 0)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("%d predicted inventory command(s) of %s got no confirmation in %.1f s and were abandoned; showing the replicated state"), Abandoned, *GetNameSafe(GetOwner()), PredictionQueue.AbandonSeconds);
		RefreshModels(Keys);
		if (PredictionQueue.IsEmpty())
		{
			PredictionInventories.Reset();
		}
	}
}

bool URockInventoryManagerComponent::IsInputHeld() const
{
	return !PredictionQueue.IsEmpty() && !PredictionQueue.CanPredict(GetPredictionTime());
}

void URockInventoryManagerComponent::UpdateInputHold()
{
	ExpirePending();
	const bool bHeld = IsInputHeld();
	if (bHeld != bInputHeld)
	{
		bInputHeld = bHeld;
		OnInputHoldChanged.Broadcast(bHeld);
	}
	if (PredictionQueue.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

void URockInventoryManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateInputHold();
}

bool URockInventoryManagerComponent::AcceptSequence(int32 Sequence)
{
	if (Sequence == 0)
	{
		return true;
	}
	// Reliable RPCs of one connection arrive in order, so a number at or below the last one is a replay
	if (Sequence <= LastAcceptedSequence)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server command %d from %s refused: sequence already seen (last %d)"), Sequence, *GetNameSafe(GetOwner()), LastAcceptedSequence);
		return false;
	}
	LastAcceptedSequence = Sequence;
	return true;
}

void URockInventoryManagerComponent::SendResult(int32 Sequence, bool bSuccess, TConstArrayView<const URockInventory*> Touched)
{
	TArray<FRockInventoryRevision> Revisions;
	if (bSuccess)
	{
		for (const URockInventory* Inventory : Touched)
		{
			if (Inventory && !Revisions.ContainsByPredicate([Inventory](const FRockInventoryRevision& Entry) { return Entry.Inventory == Inventory; }))
			{
				FRockInventoryRevision& Entry = Revisions.AddDefaulted_GetRef();
				Entry.Inventory = const_cast<URockInventory*>(Inventory);
				Entry.Revision = static_cast<int32>(Inventory->GetRevision());
			}
		}
	}
	Client_TransactionResult(Sequence, bSuccess, Revisions);
}

void URockInventoryManagerComponent::Client_TransactionResult_Implementation(int32 Sequence, bool bSuccess, const TArray<FRockInventoryRevision>& Touched)
{
	HandleHistoryAnswer(Sequence, bSuccess);
	if (PredictionQueue.IsEmpty())
	{
		return;
	}
	TArray<uint32> Keys;
	PredictionQueue.GetTouchedKeys(Keys);

	TArray<FRockKeyedRevision> Revisions;
	for (const FRockInventoryRevision& Entry : Touched)
	{
		if (Entry.Inventory)
		{
			Revisions.Emplace(Entry.Inventory->GetUniqueID(), Entry.Revision);
		}
	}
	// A refusal leaves the queue here and the rebuild shows the replicated data again; an acceptance stays until the replicated
	// inventories reach the revisions the server names.
	PredictionQueue.Ack(Sequence, bSuccess, Revisions, GetPredictionTime());
	SettlePending();
	RefreshModels(Keys);
	if (PredictionQueue.IsEmpty())
	{
		PredictionInventories.Reset();
	}
	UpdateInputHold();
}

// Commands --------------------------------------------------------------------------------------------------------------------

void URockInventoryManagerComponent::LootWorldItem(const FRockLootWorldItemTransaction& ItemTransaction)
{
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	// The client never writes replicated inventory state: the server executes and the result replicates back.
	FRockLootWorldItemTransaction Command = ItemTransaction;
	Command.TransactionID = NextSequence();
	SendLoot(Command);
	AddUndoBarrier();
}

void URockInventoryManagerComponent::Server_LootWorldItem_Implementation(FRockLootWorldItemTransaction ItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_AddItem - Not authority!"));
		return;
	}
	bool bSuccess = false;
	if (AcceptSequence(ItemTransaction.TransactionID) && AuthorizeServerCommand(ItemTransaction, {ItemTransaction.TargetInventory}) && ItemTransaction.CanExecute())
	{
		bSuccess = ItemTransaction.Execute().bSuccess;
	}
	SendResult(ItemTransaction.TransactionID, bSuccess, {ItemTransaction.TargetInventory});
}


bool URockInventoryManagerComponent::MoveItem(const FRockMoveItemTransaction& ItemTransaction)
{
	TArray<FRockMoveItemTransaction> Commands;
	FRockMoveItemTransaction& Command = Commands.Add_GetRef(ItemTransaction);
	// A player's move takes its preconditions from what the player sees
	Command.ExpectedSource = FRockSlotExpectation();
	Command.ExpectedTarget = FRockSlotExpectation();
	return SendMoves(Commands, EMoveOrigin::Player) == ESendResult::Sent;
}

ESendResult URockInventoryManagerComponent::SendMoves(TArray<FRockMoveItemTransaction>& Commands, EMoveOrigin Origin)
{
	if (Commands.IsEmpty())
	{
		return ESendResult::Refused;
	}
	const bool bPredict = ShouldPredict();
	if (!bPredict && Origin == EMoveOrigin::Player && !Commands[0].CanExecute())
	{
		// Not predicting, the inventory itself is what the player sees: refuse here, with the reason logged
		return ESendResult::Refused;
	}
	for (const FRockMoveItemTransaction& Command : Commands)
	{
		if (!Command.SourceInventory || !Command.TargetInventory || !Command.SourceSlotHandle.IsValid() || !Command.TargetSlotHandle.IsValid())
		{
			return ESendResult::Refused;
		}
	}
	if (bPredict)
	{
		ConfigureQueue();
		SettlePending();
		ExpirePending();
		if (!PredictionQueue.CanPredict(GetPredictionTime()))
		{
			UpdateInputHold();
			return ESendResult::Held;
		}
		BindPredictionToModels();
	}

	// The shown state decides, pending commands included, not the replicated inventory (an item a pending command moved is not yet
	// where the replicated inventory has it).
	TMap<uint32, URockInventory*> Involved;
	TArray<uint32> Keys;
	PredictionQueue.GetTouchedKeys(Keys);
	for (const uint32 Key : Keys)
	{
		Involved.Add(Key, FindInventory(Key));
	}
	for (const FRockMoveItemTransaction& Command : Commands)
	{
		Involved.Add(Command.SourceInventory->GetUniqueID(), Command.SourceInventory);
		Involved.Add(Command.TargetInventory->GetUniqueID(), Command.TargetInventory);
	}
	Involved.GenerateKeyArray(Keys);
	FRockPredictionState State;
	for (const TPair<uint32, URockInventory*>& Pair : Involved)
	{
		if (Pair.Value)
		{
			State.Add(Pair.Key, FRockInventoryData::FromInventory(Pair.Value));
		}
	}
	PredictionQueue.Rebase(State);

	// The whole action is checked on a copy first: it goes out complete or not at all. Each move's undo step comes with it.
	FRockPredictionState Scratch = State;
	TArray<FRockUndoStep> Steps;
	TArray<ERockUndoStepResult> Results;
	for (FRockMoveItemTransaction& Command : Commands)
	{
		FRockUndoMove Move;
		Move.SourceKey = Command.SourceInventory->GetUniqueID();
		Move.SourceSlot = Command.SourceSlotHandle;
		Move.TargetKey = Command.TargetInventory->GetUniqueID();
		Move.TargetSlot = Command.TargetSlotHandle;
		Move.Params = Command.MoveParams;
		Move.ExpectedSource = Command.ExpectedSource;
		Move.ExpectedTarget = Command.ExpectedTarget;
		const ERockUndoStepResult Result = FRockUndoStep::Make(Scratch, Move, Steps.AddDefaulted_GetRef());
		if (Result == ERockUndoStepResult::Stale)
		{
			return ESendResult::Stale;
		}
		if (Result == ERockUndoStepResult::Refused)
		{
			return ESendResult::Refused;
		}
		Results.Add(Result);
		// The preconditions hold the server to what this client saw, predicting or not
		Command.ExpectedSource = Steps.Last().Forward.ExpectedSource;
		Command.ExpectedTarget = Steps.Last().Forward.ExpectedTarget;
	}

	TArray<int32> Sequences;
	bool bComplete = true;
	for (FRockMoveItemTransaction& Command : Commands)
	{
		if (bPredict)
		{
			PredictionInventories.FindOrAdd(Command.SourceInventory->GetUniqueID()) = Command.SourceInventory;
			PredictionInventories.FindOrAdd(Command.TargetInventory->GetUniqueID()) = Command.TargetInventory;
			FRockPredictedMove Move;
			Move.Sequence = NextSequence();
			Move.SourceKey = Command.SourceInventory->GetUniqueID();
			Move.SourceSlot = Command.SourceSlotHandle;
			Move.TargetKey = Command.TargetInventory->GetUniqueID();
			Move.TargetSlot = Command.TargetSlotHandle;
			Move.Params = Command.MoveParams;
			// Same rules on the same state as the check above, so this cannot refuse
			if (!ensureMsgf(PredictionQueue.Predict(State, Move, GetPredictionTime()) == ERockMoveRefusal::None, TEXT("A move checked on the shown state was refused when predicted")))
			{
				bComplete = false;
				break;
			}
			Command.TransactionID = Move.Sequence;
		}
		else
		{
			Command.TransactionID = NextSequence();
		}
		Sequences.Add(Command.TransactionID);
		HistoryInventories.FindOrAdd(Command.SourceInventory->GetUniqueID()) = Command.SourceInventory;
		HistoryInventories.FindOrAdd(Command.TargetInventory->GetUniqueID()) = Command.TargetInventory;
	}
	Commands.SetNum(Sequences.Num());

	// The history first: an authority executes and answers inside SendMove
	if (Origin == EMoveOrigin::Player)
	{
		if (bComplete)
		{
			RecordPlayerSteps(MoveTemp(Steps), Results, Sequences);
		}
		else
		{
			AddUndoBarrier();
		}
	}
	else
	{
		const bool bUndo = Origin == EMoveOrigin::Undo;
		if (bComplete && bUndo)
		{
			UndoHistory.MarkUndone(Sequences);
		}
		else if (bComplete)
		{
			UndoHistory.MarkRedone(Sequences);
		}
		else
		{
			UndoHistory.DropTop(bUndo);
		}
		HistoryCommandSequences.Append(Sequences);
		UndoHistoryChanged();
	}

	if (bPredict)
	{
		// Show it before sending: an answer that comes back at once (a listen server, a test) finds the move queued
		RefreshModels(Keys);
		SetComponentTickEnabled(true);
		UpdateInputHold();
	}
	for (const FRockMoveItemTransaction& Command : Commands)
	{
		SendMove(Command);
	}
	return Commands.IsEmpty() ? ESendResult::Refused : ESendResult::Sent;
}

// Undo and redo ---------------------------------------------------------------------------------------------------------------

bool URockInventoryManagerComponent::Undo()
{
	return StepHistory(true);
}

bool URockInventoryManagerComponent::Redo()
{
	return StepHistory(false);
}

bool URockInventoryManagerComponent::StepHistory(bool bUndo)
{
	const FRockUndoEntry* Entry = bUndo ? UndoHistory.PeekUndo() : UndoHistory.PeekRedo();
	if (!Entry || BatchDepth > 0)
	{
		return false;
	}
	TArray<FRockUndoMove> Moves;
	Entry->GetMoves(bUndo, Moves);
	TArray<FRockMoveItemTransaction> Commands;
	for (const FRockUndoMove& Move : Moves)
	{
		URockInventory* Source = FindHistoryInventory(Move.SourceKey);
		URockInventory* Target = FindHistoryInventory(Move.TargetKey);
		if (!Source || !Target)
		{
			DropUndoEntry(bUndo, TEXT("an inventory it touched is gone"));
			return false;
		}
		FRockMoveItemTransaction& Command = Commands.Emplace_GetRef(GetOwningController(), Source, Move.SourceSlot, Target, Move.TargetSlot, Move.Params);
		Command.ExpectedSource = Move.ExpectedSource;
		Command.ExpectedTarget = Move.ExpectedTarget;
	}
	switch (SendMoves(Commands, bUndo ? EMoveOrigin::Undo : EMoveOrigin::Redo))
	{
	case ESendResult::Sent:
		return true;
	case ESendResult::Held:
		// Try again once the server has caught up; the entry stays
		return false;
	case ESendResult::Stale:
		DropUndoEntry(bUndo, TEXT("its items moved since"));
		return false;
	case ESendResult::Refused:
	default:
		DropUndoEntry(bUndo, TEXT("the inventory refuses the move now"));
		return false;
	}
}

void URockInventoryManagerComponent::RecordPlayerSteps(TArray<FRockUndoStep>&& Steps, TConstArrayView<ERockUndoStepResult> Results, TConstArrayView<int32> Sequences)
{
	FRockUndoEntry Entry;
	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		if (Results[Index] == ERockUndoStepResult::NotInvertible)
		{
			// Undo could not take it back, so undo stops here
			AddUndoBarrier();
			return;
		}
		if (Results[Index] == ERockUndoStepResult::Undoable)
		{
			Entry.Steps.Add(MoveTemp(Steps[Index]));
		}
	}
	if (Entry.Steps.IsEmpty())
	{
		return;
	}
	Entry.PendingSequences.Append(Sequences.GetData(), Sequences.Num());
	if (BatchDepth > 0)
	{
		OpenBatch.Steps.Append(MoveTemp(Entry.Steps));
		OpenBatch.PendingSequences.Append(Entry.PendingSequences);
		return;
	}
	UndoHistory.MaxEntries = GetDefault<URockInventoryDeveloperSettings>()->UndoHistoryDepth;
	UndoHistory.Record(MoveTemp(Entry));
	UndoHistoryChanged();
}

void URockInventoryManagerComponent::HandleHistoryAnswer(int32 Sequence, bool bSuccess)
{
	const bool bUndoOrRedo = HistoryCommandSequences.Remove(Sequence) > 0;
	if (bSuccess)
	{
		UndoHistory.Confirm(Sequence);
		OpenBatch.PendingSequences.Remove(Sequence);
		return;
	}
	if (BatchDepth > 0 && OpenBatch.PendingSequences.Contains(Sequence))
	{
		// Part of the batch did not happen, so it cannot be undone as one
		bDiscardBatch = true;
	}
	if (UndoHistory.DropBySequence(Sequence))
	{
		if (bUndoOrRedo)
		{
			UE_LOG(LogRockInventory, Log, TEXT("The server refused an undo or redo of %s; the entry was dropped"), *GetNameSafe(GetOwner()));
			OnUndoEntryDropped.Broadcast();
		}
		UndoHistoryChanged();
	}
}

void URockInventoryManagerComponent::DropUndoEntry(bool bUndo, const TCHAR* Why)
{
	// Not an error: another player or the server moved the items, which is what the preconditions are for
	UE_LOG(LogRockInventory, Log, TEXT("%s of %s not possible, %s; the entry was dropped"), bUndo ? TEXT("Undo") : TEXT("Redo"), *GetNameSafe(GetOwner()), Why);
	UndoHistory.DropTop(bUndo);
	OnUndoEntryDropped.Broadcast();
	UndoHistoryChanged();
}

void URockInventoryManagerComponent::AddUndoBarrier()
{
	ClearUndoHistory();
}

void URockInventoryManagerComponent::ClearUndoHistory()
{
	if (BatchDepth > 0)
	{
		bDiscardBatch = true;
	}
	HistoryCommandSequences.Reset();
	if (UndoHistory.IsEmpty())
	{
		return;
	}
	UndoHistory.Clear();
	UndoHistoryChanged();
}

void URockInventoryManagerComponent::SeverUndoHistory(URockInventory* Inventory)
{
	if (!Inventory)
	{
		return;
	}
	const uint32 Key = Inventory->GetUniqueID();
	if (BatchDepth > 0 && OpenBatch.Touches(Key))
	{
		bDiscardBatch = true;
	}
	int32 Dropped = UndoHistory.Sever(Key);
	// A weapon's attachments (FollowsParent) close with it
	for (const TPair<uint32, TWeakObjectPtr<URockInventory>>& Pair : HistoryInventories)
	{
		const URockInventory* Other = Pair.Value.Get();
		if (Other && Other != Inventory && Other->GetGatingRoot() == Inventory)
		{
			Dropped += UndoHistory.Sever(Pair.Key);
		}
	}
	if (Dropped > 0)
	{
		UndoHistoryChanged();
	}
}

void URockInventoryManagerComponent::BeginUndoBatch()
{
	if (BatchDepth++ == 0)
	{
		OpenBatch = FRockUndoEntry();
		bDiscardBatch = false;
	}
}

void URockInventoryManagerComponent::EndUndoBatch()
{
	if (!ensureMsgf(BatchDepth > 0, TEXT("EndUndoBatch without a BeginUndoBatch")) || --BatchDepth > 0)
	{
		return;
	}
	FRockUndoEntry Entry = MoveTemp(OpenBatch);
	OpenBatch = FRockUndoEntry();
	if (!bDiscardBatch && !Entry.Steps.IsEmpty())
	{
		UndoHistory.MaxEntries = GetDefault<URockInventoryDeveloperSettings>()->UndoHistoryDepth;
		UndoHistory.Record(MoveTemp(Entry));
		UndoHistoryChanged();
	}
	bDiscardBatch = false;
}

void URockInventoryManagerComponent::AddInventoryView(const URockInventory* Inventory)
{
	if (Inventory)
	{
		++InventoryViews.FindOrAdd(Inventory->GetUniqueID());
	}
}

void URockInventoryManagerComponent::RemoveInventoryView(URockInventory* Inventory)
{
	if (!Inventory)
	{
		return;
	}
	const uint32 Key = Inventory->GetUniqueID();
	int32* Count = InventoryViews.Find(Key);
	if (!Count || --*Count > 0)
	{
		return;
	}
	InventoryViews.Remove(Key);
	if (InventoryViews.IsEmpty())
	{
		// The last inventory widget went: the inventory screen closed
		ClearUndoHistory();
	}
	else
	{
		SeverUndoHistory(Inventory);
	}
}

URockInventory* URockInventoryManagerComponent::FindHistoryInventory(uint32 Key) const
{
	const TWeakObjectPtr<URockInventory>* Found = HistoryInventories.Find(Key);
	return Found ? Found->Get() : nullptr;
}

void URockInventoryManagerComponent::UndoHistoryChanged()
{
	TArray<uint32> Keys;
	UndoHistory.GetKeys(Keys);
	for (const FRockUndoStep& Step : OpenBatch.Steps)
	{
		Keys.AddUnique(Step.Forward.SourceKey);
		Keys.AddUnique(Step.Forward.TargetKey);
	}
	for (auto It = HistoryInventories.CreateIterator(); It; ++It)
	{
		if (!Keys.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
	OnUndoHistoryChanged.Broadcast();
}

void URockInventoryManagerComponent::Server_MoveItem_Implementation(FRockMoveItemTransaction ItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_MoveItem - Not authority!"));
		return;
	}
	// Every command is answered, a refusal too: the client keeps its prediction until it hears
	bool bSuccess = false;
	if (AcceptSequence(ItemTransaction.TransactionID)
		&& AuthorizeServerCommand(ItemTransaction, {ItemTransaction.SourceInventory, ItemTransaction.TargetInventory})
		&& ItemTransaction.CanExecute())
	{
		bSuccess = ItemTransaction.Execute().bSuccess;
	}
	SendResult(ItemTransaction.TransactionID, bSuccess, {ItemTransaction.SourceInventory, ItemTransaction.TargetInventory});
}


void URockInventoryManagerComponent::DropItem(const FRockDropItemTransaction& ItemTransaction)
{
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	// The client never writes replicated inventory state: the server executes and the result replicates back.
	FRockDropItemTransaction Command = ItemTransaction;
	Command.TransactionID = NextSequence();
	SendDrop(Command);
	AddUndoBarrier();
}

void URockInventoryManagerComponent::Server_DropItem_Implementation(FRockDropItemTransaction ItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_DropItem - Not authority!"));
		return;
	}
	bool bSuccess = false;
	if (AcceptSequence(ItemTransaction.TransactionID) && AuthorizeServerCommand(ItemTransaction, {ItemTransaction.SourceInventory}) && ItemTransaction.CanExecute())
	{
		bSuccess = ItemTransaction.Execute().bSuccess;
	}
	SendResult(ItemTransaction.TransactionID, bSuccess, {ItemTransaction.SourceInventory});
}

void URockInventoryManagerComponent::Server_RegisterSlotStatus_Implementation(
	URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus)
{
	// Client-supplied parameter; don't let a null crash the server
	if (!ensureMsgf(Inventory, TEXT("Server_RegisterSlotStatus_Implementation: Inventory is null")))
	{
		return;
	}
	AController* Controller = GetOwningController();
	if (!Controller || !CanAccess(Inventory, Controller))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_RegisterSlotStatus - %s may not access inventory %s, refused"), *GetNameSafe(Controller), *GetNameSafe(Inventory));
		return;
	}
	Inventory->RegisterSlotStatus(Controller, InSlotHandle, InStatus);
}

void URockInventoryManagerComponent::Server_ReleaseSlotStatus_Implementation(
	URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle)
{
	// Client-supplied parameter; don't let a null crash the server
	if (!ensureMsgf(Inventory, TEXT("Server_ReleaseSlotStatus_Implementation: Inventory is null")))
	{
		return;
	}
	AController* Controller = GetOwningController();
	if (!Controller || !CanAccess(Inventory, Controller))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_ReleaseSlotStatus - %s may not access inventory %s, refused"), *GetNameSafe(Controller), *GetNameSafe(Inventory));
		return;
	}
	Inventory->ReleaseSlotStatus(Controller, InSlotHandle);
}
