// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockInventorySectionInfo.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemStack.h"
#include "UObject/Object.h"
#include "RockInventoryQuery.generated.h"

struct FRockInventorySectionInfo;
struct FRockInventorySlotEntry;
struct FRockItemStack;
class URockItemDefinition;
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
 * 	.AndSlot([](const FRockInventorySlot& Slot)	{ return !Slot.bIsLocked; })
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
