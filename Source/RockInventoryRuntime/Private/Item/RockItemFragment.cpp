// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemFragment.h"

#include "UObject/AssetRegistryTagsContext.h"

void FRockItemFragment::OnItemCreated(FRockItemStack& ItemStack) const
{
}

bool FRockItemFragment::CanCombineItemStack(const FRockItemStack& ItemStack, const FRockItemStack& OtherItemStack) const
{
	return true;
}

void FRockItemFragment::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
}


#if WITH_EDITOR
inline EDataValidationResult FRockItemFragment::IsDataValid(FDataValidationContext& Context) const
{
	return EDataValidationResult::Valid;
}
#endif // WITH_EDITOR
