// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventorySectionInfo.h"

int32 FRockInventorySectionInfo::GetNumSlots() const
{
	return Columns * Rows;
}

int32 FRockInventorySectionInfo::GetColumns() const
{
	return Columns;
}

int32 FRockInventorySectionInfo::GetRows() const
{
	return Rows;
}

FName FRockInventorySectionInfo::GetSectionName() const
{
	return SectionName;
}

int32 FRockInventorySectionInfo::GetFirstSlotIndex() const
{
	return FirstSlotIndex;
}

int32 FRockInventorySectionInfo::GetLocalIndex(int32 AbsoluteIndex) const
{
	return AbsoluteIndex - FirstSlotIndex;
}

ERockItemSizePolicy FRockInventorySectionInfo::GetSlotSizePolicy() const
{
	return SlotSizePolicy;
}

FGameplayTagQuery FRockInventorySectionInfo::GetSectionFilter() const
{
	return SectionFilter;
}

FGameplayTagContainer FRockInventorySectionInfo::GetTags() const
{
	return Tags;
}

int32 FRockInventorySectionInfo::GetSectionIndex() const
{
	return SectionIndex;
}

void FRockInventorySectionInfo::Initialize(int32 InFirstSlotIndex, int32 InSectionIndex)
{
	FirstSlotIndex = InFirstSlotIndex;
	SectionIndex = InSectionIndex;
}

bool FRockInventorySectionInfo::IsValid() const
{
	return Columns > 0 && Rows > 0 && FirstSlotIndex >= 0 && SectionName != NAME_None;
}

FRockInventorySectionInfo FRockInventorySectionInfo::Invalid()
{
	FRockInventorySectionInfo InvalidSection;
	InvalidSection.SectionName = NAME_None;
	InvalidSection.FirstSlotIndex = -1;
	InvalidSection.Columns = 0;
	InvalidSection.Rows = 0;
	return InvalidSection;
}
