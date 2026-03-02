// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemInstance.h"

#include "RockInventoryLogging.h"
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockInventoryLibrary.h"
#include "Net/UnrealNetwork.h"


void URockItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockItemInstance, OwningInventory);
	DOREPLIFETIME(URockItemInstance, SlotHandle);
	DOREPLIFETIME(URockItemInstance, ItemHandle);
	DOREPLIFETIME(URockItemInstance, CachedDefinition);
	DOREPLIFETIME(URockItemInstance, StatTags);
	DOREPLIFETIME(URockItemInstance, NestedInventory);
}


void URockItemInstance::SetDefinition(const TObjectPtr<URockItemDefinition>& object)
{
	CachedDefinition = object;

	if (!CachedDefinition->InventoryConfig.IsNull())
	{
		if (!NestedInventory)
		{
			NestedInventory = NewObject<URockInventory>(this);
			NestedInventory->Init(CachedDefinition->InventoryConfig.LoadSynchronous());
		}
	}
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
	StatTags.SetListenerObject(this);
}

void URockItemInstance::BeginDestroy()
{
	// Clean up any resources or references
	if (OwningInventory)
	{
		OwningInventory->GetOwningActor()->RemoveReplicatedSubObject(this);
		OwningInventory = nullptr;
	}
	CachedDefinition = nullptr;

	Super::BeginDestroy();
}

int32 URockItemInstance::GetStatTagCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

void URockItemInstance::AddStatTagCount(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks)
{
	int32 oldCount = StatTags.GetStackCount(Tag);
	StatTags.AddStack(Tag, StackCount, bKeepZeroStacks);
	int32 newCount = StatTags.GetStackCount(Tag);
	OnTagStackChanged_Internal(Tag, oldCount + StackCount, oldCount);
}

void URockItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks)
{
	int32 oldCount = StatTags.GetStackCount(Tag);
	StatTags.RemoveStack(Tag, StackCount, bKeepZeroStacks);
	int32 newCount = StatTags.GetStackCount(Tag);
	OnTagStackChanged_Internal(Tag, newCount, oldCount);
}


void URockItemInstance::SetStatTagCount(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks)
{
	int32 oldCount = StatTags.GetStackCount(Tag);
	StatTags.SetStack(Tag, StackCount, bKeepZeroStacks);
	
	OnTagStackChanged_Internal(Tag, StackCount, oldCount);
}

void URockItemInstance::OnTagStackChanged_Internal(const FGameplayTag& Tag, int32 NewCount, int32 OldCount)
{
	if (IsValid(OwningInventory))
	{
		// Broadcast the item changed event if the count actually changed. This prevents unnecessary broadcasts when tags are added/removed but the stack count doesn't change (e.g., when bKeepZeroStacks is true).
		if (NewCount != OldCount)
		{
			// Notify the owning inventory that this item instance has changed, so it can update any relevant UI or gameplay logic.
			OwningInventory->BroadcastItemChanged(ItemHandle, ERockItemChangeType::Changed);
		}
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

	RegisterReplicationWithOwner();
}

void URockItemInstance::RegisterReplicationWithOwner()
{
	UObject* topLevelOwner = URockInventoryLibrary::GetTopLevelOwner(this);
	if (UActorComponent* Component = Cast<UActorComponent>(topLevelOwner))
	{
		Component->AddReplicatedSubObject(this);
	}
	else if (AActor* actor = Cast<AActor>(topLevelOwner))
	{
		actor->AddReplicatedSubObject(this);
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("URockItemInstance::RegisterReplicationWithOwner: OwningActor is null"));
		return;
	}
	if (NestedInventory)
	{
		NestedInventory->RegisterReplicationWithOwner();
	}
}

void URockItemInstance::UnregisterReplicationWithOwner()
{
	UObject* topLevelOwner = URockInventoryLibrary::GetTopLevelOwner(this);
	if (UActorComponent* Component = Cast<UActorComponent>(topLevelOwner))
	{
		Component->RemoveReplicatedSubObject(this);
	}
	else if (AActor* actor = Cast<AActor>(topLevelOwner))
	{
		actor->RemoveReplicatedSubObject(this);
	}
	if (NestedInventory)
	{
		NestedInventory->UnregisterReplicationWithOwner();
	}
}

URockInventory* URockItemInstance::GetOwningInventory() const
{
	return OwningInventory.Get();
}

void URockItemInstance::SetSlotHandle(FRockInventorySlotHandle InSlotHandle)
{
	SlotHandle = InSlotHandle;
}

FRockInventorySlotEntry URockItemInstance::GetItemSlot() const
{
	if (const URockInventory* Inventory = GetOwningInventory())
	{
		return Inventory->GetSlotByHandle(GetSlotHandle());
	}
	return FRockInventorySlotEntry();
}

FRockItemStack URockItemInstance::GetItemStack() const
{
	return GetOwningInventory()->GetItemBySlotHandle(GetSlotHandle());
}


#if UE_WITH_IRIS
void URockItemInstance::RegisterReplicationFragments(
	UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS
