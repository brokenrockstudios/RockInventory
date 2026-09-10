// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/Fragment/RockItemFragment_NestedInventory.h"

#include "Inventory/RockInventory.h"
#include "Item/RockItemInstance.h"
#include "Item/State/RockItemState_NestedInventory.h"

void FRockItemFragment_NestedInventory::OnInstanceCreated(URockItemInstance* ItemInstance) const
{
	if (InventoryConfig.IsNull())
	{
		return;
	}

	// Only initialize on the first add; a replicated instance may already have this state populated.
	if (ItemInstance->HasState<FRockItemState_NestedInventory>())
	{
		return;
	}

	FRockItemState_NestedInventory& NestedState = ItemInstance->FindOrAddState<FRockItemState_NestedInventory>();
	if (NestedState.NestedInventory)
	{
		NestedState.NestedInventory->Init(InventoryConfig.LoadSynchronous());
	}
}
