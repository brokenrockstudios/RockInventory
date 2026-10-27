// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Enums/RockItemSizePolicy.h"
#include "UObject/Object.h"

#include "RockInventorySectionInfo.generated.h"


/**
 * Tab dimension info
 * This struct is used to define the dimensions of a tab or collection in a single inventory system.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySectionInfo
{
	GENERATED_BODY()

	FRockInventorySectionInfo() = default;

	FRockInventorySectionInfo(
		FName InSectionName, int32 InFirstSlotIndex, int32 InColumns, int32 InRows,
		ERockItemSizePolicy InSlotSizePolicy = ERockItemSizePolicy::RespectSize)
		: SectionName(InSectionName), FirstSlotIndex(InFirstSlotIndex), Columns(InColumns), Rows(InRows), SlotSizePolicy(InSlotSizePolicy)
	{
	}

	bool operator==(const FRockInventorySectionInfo& Other) const
	{
		return SectionName == Other.SectionName
			&& SectionIndex == Other.SectionIndex
			&& FirstSlotIndex == Other.FirstSlotIndex
			&& Columns == Other.Columns
			&& Rows == Other.Rows
			&& SlotSizePolicy == Other.SlotSizePolicy
			&& Tags == Other.Tags
			&& SectionFilter == Other.SectionFilter;
	}

private:
	/** Unique identifier for this tab */
	UPROPERTY(EditAnywhere)
	FName SectionName;

	// Index of this section in the inventory's Sections array
	UPROPERTY()
	int32 SectionIndex = INDEX_NONE;

	/** First slot index in the AllSlots array */
	UPROPERTY()
	int32 FirstSlotIndex = 0;

	// TODO: Switch over to Columns/Rows terminology instead of Width/Height?
	/** Grid width (Columns) in slots */
	UPROPERTY(EditAnywhere)
	int32 Columns = 0;

	/** Grid height (Rows) in slots */
	UPROPERTY(EditAnywhere)
	int32 Rows = 0;

	/** Type of section - determines special behavior like size restrictions */
	UPROPERTY(EditAnywhere)
	ERockItemSizePolicy SlotSizePolicy = ERockItemSizePolicy::RespectSize;

	// Can be used for a variety of purposes, such as categorizing the section. e.g. For 'AutoEquip' slots.
	// An equipment manager might look for all sections with the "AutoEquip" tag to automatically equip items when items are added.
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer Tags;

	/** Optional tags to filter items in this tab.
	 * e.g. a Head Slot only accepts hat items, or weapons only accept weapons, Keychain only accepts keys. 
	 */
	UPROPERTY(EditAnywhere)
	FGameplayTagQuery SectionFilter;

public:
	void Initialize(int32 InFirstSlotIndex, int32 InSectionIndex);

	bool IsValid() const;

	/** Total number of slots in this tab */
	int32 GetNumSlots() const;

	/** Returns the width (Columns) of the tab */
	int32 GetColumns() const;

	/** Returns the height (Rows) of the tab */
	int32 GetRows() const;

	/** Returns the name of the tab */
	FName GetSectionName() const;

	/** Returns the index of this section in the inventory's Sections array */
	int32 GetSectionIndex() const;

	/** Returns the first slot index in the AllSlots array */
	int32 GetFirstSlotIndex() const;
	
	/** Converts an absolute slot index to a relative index within this section */
	int32 GetLocalIndex(int32 AbsoluteIndex) const;
	
	/** Returns the size policy for this section */
	ERockItemSizePolicy GetSlotSizePolicy() const;

	/** Returns the tag query filter for this section */
	FGameplayTagQuery GetSectionFilter() const;

	/** Returns the tags associated with this section */
	FGameplayTagContainer GetTags() const;

	// Returns an invalid section info
	static FRockInventorySectionInfo Invalid();
};
