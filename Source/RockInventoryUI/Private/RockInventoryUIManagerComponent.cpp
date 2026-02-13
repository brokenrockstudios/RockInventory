// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "RockInventoryUIManagerComponent.h"

#include "UI/Spatial/RockInventory_InventoryBase.h"


URockInventory_HoverItem* URockInventoryUIManagerComponent::GetHoverItem() const
{
	if (InventoryMenu)
	{
		return InventoryMenu->GetHoverItem();
	}
	return nullptr;
}
