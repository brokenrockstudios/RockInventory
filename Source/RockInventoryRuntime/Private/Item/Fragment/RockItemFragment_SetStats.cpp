#include "Item/Fragment/RockItemFragment_SetStats.h"

#include "Item/RockItemInstance.h"

void FRockItemFragment_SetStats::OnItemCreated(URockItemInstance* ItemInstance) const
{
	if (!IsValid(ItemInstance))
	{
		return;
	}

	for (const auto& KVP : InitialItemStats)
	{
		ItemInstance->StatTags.AddStack(KVP.Key, KVP.Value);
	}
}
