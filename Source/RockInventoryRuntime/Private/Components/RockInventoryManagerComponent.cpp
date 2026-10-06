// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Components/RockInventoryManagerComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Transactions/Core/RockInventoryTransaction.h"
#include "Transactions/Implementations/RockMoveItemTransaction.h"

URockInventoryManagerComponent::URockInventoryManagerComponent(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

bool URockInventoryManagerComponent::CanAccess_Implementation(const URockInventory* Inventory, const AController* Instigator) const
{
	if (!Inventory || !Instigator)
	{
		return false;
	}
	const AActor* InventoryActor = const_cast<URockInventory*>(Inventory)->GetOwningActor();
	if (!InventoryActor)
	{
		return false;
	}
	const APawn* Pawn = Instigator->GetPawn();
	if (InventoryActor == Instigator || InventoryActor == Pawn || InventoryActor == Instigator->PlayerState)
	{
		return true;
	}
	return Pawn && Pawn->GetDistanceTo(InventoryActor) <= MaxAccessReach;
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
