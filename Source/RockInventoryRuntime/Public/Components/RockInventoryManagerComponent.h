// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Access/RockInventoryAccessSubsystem.h"
#include "Inventory/RockPendingSlotOperation.h"
#include "Replication/RockInventoryReplication.h"
#include "StructUtils/InstancedStruct.h"
#include "Transactions/Implementations/RockDropItemTransaction.h"
#include "Transactions/Implementations/RockLootWorldItemTransaction.h"
#include "Transactions/Implementations/RockMoveItemTransaction.h"
#include "RockInventoryManagerComponent.generated.h"

class URockInventory;
class URockInventoryComponent;

// Should put this on the PlayerController?
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYRUNTIME_API URockInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URockInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// TODO: static URockInventoryManagerComponent* Get(UObject* WorldContextObject);

	/**
	 * Whether Instigator has at least the Required rights on Inventory. Every Server_* command checks this for each inventory it names.
	 * Asks URockInventoryAccessSubsystem: the inventory sits on the instigator, or the instigator opened it and is within reach (deny by default).
	 * Override for rules the registry cannot express.
	 */
	virtual bool CanAccess(const URockInventory* Inventory, const AController* Instigator, ERockInventoryRights Required = ERockInventoryRights::Full) const;

	/** Asks the server to open Inventory for this component's owning controller (a chest, a body, a dropped backpack). Refusals only log. */
	UFUNCTION(Server, Reliable)
	void Server_OpenInventory(URockInventory* Inventory);
	void Server_OpenInventory_Implementation(URockInventory* Inventory);
	UFUNCTION(Server, Reliable)
	void Server_CloseInventory(URockInventory* Inventory);
	void Server_CloseInventory_Implementation(URockInventory* Inventory);

	/** The manager component on this controller, its player state or its pawn. Null if none. */
	static URockInventoryManagerComponent* FindFor(const AController* Controller);
	/** The manager component of the first local player controller in the world (a client's own). Null if none. */
	static URockInventoryManagerComponent* FindLocal(const UWorld* World);

	/**
	 * Server: adds or removes Inventory in this player's Observed list, which replicates to the owning client only. The registry calls it when
	 * the player gains or loses View rights, so the client can tell Syncing, Live and Stale apart (URockInventory::GetSyncState).
	 */
	void ServerSetObserved(URockInventory* Inventory, bool bObserved);
	/** The grant for Inventory in the replicated list, or null. */
	const FRockObservedInventory* FindObserved(const URockInventory* Inventory) const;
	const TArray<FRockObservedInventory>& GetObserved() const { return Observed; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** The controller that owns this component's connection: the owner itself, its pawn's controller, or its player state's controller. Null if none. */
	AController* GetOwningController() const;

	/**
	 * Server-side gate for a client command: replaces the client-supplied Instigator with GetOwningController() and checks CanAccess for each inventory.
	 * Returns false (and logs) when there is no owning controller, an inventory is null, or access is refused.
	 */
	bool AuthorizeServerCommand(FRockItemTransactionBase& Command, TConstArrayView<const URockInventory*> Inventories) const;

private:
	bool bAwaitingServerSync = false;

	/** Inventories the server replicates to this player on top of their own (opened, nearby, shared). Owner only. */
	UPROPERTY(ReplicatedUsing = OnRep_Observed)
	TArray<FRockObservedInventory> Observed;
	UFUNCTION()
	void OnRep_Observed(const TArray<FRockObservedInventory>& OldObserved);

public:
	/**
	 * Send transaction result to client
	 *
	 * @param ClientTransactionID - The transaction ID to send the result for
	 * @param bSuccess - Whether the transaction was successful or not
	 */
	UFUNCTION(Client, Reliable)
	void Client_TransactionResult(int32 ClientTransactionID, bool bSuccess);
	void Client_TransactionResult_Implementation(int32 ClientTransactionID, bool bSuccess);

	// Basic Inventory CRUD functions
	UFUNCTION(BlueprintCallable)
	void LootWorldItem(const FRockLootWorldItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_LootWorldItem(FRockLootWorldItemTransaction ItemTransaction);
	void Server_LootWorldItem_Implementation(FRockLootWorldItemTransaction ItemTransaction);

	// TODO: Give a 'preferred location' option, and what to do if it can't place it there (fallback to other slots or 'fail')
	// TODO: For a server to do an action like 'give players to the item' from a task reward or something.  Need a more fleshed out UX dev consumer pattern

	UFUNCTION(BlueprintCallable)
	bool MoveItem(const FRockMoveItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_MoveItem(FRockMoveItemTransaction ItemTransaction);
	void Server_MoveItem_Implementation(FRockMoveItemTransaction ItemTransaction);

	UFUNCTION(BlueprintCallable)
	void DropItem(const FRockDropItemTransaction& ItemTransaction);
	UFUNCTION(Server, Reliable)
	void Server_DropItem(FRockDropItemTransaction ItemTransaction);
	void Server_DropItem_Implementation(FRockDropItemTransaction ItemTransaction);

	/** The slot is claimed for this component's owning controller; the client cannot name another. */
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RegisterSlotStatus(URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus);
	void Server_RegisterSlotStatus_Implementation(URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_ReleaseSlotStatus(URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle);
	void Server_ReleaseSlotStatus_Implementation(URockInventory* Inventory, const FRockInventorySlotHandle& InSlotHandle);
};
