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

FGameplayTag FRockInventorySectionInfo::GetSectionTag() const
{
	return SectionTag;
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

const FGameplayTagQuery& FRockInventorySectionInfo::GetSectionFilter() const
{
	return SectionFilter;
}

const FGameplayTagContainer& FRockInventorySectionInfo::GetMetaTags() const
{
	return MetaTags;
}

int32 FRockInventorySectionInfo::GetSectionIndex() const
{
	return SectionIndex;
}

bool FRockInventorySectionInfo::ContainsSlotHandle(FRockInventorySlotHandle InSlotHandle) const
{
	if (FirstSlotIndex == INDEX_NONE) { return false; }
	const int32 AbsoluteIndex = InSlotHandle.GetAbsoluteIndex();
	return AbsoluteIndex >= FirstSlotIndex && AbsoluteIndex < FirstSlotIndex + GetNumSlots();
}

void FRockInventorySectionInfo::Initialize(int32 InFirstSlotIndex, int32 InSectionIndex)
{
	ensureMsgf(Columns > 0 && Rows > 0, TEXT("SectionInfo '%s' has invalid dimensions (%dx%d)"), *SectionTag.ToString(), Columns, Rows);

	FirstSlotIndex = InFirstSlotIndex;
	SectionIndex = InSectionIndex;
}

bool FRockInventorySectionInfo::IsValid() const
{
	return Columns > 0 && Rows > 0 && FirstSlotIndex >= 0 && SectionTag.IsValid();
}

const FRockInventorySectionInfo& FRockInventorySectionInfo::Invalid()
{
	static FRockInventorySectionInfo InvalidSection;
	return InvalidSection;
}
