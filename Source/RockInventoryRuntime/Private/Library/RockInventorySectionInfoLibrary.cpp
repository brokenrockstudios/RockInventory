// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Library/RockInventorySectionInfoLibrary.h"

#include "Inventory/RockInventorySectionInfo.h"


FGameplayTag URockInventorySectionInfoLibrary::GetSectionTag(const FRockInventorySectionInfo& SectionInfo)
{
	return SectionInfo.GetSectionTag();
}
