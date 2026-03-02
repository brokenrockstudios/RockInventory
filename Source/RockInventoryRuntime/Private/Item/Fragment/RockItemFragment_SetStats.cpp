#include "Item/Fragment/RockItemFragment_SetStats.h"

#include "Item/RockItemInstance.h"

void FRockItemFragment_SetStats::OnItemCreated(FRockItemStack& ItemStack) const
{
	URockItemInstance* ItemInstance = ItemStack.GetRuntimeInstance();
	if (IsValid(ItemInstance))
	{
		for (const auto& KVP : InitialItemStats)
		{
			ItemInstance->StatTags.AddStack(KVP.Key, KVP.Value);
		}
	}
	
	ItemStack.CustomValue1 = CustomValue1;
	ItemStack.CustomValue2 = CustomValue2;
}
