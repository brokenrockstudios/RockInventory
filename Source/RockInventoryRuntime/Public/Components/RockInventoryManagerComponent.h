// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/RockPendingSlotOperation.h"
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

	// TODO: static URockInventoryManagerComponent* Get(UObject* WorldContextObject);

	/**
	 * Whether Instigator may touch Inventory through a server command. Every Server_* command checks this for each inventory it names.
	 * Default: the inventory belongs to the instigator's pawn, controller or player state, or its owning actor is within MaxAccessReach of the instigator's pawn.
	 * Override for containers with their own rules (shared stashes, team lockers).
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Rock|Inventory")
	bool CanAccess(const URockInventory* Inventory, const AController* Instigator) const;
	virtual bool CanAccess_Implementation(const URockInventory* Inventory, const AController* Instigator) const;

	/** The controller that owns this component's connection: the owner itself, its pawn's controller, or its player state's controller. Null if none. */
	AController* GetOwningController() const;

	/**
	 * Server-side gate for a client command: replaces the client-supplied Instigator with GetOwningController() and checks CanAccess for each inventory.
	 * Returns false (and logs) when there is no owning controller, an inventory is null, or access is refused.
	 */
	bool AuthorizeServerCommand(FRockItemTransactionBase& Command, TConstArrayView<const URockInventory*> Inventories) const;

	/** How far (cm) the instigator's pawn may be from an inventory's owning actor in the default CanAccess. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rock|Inventory", meta = (ClampMin = "0"))
	float MaxAccessReach = 500.f;

private:
	bool bAwaitingServerSync = false;

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
