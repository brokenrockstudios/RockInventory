// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockSlotHandle.h"
#include "Enums/RockItemSizePolicy.h"
#include "Enums/RockLootIntent.h"
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

	/** Sets the hard item filter (see SectionFilter). Returns *this so a section can be built in one expression, mainly for tests and code-built configs. */
	FRockInventorySectionInfo& SetSectionFilter(const FGameplayTagQuery& InSectionFilter);

	/** Sets the meta tags (see MetaTags). Returns *this, like SetSectionFilter. */
	FRockInventorySectionInfo& SetMetaTags(const FGameplayTagContainer& InMetaTags);

	/** Sets which loot intents may use this section (see AcceptedLootIntents). Returns *this, like SetSectionFilter. */
	FRockInventorySectionInfo& SetAcceptedLootIntents(ERockLootIntent InIntents);

	/** Sets the loot order rank (see LootPriority). Lower numbers are tried first (-1 before 0 before 10). Returns *this. */
	FRockInventorySectionInfo& SetLootPriority(int32 InPriority);

	/** Sets the soft loot preference (see LootPreference), e.g. FGameplayTagQuery::MakeQuery_MatchAnyTags(SidearmTags). Returns *this. */
	FRockInventorySectionInfo& SetLootPreference(const FGameplayTagQuery& InPreference);

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

	/** Which loot calls may place items here (ERockLootIntent flags). A call is allowed in when its intent shares a bit with this.
	 * Storage sections keep the default (Store), equipment sections accept Equip, a quick slot both, and a section nothing should auto-fill (an oven input) none.
	 * Dragging an item onto a chosen slot is not loot and ignores this.
	 */
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/RockInventoryRuntime.ERockLootIntent"))
	int32 AcceptedLootIntents = static_cast<int32>(ERockLootIntent::Store);

	/** Loot order among eligible sections: lower numbers are tried first (-1 before 0 before 10), ties keep config order. It only orders; it never makes a section eligible. */
	UPROPERTY(EditAnywhere)
	int32 LootPriority = 0;

	/** Sections whose LootPreference matches an item are tried before those that merely allow it. Empty means no preference.
	 * It only reorders: unlike SectionFilter it never excludes an item. e.g. Secondary prefers Sidearm, Primary prefers Weapon and not Sidearm, both filters allow Weapon.
	 */
	UPROPERTY(EditAnywhere)
	FGameplayTagQuery LootPreference;

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

	/** ERockLootIntent flags this section accepts */
	int32 GetAcceptedLootIntents() const;

	/** True when a loot call with these ERockLootIntent flags may use this section */
	bool AcceptsLootIntent(int32 CallIntents) const;

	int32 GetLootPriority() const;

	const FGameplayTagQuery& GetLootPreference() const;

	// Returns invalid section info
	static const FRockInventorySectionInfo& Invalid();
};
