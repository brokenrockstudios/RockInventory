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
	Server_LootWorldItem(Command);
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
	FRockMoveItemTransaction Command = ItemTransaction;
	if (!ShouldPredict())
	{
		if (!ItemTransaction.CanExecute())
		{
			return false;
		}
		Command.TransactionID = NextSequence();
		if (GetOwnerRole() != ROLE_Authority)
		{
			// Predicting is off, the preconditions still hold the server to what this client saw
			Command.ExpectedSource = FRockSlotExpectation::Capture(*Command.SourceInventory, Command.SourceSlotHandle);
			Command.ExpectedTarget = FRockSlotExpectation::Capture(*Command.TargetInventory, Command.TargetSlotHandle);
		}
		SendMove(Command);
		return true;
	}

	// Predicting: the shown state, pending commands included, decides, not the replicated inventory (an item a pending command
	// moved is not yet where the replicated inventory has it).
	if (!Command.SourceInventory || !Command.TargetInventory || !Command.SourceSlotHandle.IsValid() || !Command.TargetSlotHandle.IsValid())
	{
		return false;
	}
	ConfigureQueue();
	SettlePending();
	ExpirePending();
	if (!PredictionQueue.CanPredict(GetPredictionTime()))
	{
		UpdateInputHold();
		return false;
	}
	BindPredictionToModels();

	PredictionInventories.FindOrAdd(Command.SourceInventory->GetUniqueID()) = Command.SourceInventory;
	PredictionInventories.FindOrAdd(Command.TargetInventory->GetUniqueID()) = Command.TargetInventory;
	TArray<uint32> Keys;
	PredictionQueue.GetTouchedKeys(Keys);
	Keys.AddUnique(Command.SourceInventory->GetUniqueID());
	Keys.AddUnique(Command.TargetInventory->GetUniqueID());
	FRockPredictionState State;
	for (const uint32 Key : Keys)
	{
		if (const URockInventory* Inventory = FindInventory(Key))
		{
			State.Add(Key, FRockInventoryData::FromInventory(Inventory));
		}
	}
	PredictionQueue.Rebase(State);

	FRockPredictedMove Move;
	Move.Sequence = NextSequence();
	Move.SourceKey = Command.SourceInventory->GetUniqueID();
	Move.SourceSlot = Command.SourceSlotHandle;
	Move.TargetKey = Command.TargetInventory->GetUniqueID();
	Move.TargetSlot = Command.TargetSlotHandle;
	Move.Params = Command.MoveParams;
	if (PredictionQueue.Predict(State, Move, GetPredictionTime()) != ERockMoveRefusal::None)
	{
		return false;
	}
	const FRockPredictedMove& Queued = PredictionQueue.GetPending().Last();
	Command.TransactionID = Queued.Sequence;
	Command.ExpectedSource = Queued.ExpectedSource;
	Command.ExpectedTarget = Queued.ExpectedTarget;

	// Show it before sending: an answer that comes back at once (a listen server, a test) finds the move queued
	RefreshModels(Keys);
	SetComponentTickEnabled(true);
	UpdateInputHold();
	SendMove(Command);
	return true;
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
	Server_DropItem(Command);
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
