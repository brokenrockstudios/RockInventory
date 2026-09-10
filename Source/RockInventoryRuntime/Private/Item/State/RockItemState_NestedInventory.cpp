// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/State/RockItemState_NestedInventory.h"

#include "Inventory/RockInventory.h"
#include "Item/RockItemInstance.h"

void FRockItemState_NestedInventory::OnStateAdded(URockItemInstance* OwnerInstance)
{
	FRockItemState::OnStateAdded(OwnerInstance);

	// The owning FRockItemFragment_NestedInventory is responsible for calling Init() with its InventoryConfig.
	NestedInventory = NewObject<URockInventory>(OwnerInstance);
	// Should the owner just be the item instance or something higher up? The instance will be easier to reconcile about. So long as the instance has it's own owner set properly?

	// Who owns the 'nested inventory'? Need to test! UGH multiplayer boo 
	//NestedInventory->Owner = OwnerInstance;
	NestedInventory->Owner = OwnerInstance->GetOwningInventory();

	// Do we need to forward stuff about the nested inventory up to the top level inventory?
	// NestedInventory->OnItemChanged.AddUObject(OwnerInstance, &URockItemInstance::NotifyChange);
	// NestedInventory->OnSlotChanged.AddUObject(OwnerInstance, &URockItemInstance::NotifyChange);
	// Possibly because if a nested inventory changes (e.g. some 'part' on an item breaks, the parent needs to be aware?)
}

void FRockItemState_NestedInventory::OnStateRemoved(URockItemInstance* OwnerInstance)
{
	FRockItemState::OnStateRemoved(OwnerInstance);
	NestedInventory = nullptr;
}
