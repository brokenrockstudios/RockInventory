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
