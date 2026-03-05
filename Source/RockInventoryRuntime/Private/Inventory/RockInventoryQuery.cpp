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

FRockInventoryQuery FRockInventoryQuery::ForSectionWithTag(FGameplayTag SectionTag)
{
	FRockInventoryQuery Query;
	Query.SectionPredicate = [SectionTag](const FRockInventorySectionInfo* Section)
	{
		return Section->GetTags().HasTag(SectionTag);
	};
	return Query;
}

FRockInventoryQuery FRockInventoryQuery::ForSlotLockedSlots()
{
	FRockInventoryQuery Query;
	Query.SlotPredicate = [](const FRockInventorySlotEntry* Slot)
	{
		return Slot->bIsLocked;
	};
	return Query;
}

FRockInventoryQuery FRockInventoryQuery::ForSlotUnlockedSlots()
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

FRockInventoryQuery& FRockInventoryQuery::AndSection(TFunction<bool(const FRockInventorySectionInfo*)> Pred)
{
	if (SectionPredicate)
	{
		auto Existing = MoveTemp(SectionPredicate);
		SectionPredicate = [Existing, Pred](const FRockInventorySectionInfo* S)
		{
			return Existing(S) && Pred(S);
		};
	}
	else
	{
		SectionPredicate = MoveTemp(Pred);
	}
	return *this;
}

FRockInventoryQuery& FRockInventoryQuery::AndSlot(TFunction<bool(const FRockInventorySlotEntry*)> Pred)
{
	if (SlotPredicate)
	{
		auto Existing = MoveTemp(SlotPredicate);
		SlotPredicate = [Existing, Pred](const FRockInventorySlotEntry* S)
		{
			return Existing(S) && Pred(S);
		};
	}
	else
	{
		SlotPredicate = MoveTemp(Pred);
	}
	return *this;
}

FRockInventoryQuery& FRockInventoryQuery::AndItem(TFunction<bool(const FRockItemStack*)> Pred)
{
	if (ItemPredicate)
	{
		auto Existing = MoveTemp(ItemPredicate);
		ItemPredicate = [Existing, Pred](const FRockItemStack* I)
		{
			return Existing(I) && Pred(I);
		};
	}
	else
	{
		ItemPredicate = MoveTemp(Pred);
	}
	return *this;
}
