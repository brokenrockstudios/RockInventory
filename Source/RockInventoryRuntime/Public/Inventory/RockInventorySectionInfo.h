// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockSlotHandle.h"
#include "Enums/RockItemSizePolicy.h"
#include "UObject/Object.h"

#include "RockInventorySectionInfo.generated.h"


/**
 * Section dimension info
 * This struct is used to define the dimensions of a section in a single inventory system.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySectionInfo
{
	GENERATED_BODY()

	FRockInventorySectionInfo() = default;

	FRockInventorySectionInfo(
		FGameplayTag InSectionTag, int32 InFirstSlotIndex, int32 InColumns, int32 InRows,
		ERockItemSizePolicy InSlotSizePolicy = ERockItemSizePolicy::RespectSize)
		: SectionTag(InSectionTag), FirstSlotIndex(InFirstSlotIndex), Columns(InColumns), Rows(InRows), SlotSizePolicy(InSlotSizePolicy)
	{
	}

	bool ContainsSlotHandle(FRockInventorySlotHandle InSlotHandle) const;

private:
	UPROPERTY(EditAnywhere, meta = (Categories = "Inventory.Section,Inventory.Group"))
	FGameplayTag SectionTag;

	// Index of this section in the inventory's Sections array
	UPROPERTY()
	int32 SectionIndex = INDEX_NONE;

	/** First slot index in the AllSlots array */
	UPROPERTY()
	int32 FirstSlotIndex = INDEX_NONE;

	/** Grid width (Columns) in slots */
	UPROPERTY(EditAnywhere)
	int32 Columns = 0;

	/** Grid height (Rows) in slots */
	UPROPERTY(EditAnywhere)
	int32 Rows = 0;

	/** Type of section - determines special behavior like size restrictions */
	UPROPERTY(EditAnywhere)
	ERockItemSizePolicy SlotSizePolicy = ERockItemSizePolicy::RespectSize;

	// Can be used for a variety of purposes, such as categorizing the section. e.g. For 'AutoEquip' slots, other behaviors, or characteristics about the section.
	// An equipment manager might look for all sections with the "AutoEquip" tag to automatically equip items when items are added.
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer MetaTags;

	/** Optional tags to filter items in this section.
	 * e.g., a Head Slot only accepts hat items, weapons only accept weapons, or a keychain only accepts keys. See ItemDefinition comment for more info
	 */
	UPROPERTY(EditAnywhere)
	FGameplayTagQuery SectionFilter;

public:
	void Initialize(int32 InFirstSlotIndex, int32 InSectionIndex);

	bool IsValid() const;

	/** Total number of slots in this section */
	int32 GetNumSlots() const;

	/** Returns the width (Columns) of the section */
	int32 GetColumns() const;

	/** Returns the height (Rows) of the section */
	int32 GetRows() const;

	/** Returns the tag of the section */
	FGameplayTag GetSectionTag() const;

	/** Returns the index of this section in the inventory's Sections array */
	int32 GetSectionIndex() const;

	/** Returns the first slot index in the AllSlots array */
	int32 GetFirstSlotIndex() const;

	/** Converts an absolute slot index to a relative index within this section */
	int32 GetLocalIndex(int32 AbsoluteIndex) const;

	/** Returns the size policy for this section */
	ERockItemSizePolicy GetSlotSizePolicy() const;

	/** Returns the tag query filter for this section */
	const FGameplayTagQuery& GetSectionFilter() const;

	/** Returns the meta-tags associated with this section */
	const FGameplayTagContainer& GetMetaTags() const;

	// Returns invalid section info
	static const FRockInventorySectionInfo& Invalid();
};
