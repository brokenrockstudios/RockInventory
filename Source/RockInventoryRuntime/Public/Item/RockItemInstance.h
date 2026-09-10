// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockItemStack.h"
#include "RockItemState.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "RockItemInstance.generated.h"

class URockInventory;
struct FRockItemState_Metadata;

/**
 * Base class for all item instances in the Rock Inventory system.
 * Item instances represent runtime objects that can have state and behavior associated with them.
 * They are created when an item definition requires runtime instantiation.
 * 
 * Item instances maintain a reference to their owning inventory and slot, allowing them to access
 * and modify their associated item stack data.
 */
UCLASS(BlueprintType)
class ROCKINVENTORYRUNTIME_API URockItemInstance : public UObject
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetDefinition(const TObjectPtr<URockItemDefinition>& object);

	// -- Core Properties ----------------------------------------------------

	/** The inventory that currently owns this item instance */
	// Note: This might be null, in the case of a 'world item' that is not currently in an inventory.
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	TObjectPtr<URockInventory> OwningInventory = nullptr;
	// TODO: Replicated Owner since UObject's Owner doesn't replicate 

	// --- Replicated ---
	/** Item Handle */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	FRockItemStackHandle ItemHandle;

	// TODO: Consider swapping to FastArraySerializer later.
	/** Replicated runtime state fragments (e.g. metadata tags, nested inventory, durability) */
	UPROPERTY(ReplicatedUsing = OnRep_States, EditAnywhere, BlueprintReadOnly, Category = "RockInventory|State", meta = (BaseStruct = "/Script/RockInventoryRuntime.RockItemState"))
	TArray<FInstancedStruct> States;

	// --- Not Replicated ---
	/** Cached reference to the item definition for quick access */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RockInventory|Core")
	TObjectPtr<URockItemDefinition> CachedDefinition = nullptr;

	///////////////////////////////////////////////////////////////////////////
	// Core Functions

	/** Sets the owning inventory for this item instance */
	void SetOwningInventory(URockInventory* InOwningInventory);
	void RegisterReplicationWithOwner();
	void UnregisterReplicationWithOwner();

	/** Gets the owning inventory for this item instance */
	URockInventory* GetOwningInventory() const;

	/** Gets the item stack associated with this item instance */
	FRockItemStack GetItemStack() const;

	/** Gets the item definition for this item instance */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Core")
	const URockItemDefinition* GetItemDefinition() const;

	/** Gets a nested inventory for this item instance, if any (e.g. backpacks, containers) */
	UFUNCTION(BlueprintCallable, Category = "RockInventory|Core")
	URockInventory* GetNestedInventory() const;

	///////////////////////////////////////////////////////////////////////////
	// State Access

	/** Finds a runtime state of the given type, or nullptr if not present */
	template <typename T> requires std::derived_from<T, FRockItemState>
	const T* FindState() const;

	/** Finds a mutable runtime state of the given type, or nullptr if not present */
	template <typename T> requires std::derived_from<T, FRockItemState>
	T* FindMutableState();

	/** Finds a runtime state of the given type, adding it if not already present */
	template <typename T> requires std::derived_from<T, FRockItemState>
	T& FindOrAddState();

	/** Returns true if a runtime state of the given type is present */
	template <typename T> requires std::derived_from<T, FRockItemState>
	bool HasState() const;

	/** Notifies that a runtime state has changed */
	void NotifyStateChanged();

	UFUNCTION()
	void OnRep_States();
protected:
	///////////////////////////////////////////////////////////////////////////
	// UObject Interface
	virtual bool IsSupportedForNetworking() const override;
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

#if UE_WITH_IRIS
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS
private:
	/** Registers this instance as the listener on the Metadata state's stat tags, if present */
	void RegisterStatTagsListener();
};

template <typename T> requires std::derived_from<T, FRockItemState>
const T* URockItemInstance::FindState() const
{
	for (const FInstancedStruct& State : States)
	{
		if (const T* StatePtr = State.GetPtr<T>())
		{
			return StatePtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FRockItemState>
T* URockItemInstance::FindMutableState()
{
	for (FInstancedStruct& State : States)
	{
		if (T* StatePtr = State.GetMutablePtr<T>())
		{
			return StatePtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FRockItemState>
T& URockItemInstance::FindOrAddState()
{
	if (T* Existing = FindMutableState<T>())
	{
		return *Existing;
	}
	FInstancedStruct& NewState = States.Add_GetRef(FInstancedStruct::Make<T>());
	T* NewStatePtr = NewState.GetMutablePtr<T>();
	NewStatePtr->OnStateAdded(this);
	return *NewStatePtr;
}

template <typename T> requires std::derived_from<T, FRockItemState>
bool URockItemInstance::HasState() const
{
	return FindState<T>() != nullptr;
}
