// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"
#include "RockInventoryTestTags.h"

#include "Inventory/RockInventoryQuery.h"
#include "Library/RockInventoryLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// Section filters (hard item eligibility) and meta tags (section characteristics), both private fields of FRockInventorySectionInfo.
TEST_CLASS(RockInventorySectionTests, "BRS.RockInventory.Sections")
{
	FRockInventoryFixture Fixture;

	/** Pockets (2x1, MetaA), Backpack (2x1, no meta), Storage (2x1, MetaA and MetaB), in that config order. */
	void InitMetaLayout()
	{
		FGameplayTagContainer MetaA;
		MetaA.AddTag(RockInventoryTestTags::MetaA);
		FGameplayTagContainer MetaAB = MetaA;
		MetaAB.AddTag(RockInventoryTestTags::MetaB);
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1, FGameplayTagContainer(), MetaA),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 2, 1),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Storage, 2, 1, FGameplayTagContainer(), MetaAB)});
	}

	TEST_METHOD(Setters_FilterAndMetaTags_AreReadBackAndSurviveInventoryInit)
	{
		FGameplayTagContainer Required(RockInventoryTestTags::Weapon);
		FGameplayTagContainer Meta(RockInventoryTestTags::MetaB);
		Fixture.Init({FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1, Required, Meta)});

		const FRockInventorySectionInfo& Section = Fixture.Inventory->GetSectionInfo(RockInventoryTags::Inventory_Section_Pockets);
		ASSERT_THAT(IsFalse(Section.GetSectionFilter().IsEmpty()));
		ASSERT_THAT(IsTrue(Section.GetMetaTags().HasTag(RockInventoryTestTags::MetaB)));
		ASSERT_THAT(IsFalse(Section.GetMetaTags().HasTag(RockInventoryTestTags::MetaA)));
	}

	TEST_METHOD(Setters_ReturnTheSameSectionForChaining)
	{
		FRockInventorySectionInfo Section(RockInventoryTags::Inventory_Section_Pockets, 0, 1, 1);
		FRockInventorySectionInfo& Result = Section.SetMetaTags(FGameplayTagContainer(RockInventoryTestTags::MetaA));
		ASSERT_THAT(IsTrue(&Result == &Section));
		ASSERT_THAT(IsTrue(&Section.SetSectionFilter(FGameplayTagQuery()) == &Section));
	}

	TEST_METHOD(CanBePlaced_FilterMatches_OnlyForItemsCarryingTheTag)
	{
		FRockInventorySectionInfo Section = FRockInventoryFixture::MakeSection(
			RockInventoryTags::Inventory_Section_Pockets, 1, 1, FGameplayTagContainer(RockInventoryTestTags::Weapon));
		Section.Initialize(0, 0);
		URockItemDefinition* Sword = Fixture.MakeTaggedDefinition("Sword", FGameplayTagContainer(RockInventoryTestTags::Weapon));
		URockItemDefinition* Apple = Fixture.MakeTaggedDefinition("Apple", FGameplayTagContainer(RockInventoryTestTags::Food));
		URockItemDefinition* Rock = Fixture.MakeDefinition("Rock");

		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemBePlacedInSection(FRockItemStack(Sword, 1), Section)));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemBePlacedInSection(FRockItemStack(Apple, 1), Section)));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemBePlacedInSection(FRockItemStack(Rock, 1), Section)));
	}

	TEST_METHOD(CanBePlaced_ItemTypeTagsCountToo)
	{
		// GetAllTags() is ItemType plus ItemTags, so a filter on a type tag matches an item that only sets ItemType.
		FRockInventorySectionInfo Section = FRockInventoryFixture::MakeSection(
			RockInventoryTags::Inventory_Section_Pockets, 1, 1, FGameplayTagContainer(RockInventoryTestTags::Weapon));
		Section.Initialize(0, 0);
		URockItemDefinition* Sword = Fixture.MakeDefinition("Sword");
		Sword->ItemType.AddTag(RockInventoryTestTags::Weapon);
		Sword->RebuildCachedTags();

		ASSERT_THAT(IsTrue(URockInventoryLibrary::CanItemBePlacedInSection(FRockItemStack(Sword, 1), Section)));
	}

	TEST_METHOD(CanBePlaced_TagsSetWithoutRebuildingTheCache_AreNotSeen)
	{
		// Pins why MakeTaggedDefinition rebuilds: GetAllTags() is a cache filled on PostLoad and on editor edits only.
		FRockInventorySectionInfo Section = FRockInventoryFixture::MakeSection(
			RockInventoryTags::Inventory_Section_Pockets, 1, 1, FGameplayTagContainer(RockInventoryTestTags::Weapon));
		Section.Initialize(0, 0);
		URockItemDefinition* Sword = Fixture.MakeDefinition("Sword");
		Sword->ItemTags.AddTag(RockInventoryTestTags::Weapon);

		ASSERT_THAT(IsFalse(URockInventoryLibrary::CanItemBePlacedInSection(FRockItemStack(Sword, 1), Section)));
	}

	TEST_METHOD(AcceptingItemTypeQuery_ListsSlotsOfSectionsThatAcceptTheTags)
	{
		Fixture.Init({
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1, FGameplayTagContainer(RockInventoryTestTags::Weapon)),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 3, 1)});

		// The unfiltered Backpack (3 slots) accepts everything; the Weapon-only Pockets (2 slots) accepts the weapon.
		const FRockInventoryQuery ForWeapon = FRockInventoryQuery::ForSectionsAcceptingItemType(FGameplayTagContainer(RockInventoryTestTags::Weapon));
		ASSERT_THAT(AreEqual(5, Fixture.Inventory->FindAllSlots(ForWeapon).Num()));

		const FRockInventoryQuery ForFood = FRockInventoryQuery::ForSectionsAcceptingItemType(FGameplayTagContainer(RockInventoryTestTags::Food));
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->FindAllSlots(ForFood).Num()));

		const FRockInventoryQuery ForNoTags = FRockInventoryQuery::ForSectionsAcceptingItemType(FGameplayTagContainer());
		ASSERT_THAT(AreEqual(3, Fixture.Inventory->FindAllSlots(ForNoTags).Num()));
	}

	TEST_METHOD(MetaTag_FindFirstSlot_IsTheFirstSlotOfTheFirstSectionWithTheTag)
	{
		InitMetaLayout();

		ASSERT_THAT(AreEqual(
			Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0),
			URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(Fixture.Inventory, RockInventoryTestTags::MetaA)));
		ASSERT_THAT(AreEqual(
			Fixture.SlotAt(RockInventoryTags::Inventory_Section_Storage, 0, 0),
			URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(Fixture.Inventory, RockInventoryTestTags::MetaB)));
	}

	TEST_METHOD(MetaTag_FindFirstSlot_ReturnsAnEmptySlotAsWellAsAFullOne)
	{
		// The query has no item predicate, so occupancy does not matter.
		InitMetaLayout();
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple");
		Fixture.PlaceAt(Apple, 1, Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0));

		ASSERT_THAT(AreEqual(
			Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0),
			URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(Fixture.Inventory, RockInventoryTestTags::MetaA)));
	}

	TEST_METHOD(MetaTag_FindFirstSlot_NoSectionHasTheTag_IsInvalid)
	{
		InitMetaLayout();

		ASSERT_THAT(IsFalse(URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(Fixture.Inventory, RockInventoryTestTags::Food).IsValid()));
		ASSERT_THAT(IsFalse(URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(Fixture.Inventory, FGameplayTag()).IsValid()));
	}

	TEST_METHOD(MetaTag_FindFirstSlot_NullInventory_IsInvalid)
	{
		ASSERT_THAT(IsFalse(URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(nullptr, RockInventoryTestTags::MetaA).IsValid()));
	}

	TEST_METHOD(MetaTag_FindAllSlots_ListsEverySlotOfEveryTaggedSectionInConfigOrder)
	{
		InitMetaLayout();

		const TArray<FRockInventorySlotHandle> Slots =
			URockInventoryLibrary::FindAllSlotsInSectionsWithMetaTag(Fixture.Inventory, RockInventoryTestTags::MetaA);
		ASSERT_THAT(AreEqual(4, Slots.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 0, 0), Slots[0]));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Pockets, 1, 0), Slots[1]));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Storage, 0, 0), Slots[2]));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Storage, 1, 0), Slots[3]));
	}

	TEST_METHOD(MetaTag_FindAllSlots_OnlyTheSectionCarryingTheSecondTag)
	{
		InitMetaLayout();

		const TArray<FRockInventorySlotHandle> Slots =
			URockInventoryLibrary::FindAllSlotsInSectionsWithMetaTag(Fixture.Inventory, RockInventoryTestTags::MetaB);
		ASSERT_THAT(AreEqual(2, Slots.Num()));
		ASSERT_THAT(AreEqual(Fixture.SlotAt(RockInventoryTags::Inventory_Section_Storage, 0, 0), Slots[0]));
	}

	TEST_METHOD(MetaTag_FindAllSlots_NoMatchOrNullInventory_IsEmpty)
	{
		InitMetaLayout();

		ASSERT_THAT(AreEqual(0, URockInventoryLibrary::FindAllSlotsInSectionsWithMetaTag(Fixture.Inventory, RockInventoryTestTags::Food).Num()));
		ASSERT_THAT(AreEqual(0, URockInventoryLibrary::FindAllSlotsInSectionsWithMetaTag(nullptr, RockInventoryTestTags::MetaA).Num()));
	}

	TEST_METHOD(MetaTag_SectionTagIsNotAMetaTag)
	{
		// ForSectionWithMetaTag looks only at MetaTags, never at the section's own tag.
		InitMetaLayout();
		ASSERT_THAT(IsFalse(URockInventoryLibrary::FindFirstSlotInSectionWithMetaTag(Fixture.Inventory, RockInventoryTags::Inventory_Section_Pockets).IsValid()));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
