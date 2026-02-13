// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Library/RockInventorySectionInfoLibrary.h"

#include "Inventory/RockInventorySectionInfo.h"


FName URockInventorySectionInfoLibrary::GetSectionName(const FRockInventorySectionInfo& SectionInfo)
{
	return SectionInfo.GetSectionName();
}
