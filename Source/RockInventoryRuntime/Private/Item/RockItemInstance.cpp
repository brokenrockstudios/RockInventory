// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemInstance.h"

#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "Item/RockItemDefinition.h"
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
	// DOREPLIFETIME(URockItemInstance, StatStackContainer);
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
	OwningInventory = InOwningInventory;
	// TODO: IMPORTANTE: Do a RemoveSubobject on this and all nested if moved to another inventory or removed
}

void URockItemInstance::RegisterReplicationWithOwner()
{
	if (const URockInventory* Inventory = GetOwningInventory())
	{
		if (AActor* Actor = Inventory->GetOwningActor())
		{
			Actor->AddReplicatedSubObject(this);
			if (NestedInventory)
			{
				NestedInventory->RegisterReplicationWithOwner();
			}
		}
	}
}

void URockItemInstance::UnregisterReplicationWithOwner()
{
	if (const URockInventory* Inventory = GetOwningInventory())
	{
		if (AActor* Actor = Inventory->GetOwningActor())
		{
			Actor->RemoveReplicatedSubObject(this);
			if (NestedInventory)
			{
				NestedInventory->UnregisterReplicationWithOwner();
			}
		}
	}
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

// FRockItemStack URockItemInstance::GetItemStack() const
// {
// 	const FRockInventorySlotEntry& Slot = GetItemSlot();
// 	if (Slot.IsValid())
// 	{
// 		return Slot.Item;
// 	}
// 	return nullptr;
// }

// // In URockItemInstance
// void URockItemInstance::DoSomethingThatNeedsStackData()
// {
// 	if (Owner.IsValid())
// 	{
// 		FRockItemStack MyStackData;
// 		if (Owner->FindItemStackForInstance(this, MyStackData))
// 		{
// 			// Now use MyStackData (e.g., MyStackData.Quantity)
// 		}
//
// 		// Or, if you need to modify (use carefully!)
// 		// FRockItemStack* MyStackPtr = Owner->FindItemStackMutablePtrForInstance(this);
// 		// if (MyStackPtr) { MyStackPtr->SomeValue = ...; Owner->MarkDirtyForReplication(); }
// 	}
// }


#if UE_WITH_IRIS
void URockItemInstance::RegisterReplicationFragments(
	UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS
