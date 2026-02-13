// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemFragment.h"

void FRockItemFragment::OnItemCreated(URockItemInstance* ItemInstance) const
{
	// Should we be able to modify the parent ItemStack? (e.g. CustomValue or StackCount?)
	// Perhaps we will refactor only if we need to. Otherwise assuming it's instance data only.
	
}

bool FRockItemFragment::CanCombineItemStack(const FRockItemStack& ItemStack, const FRockItemStack& OtherItemStack) const
{
	return true;
}


#if WITH_EDITOR
inline EDataValidationResult FRockItemFragment::IsDataValid(FDataValidationContext& Context) const
{
	return EDataValidationResult::Valid;
}
#endif // WITH_EDITOR
