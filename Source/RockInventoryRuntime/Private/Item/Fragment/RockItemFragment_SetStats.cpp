#include "Item/Fragment/RockItemFragment_SetStats.h"

#include "RockInventoryLogging.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Item/State/RockItemState_Metadata.h"

void FRockItemFragment_SetStats::OnInstanceCreated(URockItemInstance* ItemInstance) const
{
	if (IsValid(ItemInstance) && !InitialItemStats.IsEmpty())
	{
		ItemInstance->FindOrAddState<FRockItemState_Metadata>();
	}
}

void FRockItemFragment_SetStats::OnItemCreated(FRockItemStack& ItemStack) const
{
	URockItemDefinition* Def = ItemStack.GetDefinition();
	if (Def)
	{
		ensureMsgf(CustomValue1 == 0 || Def->CustomValue1Tag.IsValid(), TEXT("SetStats: CustomValue1 set but definition has no CustomValue1Tag on %s"), *Def->GetName());
		ensureMsgf(CustomValue2 == 0 || Def->CustomValue2Tag.IsValid(), TEXT("SetStats: CustomValue2 set but definition has no CustomValue2Tag on %s"), *Def->GetName());

		ItemStack.SetCustomValue1(CustomValue1, {});
		ItemStack.SetCustomValue2(CustomValue2, {});
	}

	if (!InitialItemStats.IsEmpty())
	{
		URockItemInstance* ItemInstance = ItemStack.GetRuntimeInstance();
		if (IsValid(ItemInstance))
		{
			FRockItemState_Metadata* metadata = ItemInstance->FindMutableState<FRockItemState_Metadata>();
			checkf(metadata, TEXT("SetStats: ItemInstance has no metadata state on %s"), *Def->GetName());
			
			for (const auto& KVP : InitialItemStats)
			{
				metadata->AddStatTagCount(KVP.Key, KVP.Value);
			}
		}
	}
}
