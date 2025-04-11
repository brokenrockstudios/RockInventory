// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemInstance.h"
#include "Library/RockItemInstanceLibrary.h"
#include "Item/RockItemDefinition.h"

void URockItemInstance::Init(URockInventory* InOwningInventory, URockItemDefinition* InDefinition, const FRockInventorySlotHandle& InSlotHandle)
{
	SetOwningInventory(InOwningInventory);
	SetSlotHandle(InSlotHandle);
	CachedDefinition = InDefinition;
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

	// Initialize any default values or setup here
}

void URockItemInstance::BeginDestroy()
{
	// Clean up any resources or references
	OwningInventory = nullptr;
	CachedDefinition = nullptr;
	
	Super::BeginDestroy();
}

bool URockItemInstance::FindItemStackForThisInstance(FRockItemStack& OutItemStack) const
{
	return URockItemInstanceLibrary::FindItemStackForInstance(this, OutItemStack);
}

bool URockItemInstance::FindItemSlotForThisInstance(FRockInventorySlot& OutItemSlot) const
{
	return URockItemInstanceLibrary::FindItemSlotForInstance(this, OutItemSlot);
}

void URockItemInstance::SetOwningInventory(URockInventory* InOwningInventory)
{
	OwningInventory = InOwningInventory;
}

void URockItemInstance::SetSlotHandle(FRockInventorySlotHandle InSlotHandle)
{
	SlotHandle = InSlotHandle;
}

FRockInventorySlot URockItemInstance::GetItemSlot() const
{
	if (const URockInventory* Inventory = GetOwningInventory())
	{
		return Inventory->GetSlotByHandle(GetSlotHandle());
	}
	return FRockInventorySlot();
}

FRockItemStack URockItemInstance::GetItemStack() const
{
	const FRockInventorySlot& Slot = GetItemSlot();
	if (Slot.IsValid())
	{
		return Slot.Item;
	}
	return nullptr;
}

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
