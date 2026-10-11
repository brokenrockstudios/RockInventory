// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"
#include "RockInventoryTestTags.h"

#include "Inventory/RockInventoryConfig.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Misc/DataValidation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// URockInventoryConfig loot setting checks (T-37): CollectLootIssues and the IsDataValid warnings built on it.
TEST_CLASS(RockInventoryConfigTests, "BRS.RockInventory.Config")
{
	static FRockInventorySectionInfo Storage(const FGameplayTag& Tag)
	{
		return FRockInventorySectionInfo(Tag, 0, 4, 4);
	}

	static FRockInventorySectionInfo Equipment(const FGameplayTag& Tag)
	{
		FRockInventorySectionInfo Section(Tag, 0, 1, 1);
		Section.SetAcceptedLootIntents(ERockLootIntent::Equip);
		return Section;
	}

	static TArray<FRockConfigIssue> Check(const TArray<FRockInventorySectionInfo>& Sections)
	{
		TArray<FRockConfigIssue> Issues;
		URockInventoryConfig::CollectLootIssues(Sections, Issues);
		return Issues;
	}

	static int32 CountOf(const TArray<FRockConfigIssue>& Issues, ERockConfigIssue Kind)
	{
		int32 Count = 0;
		for (const FRockConfigIssue& Issue : Issues)
		{
			Count += Issue.Kind == Kind ? 1 : 0;
		}
		return Count;
	}

	static FGameplayTagQuery AnyOf(std::initializer_list<FGameplayTag> Tags)
	{
		FGameplayTagContainer Container;
		for (const FGameplayTag& Tag : Tags)
		{
			Container.AddTag(Tag);
		}
		return FGameplayTagQuery::MakeQuery_MatchAnyTags(Container);
	}

	TEST_METHOD(Sensible_EquipmentPlusStorage_HasNoIssues)
	{
		FRockInventorySectionInfo Primary = Equipment(RockInventoryTestTags::SectionPrimary);
		Primary.SetSectionFilter(AnyOf({RockInventoryTestTags::Weapon})).SetLootPreference(AnyOf({RockInventoryTestTags::Wieldable}));
		FRockInventorySectionInfo Secondary = Equipment(RockInventoryTestTags::SectionSecondary);
		Secondary.SetSectionFilter(AnyOf({RockInventoryTestTags::Weapon})).SetLootPreference(AnyOf({RockInventoryTestTags::Sidearm}));
		ASSERT_THAT(AreEqual(0, Check({Primary, Secondary, Storage(RockInventoryTags::Inventory_Section_Backpack)}).Num()));
	}

	TEST_METHOD(NoSections_WarnsNoStorage)
	{
		const TArray<FRockConfigIssue> Issues = Check({});
		ASSERT_THAT(AreEqual(1, Issues.Num()));
		ASSERT_THAT(IsTrue(Issues[0].Kind == ERockConfigIssue::NoStorageSection));
		ASSERT_THAT(AreEqual(static_cast<int32>(INDEX_NONE), Issues[0].SectionIndex));
	}

	TEST_METHOD(OnlyEquipSections_WarnsNoStorage)
	{
		const TArray<FRockConfigIssue> Issues = Check({Equipment(RockInventoryTestTags::SectionPrimary), Equipment(RockInventoryTestTags::SectionHead)});
		ASSERT_THAT(AreEqual(1, CountOf(Issues, ERockConfigIssue::NoStorageSection)));
	}

	TEST_METHOD(StoreSectionWithoutSlots_DoesNotCountAsStorage)
	{
		FRockInventorySectionInfo Empty(RockInventoryTags::Inventory_Section_Backpack, 0, 0, 0);
		ASSERT_THAT(AreEqual(1, CountOf(Check({Empty}), ERockConfigIssue::NoStorageSection)));
	}

	TEST_METHOD(SectionAcceptingStoreAndEquip_CountsAsStorage)
	{
		FRockInventorySectionInfo Quick(RockInventoryTestTags::SectionPrimary, 0, 1, 1);
		Quick.SetAcceptedLootIntents(ERockLootIntent::Store | ERockLootIntent::Equip);
		ASSERT_THAT(AreEqual(0, CountOf(Check({Quick}), ERockConfigIssue::NoStorageSection)));
	}

	TEST_METHOD(DuplicateSectionTag_WarnsOnceWithTheIndexOfTheRepeat)
	{
		const TArray<FRockConfigIssue> Issues = Check({
			Storage(RockInventoryTags::Inventory_Section_Backpack),
			Storage(RockInventoryTags::Inventory_Section_Pockets),
			Storage(RockInventoryTags::Inventory_Section_Backpack),
			Storage(RockInventoryTags::Inventory_Section_Backpack)});
		ASSERT_THAT(AreEqual(1, Issues.Num()));
		ASSERT_THAT(IsTrue(Issues[0].Kind == ERockConfigIssue::DuplicateSectionTag));
		ASSERT_THAT(AreEqual(2, Issues[0].SectionIndex));
	}

	TEST_METHOD(UnsetSectionTags_AreNotReportedAsDuplicates)
	{
		ASSERT_THAT(AreEqual(0, CountOf(Check({Storage(FGameplayTag()), Storage(FGameplayTag())}), ERockConfigIssue::DuplicateSectionTag)));
	}

	TEST_METHOD(Preference_OnATagTheFilterExcludes_WarnsUnreachable)
	{
		// The filter rejects every Sidearm, and the preference only wants Sidearms. (A filter that merely lists other tags is reachable: one item can carry both.)
		FGameplayTagQuery NoSidearm = FGameplayTagQuery::BuildQuery(
			FGameplayTagQueryExpression().NoTagsMatch().AddTag(RockInventoryTestTags::Sidearm));
		FRockInventorySectionInfo Section = Storage(RockInventoryTags::Inventory_Section_Backpack);
		Section.SetSectionFilter(NoSidearm).SetLootPreference(AnyOf({RockInventoryTestTags::Sidearm}));
		const TArray<FRockConfigIssue> Issues = Check({Section});
		ASSERT_THAT(AreEqual(1, Issues.Num()));
		ASSERT_THAT(IsTrue(Issues[0].Kind == ERockConfigIssue::UnreachableLootPreference));
		ASSERT_THAT(AreEqual(0, Issues[0].SectionIndex));
	}

	TEST_METHOD(Preference_ExcludingWhatTheFilterRequires_WarnsUnreachable)
	{
		// Every item the filter admits carries Weapon, and the preference needs it absent.
		FGameplayTagQuery NoWeapon = FGameplayTagQuery::BuildQuery(
			FGameplayTagQueryExpression().NoTagsMatch().AddTag(RockInventoryTestTags::Weapon));
		FRockInventorySectionInfo Section = Storage(RockInventoryTags::Inventory_Section_Backpack);
		Section.SetSectionFilter(AnyOf({RockInventoryTestTags::Weapon})).SetLootPreference(NoWeapon);
		ASSERT_THAT(AreEqual(1, CountOf(Check({Section}), ERockConfigIssue::UnreachableLootPreference)));
	}

	TEST_METHOD(Preference_AnItemCouldSatisfyBoth_IsFine)
	{
		// Filter admits Weapon or Food, preference wants Sidearm: an item tagged Weapon and Sidearm passes both.
		FRockInventorySectionInfo Section = Storage(RockInventoryTags::Inventory_Section_Backpack);
		Section.SetSectionFilter(AnyOf({RockInventoryTestTags::Weapon, RockInventoryTestTags::Food})).SetLootPreference(AnyOf({RockInventoryTestTags::Sidearm}));
		ASSERT_THAT(AreEqual(0, CountOf(Check({Section}), ERockConfigIssue::UnreachableLootPreference)));
	}

	TEST_METHOD(Preference_WithNoFilter_IsFine)
	{
		FRockInventorySectionInfo Section = Storage(RockInventoryTags::Inventory_Section_Backpack);
		Section.SetLootPreference(AnyOf({RockInventoryTestTags::Sidearm}));
		ASSERT_THAT(AreEqual(0, CountOf(Check({Section}), ERockConfigIssue::UnreachableLootPreference)));
	}

	TEST_METHOD(Preference_OnAChildOfTheFilterTag_IsFine)
	{
		// Matching is hierarchical: an item tagged with a child tag satisfies a filter on its parent. The test item tags are siblings,
		// so use the hierarchy of a section tag instead: filter on the parent, preference on the child.
		FRockInventorySectionInfo Section = Storage(RockInventoryTags::Inventory_Section_Backpack);
		Section.SetSectionFilter(AnyOf({RockInventoryTags::Inventory_Section_Backpack.GetTag().RequestDirectParent()}))
			.SetLootPreference(AnyOf({RockInventoryTags::Inventory_Section_Backpack}));
		ASSERT_THAT(AreEqual(0, CountOf(Check({Section}), ERockConfigIssue::UnreachableLootPreference)));
	}

#if WITH_EDITOR // URockInventoryConfig::IsDataValid is editor-only
	TEST_METHOD(IsDataValid_ReportsEachIssueAsAWarningAndStaysValid)
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {Equipment(RockInventoryTestTags::SectionPrimary), Equipment(RockInventoryTestTags::SectionPrimary)};

		FDataValidationContext Context;
		const EDataValidationResult Result = Config->IsDataValid(Context);
		ASSERT_THAT(IsTrue(Result == EDataValidationResult::Valid));
		ASSERT_THAT(AreEqual(2u, static_cast<uint32>(Context.GetNumWarnings()))); // duplicate tag, no storage
		ASSERT_THAT(AreEqual(0u, static_cast<uint32>(Context.GetNumErrors())));
	}

	TEST_METHOD(IsDataValid_CleanConfig_HasNoWarnings)
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {Equipment(RockInventoryTestTags::SectionPrimary), Storage(RockInventoryTags::Inventory_Section_Backpack)};

		FDataValidationContext Context;
		ASSERT_THAT(IsTrue(Config->IsDataValid(Context) == EDataValidationResult::Valid));
		ASSERT_THAT(AreEqual(0u, static_cast<uint32>(Context.GetNumWarnings())));
	}
#endif // WITH_EDITOR
};
#endif // WITH_DEV_AUTOMATION_TESTS
