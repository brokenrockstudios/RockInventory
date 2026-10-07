// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Replication/RockInventoryReplication.h"
#include "RockInventoryConfig.generated.h"

struct FRockInventorySectionInfo;

/** What a loot setting check found wrong in a config. All of them are warnings: the inventory still works, loot just cannot use it as intended. */
enum class ERockConfigIssue : uint8
{
	/** No section with slots accepts the Store intent, so a plain pickup (Store) can never place anything. */
	NoStorageSection,
	/** Two sections share a section tag, so lookups by tag and loot placements are ambiguous. */
	DuplicateSectionTag,
	/** A section's LootPreference can never match an item its SectionFilter lets in, so the preference never has an effect. */
	UnreachableLootPreference,
};

struct ROCKINVENTORYRUNTIME_API FRockConfigIssue
{
	ERockConfigIssue Kind = ERockConfigIssue::NoStorageSection;
	/** Index into the checked section array, or INDEX_NONE for a whole-config issue. */
	int32 SectionIndex = INDEX_NONE;
	FText Message;
};

/**
 *
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Define the height/width and tabID for each tab.
	// A character might have 8 tabs. 7 tabs of 1x1 for character slots (head, armor, gun, etc.) and 1 large of 8x5 for general inventory
	// For example, a Backpack might have a single tab of 4x5
	// A chest rig, might have 4 tabs of 1x2
	// Note: that the TabIndex will equal the order they are defined here.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FRockInventorySectionInfo> InventoryTabs;

	/**
	 * Who sees this inventory's contents when it is an item's nested inventory, and what kind of container it is. Separate: a container that
	 * has to be opened (a backpack's contents). FollowsParent: seen and accessed with the inventory holding the item (a weapon's attachments).
	 * OwnerOnly: only the player it sits on ever receives or may touch the contents (a secure container); also valid on a top-level inventory.
	 * Copied to URockInventory::NestedVisibility by Init.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	ERockNestedVisibility Visibility = ERockNestedVisibility::Separate;

	// TODO:
	// Consider having a 'parent' config' or even an 'array' of composable configs?

	/**
	 * Checks the loot settings of a section list (intents, section tags, preference versus filter) without needing an asset.
	 * IsDataValid reports these as warnings. A preference is checked by trying every combination of the tags the filter and the
	 * preference mention (skipped, no issue reported, when they mention more than MaxTagsForPreferenceCheck distinct tags).
	 */
	static void CollectLootIssues(const TArray<FRockInventorySectionInfo>& Sections, TArray<FRockConfigIssue>& OutIssues);

	static constexpr int32 MaxTagsForPreferenceCheck = 10;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
