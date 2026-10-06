// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockInventorySectionInfo.h"
#include "Enums/RockLootIntent.h"
#include "Item/RockItemDefinition.h"
#include "Enums/RockItemOrientation.h"
#include "Inventory/InventoryReferenceHelper.h"
#include "Item/RockItemStack.h"
#include "UObject/Object.h"
#include "RockInventoryQuery.generated.h"

struct FRockInventorySectionInfo;
struct FRockInventorySlotEntry;
struct FRockItemStack;
class URockItemDefinition;
class URockInventory;
/**
 * Query struct for filtering inventory slots, sections, and item stacks.
 * 
 * Predicates are evaluated in order: Section->Slot->Item, so you can optimize your query by putting the most restrictive predicate first.
 * 
 *	FRockInventoryQuery::ForSectionWithTag(Tag::Section::Equipment)
 *	FRockInventoryQuery::ForItemOfType(Tag::Item::Weapon)
 * 
 * The Querys are accepted by the inventory. Such as URockInventory::FindFirstSlot(const FRockInventoryQuery& Query)
 * 
 * Usage: combined queries via And* chaining:
 * All unlocked slots in equipment sections that contain a weapon
 * FRockInventoryQuery Q = FRockInventoryQuery::ForSectionWithTag(Tag::Section::Equipment)
 * 	.AndSlot([](const FRockInventorySlotEntry& Slot)	{ return !Slot.bIsLocked; })
 * 	.AndItem([](const FRockItemStack& Stack) { return Stack.ItemDef && Stack.ItemDef->ItemType.HasTag(Tag::Item::Weapon); });
 * 	
* 	Usage - fully custom:
 *	FRockInventoryQuery Q;
 *	Q.ItemPredicate = [](const FRockItemStack* Stack) { return Stack->StackCount > 5; };
 */
USTRUCT()
struct ROCKINVENTORYRUNTIME_API FRockInventoryQuery
{
	GENERATED_BODY()

public:
	// Note: We validate Item is non-null before calling the ItemPredicate, so you can assume it's valid in the predicate and don't have to check again.
	TFunction<bool(const FRockItemStack*)> ItemPredicate;
	// Note: We validate Slot is non-null before calling the SlotPredicate, so you can assume it's valid in the predicate and don't have to check again.
	TFunction<bool(const FRockInventorySlotEntry*)> SlotPredicate;
	// Note: We validate Section is non-null before calling the SectionPredicate, so you can assume it's valid in the predicate and don't have to check again.
	TFunction<bool(const FRockInventorySectionInfo*)> SectionPredicate;

	// Helper constructors for common queries. These are not exhaustive and you can combine them with the And* functions to create more complex queries.
	static FRockInventoryQuery ForItemWithTag(FGameplayTag Tag);
	static FRockInventoryQuery ForItemOfType(FGameplayTag ItemTypeTag);
	static FRockInventoryQuery ForItemWithDefinition(URockItemDefinition* ItemDef);

	// Section
	static FRockInventoryQuery ForSectionWithSectionTag(FGameplayTag SectionTag);
	static FRockInventoryQuery ForSectionWithMetaTag(FGameplayTag MetaTag);

	// Slot
	static FRockInventoryQuery ForSlotLocked();
	static FRockInventoryQuery ForSlotUnlocked();

	// "Which sections would accept this item?" respects the SectionFilter
	static FRockInventoryQuery ForSectionsAcceptingItemType(const FGameplayTagContainer& ItemTags);

	FRockInventoryQuery& AndSection(TFunction<bool(const FRockInventorySectionInfo*)> Predicate);
	FRockInventoryQuery& AndSlot(TFunction<bool(const FRockInventorySlotEntry*)> Predicate);
	FRockInventoryQuery& AndItem(TFunction<bool(const FRockItemStack*)> Predicate);

	template <typename T>
	static FRockInventoryQuery ForItemsWithFragment();
};


template <typename T>
FRockInventoryQuery FRockInventoryQuery::ForItemsWithFragment()
{
	static_assert(TIsDerivedFrom<T, FRockItemFragment>::IsDerived, "T must be a FRockItemFragment");
	// Fallback alternative to DerivedFrom? std::is_base_of_v<FRockItemFragment, T> 

	FRockInventoryQuery Q;
	Q.ItemPredicate = [](const FRockItemStack* Stack)
	{
		// We can safely assume Stack and Stack->GetDefinition() are valid 
		// because of the check in ForEachSlot, but a defensive check costs almost nothing.
		if (const URockItemDefinition* Def = Stack->GetDefinition())
		{
			return Def->HasFragment<T>();
		}
		return false;
	};
	return Q;
}

/** Data-only input of a loot call (URockInventoryLibrary::LootItemToInventory / PreviewLoot). The defaults loot like a plain pickup. */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockLootParams
{
	GENERATED_BODY()

	/** ERockLootIntent flags the call may use. A section is a candidate only when it accepts at least one of these (FRockInventorySectionInfo::AcceptedLootIntents). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/RockInventoryRuntime.ERockLootIntent"))
	int32 Intent = static_cast<int32>(ERockLootIntent::Store | ERockLootIntent::Equip);

	/**
	 * An Equip-only call (Equip without Store) that finds no empty equipment slot may displace the first occupied one, in plan order, that the item fits in.
	 * The displaced stack is stored through a Store call; if it cannot be stored the whole call is refused and nothing changes. Ignored when Store is also set.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowSwap = false;

	/** Sections carrying any of these meta tags are never used by this call (e.g. Inventory.Behavior.AutoEquip sections). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ExcludeSectionMetaTags;

	bool HasIntent(ERockLootIntent Flag) const { return (Intent & static_cast<int32>(Flag)) != 0; }

	/** True when this call may displace an occupied equipment slot: bAllowSwap on an Equip call that does not also allow Store. */
	bool CanSwap() const { return bAllowSwap && HasIntent(ERockLootIntent::Equip) && !HasIntent(ERockLootIntent::Store); }
};

/** One placement a loot call made (or, from PreviewLoot, would make now). */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockLootPlacement
{
	GENERATED_BODY()

	/** The slot that took the items: an existing stack that was topped up, or the anchor slot of a new stack. */
	UPROPERTY(BlueprintReadOnly)
	FRockSlotReference Slot;

	/** Units placed by this entry. */
	UPROPERTY(BlueprintReadOnly)
	int32 Count = 0;

	/** Orientation of the stack in that slot (the chosen one for a new stack, the existing one for a merge). */
	UPROPERTY(BlueprintReadOnly)
	ERockItemOrientation Orientation = ERockItemOrientation::Horizontal;

	/** True when a new stack was created, false when an existing stack was topped up. */
	UPROPERTY(BlueprintReadOnly)
	bool bNewStack = false;
};

/**
 * One section a loot call may use, in the order it is tried. Entries name an inventory as well as a section so nested inventories can join later;
 * the planner only emits entries of the inventory it was given today.
 */
struct FRockLootPlanEntry
{
	const URockInventory* Inventory = nullptr;
	int32 SectionIndex = INDEX_NONE;
	/** 0: the section's LootPreference matches the item, 1: merely allowed. */
	int32 Tier = 1;
	int32 Priority = 0;
};

/** Sections in the order a loot call tries them. Inline storage covers any realistic config without an allocation. */
using FRockLootPlan = TArray<FRockLootPlanEntry, TInlineAllocator<16>>;

/** What a loot call did, or what PreviewLoot says it would do right now. Placements are in the order they are applied. */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockLootResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FRockLootPlacement> Placements;

	/** Units that found no home. 0 when everything was placed. */
	UPROPERTY(BlueprintReadOnly)
	int32 Excess = 0;

	/** True when an Equip call displaced an occupied slot (FRockLootParams::bAllowSwap). Placements then holds the new stack in DisplacedSlot's place. */
	UPROPERTY(BlueprintReadOnly)
	bool bSwapped = false;

	/** The slot whose stack was displaced. Valid only when bSwapped. */
	UPROPERTY(BlueprintReadOnly)
	FRockSlotReference DisplacedSlot;

	/** Where the displaced stack goes (a Store call on the same inventory): merges, then at most one new stack. Applied after Placements. Empty unless bSwapped. */
	UPROPERTY(BlueprintReadOnly)
	TArray<FRockLootPlacement> DisplacedPlacements;

	bool IsFullyPlaced() const { return Excess <= 0; }

	/** Total units placed across all placements. */
	int32 GetPlacedCount() const;

	/** The placement that created a new stack, or null when the call only merged (or placed nothing). */
	const FRockLootPlacement* FindNewStack() const;
};