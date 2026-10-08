// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "Client/RockInventoryClientModel.h"

namespace RockInventoryUI
{
	/**
	 * The client model of an inventory (T-77). Widgets read inventory state through it and listen to its OnChanged; the inventory
	 * itself stays the key for commands (moves go to the server through the manager component) and for identity.
	 */
	inline URockInventoryClientModel* ModelOf(URockInventory* Inventory)
	{
		return URockInventoryClientModelSubsystem::GetModel(Inventory);
	}
}
