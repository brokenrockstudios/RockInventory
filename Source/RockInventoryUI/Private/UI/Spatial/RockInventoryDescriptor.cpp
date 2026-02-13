#include "UI/ItemDescription/RockInventoryDescriptor.h"

#include "Rarity/RockRarityData.h"

// FRockExtractorRegistry& FRockExtractorRegistry::Get()
// {
// }
//
// void FRockExtractorRegistry::Register(TUniquePtr<IRockFragmentExtractor> Extractor)
// {
// }
//
// const TArray<const IRockFragmentExtractor*>& FRockExtractorRegistry::Find(UScriptStruct* FragType) const
// {
// }

FRockItemDescriptorBuilder& FRockItemDescriptorBuilder::Get()
{
	static FRockItemDescriptorBuilder Instance;
	return Instance;
}

bool FRockItemDescriptorBuilder::Build(const FRockItemStack& Item, FRockItemDescriptor& OutDesc) const
{
	if (!Item.IsValid())
	{
		return false;
	}
	const URockItemDefinition* Def = Item.GetDefinition();
	OutDesc.ItemStackCopy = Item;

	// 1) Set core fields
	OutDesc.ItemId = Def->ItemId;
	OutDesc.DisplayName = Def->DisplayName;
	OutDesc.Description = Def->Description;

	OutDesc.Icon = Def->IconData;
	OutDesc.Rarity = Def->ItemRarity;	

	OutDesc.ItemTags = Def->ItemTags;
	OutDesc.StackCount = Item.GetStackCount();

	// TODO: Build from the instances and build out the floatstats, Notes, and other Maps
	// 2) Real Fragments (fill in everything)

	return true;
}

// void FRockItemDescriptorBuilder::RegisterExtractor(TUniquePtr<class IRockFragmentExtractor> Extractor) { }
