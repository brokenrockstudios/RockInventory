// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Components/RockInventoryManagerComponent.h"

#include "Access/RockInventoryAccessSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Net/UnrealNetwork.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "Transactions/Implementations/RockMoveItemTransaction.h"

URockInventoryManagerComponent::URockInventoryManagerComponent(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
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

void URockInventoryManagerComponent::Client_TransactionResult_Implementation(int32 ClientTransactionID, bool bSuccess)
{
	// If we ever receive a bSuccess == false
	// we need to clear history and refresh everything!
	// Since our prediction is wrong and the state could be out of sync
	if (bSuccess)
	{
		// At this time, we shouldn't ever have more than 1 predictive move. As that complicates unwinding a lot of things.
		// PendingServerTransactions.Remove(ClientTransactionID);
	}
	else
	{
		// UH OH. Prevent any further transactions until we've resynced
		bAwaitingServerSync = true;
		// ensureMsgf(false, TEXT("ClientTransactionResult_Implementation - Not yet implemented"));
		// Request a redownload of the inventory of the relevant inventory and slots.
	}
}

void URockInventoryManagerComponent::LootWorldItem(const FRockLootWorldItemTransaction& ItemTransaction)
{
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	// The client never writes replicated inventory state: the server executes and the result replicates back.
	Server_LootWorldItem(ItemTransaction);
}

void URockInventoryManagerComponent::Server_LootWorldItem_Implementation(FRockLootWorldItemTransaction ItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_AddItem - Not authority!"));
		return;
	}
	if (!AuthorizeServerCommand(ItemTransaction, {ItemTransaction.TargetInventory}))
	{
		return;
	}
	// If we can't execute, don't execute
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	const FRockLootWorldItemUndoTransaction& Undo = ItemTransaction.Execute();

	Client_TransactionResult(ItemTransaction.TransactionID, Undo.bSuccess);
}


bool URockInventoryManagerComponent::MoveItem(const FRockMoveItemTransaction& ItemTransaction)
{
	if (!ItemTransaction.CanExecute())
	{
		return false;
	}
	// The client never writes replicated inventory state: the server executes and the result replicates back.
	Server_MoveItem(ItemTransaction);
	return true;
}

void URockInventoryManagerComponent::Server_MoveItem_Implementation(FRockMoveItemTransaction ItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_MoveItem - Not authority!"));
		return;
	}
	if (!AuthorizeServerCommand(ItemTransaction, {ItemTransaction.SourceInventory, ItemTransaction.TargetInventory}))
	{
		return;
	}
	// If we can't execute, don't execute
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	const FRockMoveItemUndoTransaction& Undo = ItemTransaction.Execute();
	Client_TransactionResult(ItemTransaction.TransactionID, Undo.bSuccess);
}


void URockInventoryManagerComponent::DropItem(const FRockDropItemTransaction& ItemTransaction)
{
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	// The client never writes replicated inventory state: the server executes and the result replicates back.
	Server_DropItem(ItemTransaction);
}

void URockInventoryManagerComponent::Server_DropItem_Implementation(FRockDropItemTransaction ItemTransaction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Server_DropItem - Not authority!"));
		return;
	}
	if (!AuthorizeServerCommand(ItemTransaction, {ItemTransaction.SourceInventory}))
	{
		return;
	}
	// If we can't execute, don't execute
	if (!ItemTransaction.CanExecute())
	{
		return;
	}
	const FRockDropItemUndoTransaction& Undo = ItemTransaction.Execute();
	Client_TransactionResult(ItemTransaction.TransactionID, Undo.bSuccess);
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
