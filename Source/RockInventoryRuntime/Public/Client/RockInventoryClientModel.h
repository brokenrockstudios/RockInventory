// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockInventoryData.h"
#include "Replication/RockInventoryReplication.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "RockInventoryClientModel.generated.h"

class URockInventory;
class URockInventoryClientModel;
class URockInventoryManagerComponent;
struct FRockInventoryChangeBatch;

/**
 * What visibly changed between two states of a client model: the only thing the UI has to react to.
 * Slots are listed once each (a move is two slots), stacks by the handle they have in the old and the new data.
 * Plain data, no UObject.
 */
struct ROCKINVENTORYRUNTIME_API FRockInventoryPresentationDiff
{
	/** Slots whose item handle, orientation or lock state differ (a slot that exists in only one of the states counts). */
	TArray<FRockInventorySlotHandle> ChangedSlots;
	/** Handles that hold a stack in the new data and none in the old. */
	TArray<FRockItemStackHandle> StacksCreated;
	/** Handles that held a stack in the old data and none in the new. */
	TArray<FRockItemStackHandle> StacksRemoved;
	/** Handles that hold a different stack in the new data than in the old (count, custom values, instance). */
	TArray<FRockItemStackHandle> StacksModified;
	/** The sections differ (slot count, layout or filters): a UI rebuilds its grid. */
	bool bLayoutChanged = false;
	bool bSyncStateChanged = false;
	ERockInventorySyncState PreviousSyncState = ERockInventorySyncState::Unknown;

	bool IsEmpty() const
	{
		return ChangedSlots.IsEmpty() && StacksCreated.IsEmpty() && StacksRemoved.IsEmpty() && StacksModified.IsEmpty() && !bLayoutChanged && !bSyncStateChanged;
	}

	/** Compares two states. Pure: no world, no UObject inventory. */
	static FRockInventoryPresentationDiff Between(const FRockInventoryData& Old, ERockInventorySyncState OldState, const FRockInventoryData& New, ERockInventorySyncState NewState);
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryClientModelChanged, URockInventoryClientModel&, const FRockInventoryPresentationDiff&);

/**
 * The layer between the authority inventory and the UI (T-77). One model per observed `URockInventory` on a client (or listen
 * server): it holds a plain-data copy (`FRockInventoryData`) rebuilt from the replicated inventory whenever it announces a change
 * batch or a new sync state, diffs the old copy against the new one and tells its listeners only what visibly changed.
 *
 * Widgets and view models read the model and listen to `OnChanged`; they never call the inventory's getters. Prediction (T-78)
 * layers on top: every rebuild takes the replicated data and lets the player's manager component re-apply its pending commands on
 * it (`URockInventoryClientModelSubsystem::ApplyPrediction`), so the model is replicated data plus pending commands and the UI does
 * not change.
 *
 * `SetState` is the pure core (no inventory needed), so tests and the prediction code drive it directly.
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryClientModel : public UObject
{
	GENERATED_BODY()

public:
	/** Starts listening to the inventory and takes its current state without announcing it. A model follows one inventory for its lifetime. */
	void Bind(URockInventory* InInventory);
	void Unbind();

	/** The inventory this model mirrors (the key for commands such as moves, which still go to the server through the manager). Null once it is destroyed. */
	URockInventory* GetInventory() const { return Inventory.Get(); }

	/** Re-reads the inventory now and announces what changed. Returns the diff (empty when nothing visibly changed). */
	FRockInventoryPresentationDiff Rebuild();

	/** Replaces the state and announces the diff when it is not empty. The core of Rebuild, usable without an inventory. */
	FRockInventoryPresentationDiff SetState(FRockInventoryData&& NewData, int32 NewRevision, ERockInventorySyncState NewSyncState);

	/** Called once per diff that is not empty, after the model holds the new state. */
	FOnInventoryClientModelChanged OnChanged;

	const FRockInventoryData& GetData() const { return Data; }
	/** The inventory's Revision when this state was taken. */
	int32 GetRevision() const { return Revision; }
	ERockInventorySyncState GetSyncState() const { return SyncState; }

	// Reads mirroring the inventory's getters, so UI code changes its receiver and nothing else. Out-of-range or stale handles give the invalid value, without logging.
	FRockInventorySlotEntry GetSlotByHandle(const FRockInventorySlotHandle& SlotHandle) const;
	const FRockInventorySlotEntry& GetSlotByAbsoluteIndex(int32 AbsoluteIndex) const;
	/** The slot holding the stack (its anchor cell), or an invalid entry. */
	FRockInventorySlotEntry GetSlotByItemHandle(const FRockItemStackHandle& ItemHandle) const;
	FRockItemStack GetItemByHandle(const FRockItemStackHandle& ItemHandle) const;
	FRockItemStack GetItemBySlotHandle(const FRockInventorySlotHandle& SlotHandle) const;
	const FRockInventorySectionInfo& GetSectionInfo(const FGameplayTag& SectionTag) const;
	const FRockInventorySectionInfo& GetSectionInfoBySlotHandle(const FRockInventorySlotHandle& SlotHandle) const;
	int32 GetSectionIndex(const FGameplayTag& SectionTag) const;

private:
	UFUNCTION()
	void HandleChangeBatch(const FRockInventoryChangeBatch& Batch);
	void HandleSyncState(URockInventory& Changed, ERockInventorySyncState NewState);

	TWeakObjectPtr<URockInventory> Inventory;
	FRockInventoryData Data;
	int32 Revision = 0;
	ERockInventorySyncState SyncState = ERockInventorySyncState::Unknown;
	FDelegateHandle SyncStateHandle;

	/** The inventory's replicated state with the player's pending commands applied on it. */
	FRockInventoryData TakeState(const URockInventory& Source) const;
};

/**
 * Owns the client models of a world: `GetModel(Inventory)` creates one the first time and returns the same one afterwards, so every
 * widget showing an inventory shares one model and one rebuild per replication update.
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryClientModelSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The model for the inventory in its world, or null for a null inventory or one without a world. */
	static URockInventoryClientModel* GetModel(URockInventory* Inventory);

	URockInventoryClientModel* FindOrCreateModel(URockInventory* Inventory);
	/** The model for the inventory if one exists. Never creates. */
	URockInventoryClientModel* FindModel(const URockInventory* Inventory) const;

	/** The manager component whose pending commands every model applies on top of replicated data (the local player's), or null. */
	void SetPredictionSource(URockInventoryManagerComponent* Source) { PredictionSource = Source; }
	URockInventoryManagerComponent* GetPredictionSource() const { return PredictionSource.Get(); }
	/** Lets the prediction source apply its pending commands to Data, the replicated state of Inventory. No source: nothing changes. */
	void ApplyPrediction(const URockInventory& Inventory, FRockInventoryData& Data) const;

private:
	TWeakObjectPtr<URockInventoryManagerComponent> PredictionSource;

	UPROPERTY(Transient)
	TMap<TObjectPtr<URockInventory>, TObjectPtr<URockInventoryClientModel>> Models;
};
