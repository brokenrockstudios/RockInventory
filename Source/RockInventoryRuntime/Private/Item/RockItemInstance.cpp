// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemInstance.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "Item/RockItemDefinition.h"
#include "Item/State/RockItemState_Metadata.h"
#include "Item/State/RockItemState_NestedInventory.h"
#include "Library/RockInventoryLibrary.h"
#include "Net/UnrealNetwork.h"


void URockItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockItemInstance, OwningInventory);
	DOREPLIFETIME(URockItemInstance, ItemHandle);
	DOREPLIFETIME(URockItemInstance, States);
}

void URockItemInstance::SetDefinition(const TObjectPtr<URockItemDefinition>& object)
{
	CachedDefinition = object;

	// Give every fragment a chance to add whatever runtime state(s) it requires (e.g. nested inventory, durability).
	for (const FInstancedStruct& Fragment : CachedDefinition->GetAllFragments())
	{
		if (const FRockItemFragment* FragmentPtr = Fragment.GetPtr<FRockItemFragment>())
		{
			FragmentPtr->OnInstanceCreated(this);
		}
	}
}

URockInventory* URockItemInstance::GetNestedInventory() const
{
	if (const FRockItemState_NestedInventory* State = FindState<FRockItemState_NestedInventory>())
	{
		return State->NestedInventory;
	}
	return nullptr;
}

const URockItemDefinition* URockItemInstance::GetItemDefinition() const
{
	return CachedDefinition;
}

bool URockItemInstance::IsSupportedForNetworking() const
{
	return true;
}

void URockItemInstance::PostInitProperties()
{
	Super::PostInitProperties();
	RegisterStatTagsListener();
}

void URockItemInstance::OnRep_States()
{
	RegisterStatTagsListener();

	// States without their own per-element replication callback (e.g. plain FGameplayTagContainer
	// Tags, unlike the FastArraySerializer-backed StatTags) rely on this whole-array OnRep to notify
	// that something changed. StatTags will end up notifying twice (once here, once via its own
	// FastArraySerializer callback), which is harmless since BroadcastItemChanged is idempotent-ish UI refresh.
	NotifyStateChanged();
}

void URockItemInstance::RegisterStatTagsListener()
{
	// TODO: This should happen somewhere else.
	if (FRockItemState_Metadata* itemMetadata = FindMutableState<FRockItemState_Metadata>())
	{
		itemMetadata->OnStateAdded(this);
	}
}

void URockItemInstance::BeginDestroy()
{
	if (OwningInventory)
	{
		// The owning actor may already be gone (or unresolvable) by the time we're destroyed
		if (AActor* OwningActor = OwningInventory->GetOwningActor())
		{
			OwningActor->RemoveReplicatedSubObject(this);
		}
		OwningInventory = nullptr;
	}
	CachedDefinition = nullptr;

	Super::BeginDestroy();
}

void URockItemInstance::NotifyStateChanged()
{
	if (IsValid(OwningInventory))
	{
		// Notify the owning inventory that this item instance has changed, so it can update any relevant UI or gameplay logic.
		OwningInventory->BroadcastItemChanged(ItemHandle, ERockItemChangeType::Changed);
	}
}

void URockItemInstance::SetOwningInventory(URockInventory* InOwningInventory)
{
	if (OwningInventory == InOwningInventory)
	{
		return;
	}

	if (OwningInventory)
	{
		// Unregister from the old inventory
		UnregisterReplicationWithOwner();
	}
	OwningInventory = InOwningInventory;
	// A nested inventory follows its item: its owner is the inventory holding the item (null while the item is in the world).
	// Register below walks Owner to find the replication owner, so this must happen before it.
	if (URockInventory* Nested = GetNestedInventory())
	{
		Nested->SetOwner(InOwningInventory);
	}

	RegisterReplicationWithOwner();
}

void URockItemInstance::RegisterReplicationWithOwner()
{
	// Gated like the inventory the item is in: the same viewers see the instance (an instance in a world item has the owner only)
	if (!RockInventoryReplication::RegisterSubObject(URockInventoryLibrary::GetTopLevelOwner(this), this, OwningInventory))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("URockItemInstance::RegisterReplicationWithOwner: OwningActor is null"));
		return;
	}
	if (URockInventory* Nested = GetNestedInventory())
	{
		Nested->RegisterReplicationWithOwner();
	}
}

void URockItemInstance::UnregisterReplicationWithOwner()
{
	RockInventoryReplication::UnregisterSubObject(URockInventoryLibrary::GetTopLevelOwner(this), this);
	if (URockInventory* Nested = GetNestedInventory())
	{
		Nested->UnregisterReplicationWithOwner();
	}
}

URockInventory* URockItemInstance::GetOwningInventory() const
{
	return OwningInventory.Get();
}

FRockItemStack URockItemInstance::GetItemStack() const
{
	const URockInventory* Inventory = GetOwningInventory();
	// TODO: An instance on a world item has no owning inventory, so we can't resolve its stack this way yet.
	// Surface it loudly in dev builds until that case is handled, but fail safe with an invalid stack.
	if (!ensureMsgf(Inventory, TEXT("URockItemInstance::GetItemStack - %s has no owning inventory (possibly on a world item). Returning an invalid stack."), *GetName()))
	{
		return FRockItemStack::Invalid();
	}
	return Inventory->GetItemByHandle(ItemHandle);
}

#if UE_WITH_IRIS
void URockItemInstance::RegisterReplicationFragments(
	UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS
