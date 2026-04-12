#include "Item/Fragment/RockItemFragment_SetStats.h"

#include "Item/RockItemInstance.h"

void FRockItemFragment_SetStats::OnItemCreated(FRockItemStack& ItemStack) const
{
	URockItemDefinition* Def = ItemStack.GetDefinition();
	if (Def)
	{
		ensureMsgf(CustomValue1 == 0 || Def->CustomValue1Tag.IsValid(), TEXT("SetStats: CustomValue1 set but definition has no CustomValue1Tag on %s"), *Def->GetName());
		ensureMsgf(CustomValue2 == 0 || Def->CustomValue2Tag.IsValid(), TEXT("SetStats: CustomValue2 set but definition has no CustomValue2Tag on %s"), *Def->GetName());

		ItemStack.CustomValue1 = CustomValue1;
		ItemStack.CustomValue2 = CustomValue2;
	}

	URockItemInstance* ItemInstance = ItemStack.GetRuntimeInstance();
	if (IsValid(ItemInstance))
	{
		for (const auto& KVP : InitialItemStats)
		{
			ItemInstance->StatTags.AddStack(KVP.Key, KVP.Value);
		}
	}
}
