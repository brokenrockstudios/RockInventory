// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Inventory/RockInventoryQuery.h"

#include "Inventory/RockInventorySlot.h"

FRockInventoryQuery FRockInventoryQuery::ForItemWithTag(FGameplayTag Tag)
{
	FRockInventoryQuery Query;
	Query.ItemPredicate = [Tag](const FRockItemStack* Stack)
	{
		return Stack->GetDefinition() && Stack->GetDefinition()->ItemTags.HasTag(Tag);
	};
	return Query;
}

FRockInventoryQuery FRockInventoryQuery::ForItemOfType(FGameplayTag ItemTypeTag)
{
	FRockInventoryQuery Query;
	Query.ItemPredicate = [ItemTypeTag](const FRockItemStack* Stack)
	{
		return Stack->GetDefinition() && Stack->GetDefinition()->ItemType.HasTag(ItemTypeTag);
	};
	return Query;
}

FRockInventoryQuery FRockInventoryQuery::ForItemWithDefinition(URockItemDefinition* ItemDef)
{
	FRockInventoryQuery Query;
	Query.ItemPredicate = [ItemDef](const FRockItemStack* Stack)
	{
		return Stack->GetDefinition() == ItemDef;
	};
	return Query;
}

FRockInventoryQuery FRockInventoryQuery::ForSectionWithSectionTag(FGameplayTag SectionTag)
{
	FRockInventoryQuery Query;
	Query.SectionPredicate = [SectionTag](const FRockInventorySectionInfo* Section)
	{
		return Section->GetSectionTag() == SectionTag;
	};
	return Query;
}

FRockInventoryQuery FRockInventoryQuery::ForSectionWithMetaTag(FGameplayTag MetaTag)
{
	FRockInventoryQuery Query;
	Query.SectionPredicate = [MetaTag](const FRockInventorySectionInfo* Section)
	{
		return Section->GetMetaTags().HasTag(MetaTag);
	};
	return Query;
}

FRockInventoryQuery FRockInventoryQuery::ForSlotLocked()
{
	FRockInventoryQuery Query;
	Query.SlotPredicate = [](const FRockInventorySlotEntry* Slot)
	{
		return Slot->bIsLocked;
	};
	return Query;
}

FRockInventoryQuery FRockInventoryQuery::ForSlotUnlocked()
{
	FRockInventoryQuery Query;
	Query.SlotPredicate = [](const FRockInventorySlotEntry* Slot)
	{
		return !Slot->bIsLocked;
	};
	return Query;
}


FRockInventoryQuery FRockInventoryQuery::ForSectionsAcceptingItemType(const FGameplayTagContainer& ItemTags)
{
	FRockInventoryQuery Query;
	Query.SectionPredicate = [ItemTags](const FRockInventorySectionInfo* Section)
	{
		return Section->GetSectionFilter().IsEmpty() || Section->GetSectionFilter().Matches(ItemTags);
	};
	return Query;
}

FRockInventoryQuery& FRockInventoryQuery::AndSection(TFunction<bool(const FRockInventorySectionInfo*)> Predicate)
{
	if (SectionPredicate)
	{
		auto ExistingPred = MoveTemp(SectionPredicate);
		SectionPredicate = [Existing = MoveTemp(ExistingPred), Pred=MoveTemp(Predicate)](const FRockInventorySectionInfo* S)
		{
			return Existing(S) && Pred(S);
		};
	}
	else
	{
		SectionPredicate = MoveTemp(Predicate);
	}
	return *this;
}

FRockInventoryQuery& FRockInventoryQuery::AndSlot(TFunction<bool(const FRockInventorySlotEntry*)> Predicate)
{
	if (SlotPredicate)
	{
		auto ExistingPred = MoveTemp(SlotPredicate);
		SlotPredicate = [Existing = MoveTemp(ExistingPred), Pred=MoveTemp(Predicate)](const FRockInventorySlotEntry* S)
		{
			return Existing(S) && Pred(S);
		};
	}
	else
	{
		SlotPredicate = MoveTemp(Predicate);
	}
	return *this;
}

FRockInventoryQuery& FRockInventoryQuery::AndItem(TFunction<bool(const FRockItemStack*)> Predicate)
{
	if (ItemPredicate)
	{
		auto ExistingPred = MoveTemp(ItemPredicate);
		ItemPredicate = [Existing = MoveTemp(ExistingPred), Pred=MoveTemp(Predicate)](const FRockItemStack* I)
		{
			return Existing(I) && Pred(I);
		};
	}
	else
	{
		ItemPredicate = MoveTemp(Predicate);
	}
	return *this;
}
