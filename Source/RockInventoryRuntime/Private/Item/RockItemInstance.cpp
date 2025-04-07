// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Item/RockItemInstance.h"

#include "Library/RockItemInstanceLibrary.h"

const URockItemDefinition* URockItemInstance::GetItemDefinition() const
{
	if (!CachedDefinition){
		// update CachedDefinition
	}
	
	return CachedDefinition;
}

bool URockItemInstance::IsSupportedForNetworking() const
{
	return true;
}

bool URockItemInstance::FindItemStackForThisInstance(FRockItemStack& OutItemStack) const
{
	return URockItemInstanceLibrary::FindItemStackForInstance(this, OutItemStack);
}

bool URockItemInstance::FindItemSlotForThisInstance(FRockInventorySlot& OutItemStack) const
{
	return URockItemInstanceLibrary::FindItemSlotForInstance(this, OutItemStack);
}

void URockItemInstance::SetSlotHandle(FRockSlotHandle InSlotHandle)
{
	SlotHandle = InSlotHandle;
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
