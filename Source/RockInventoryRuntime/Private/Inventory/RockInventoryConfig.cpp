// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Inventory/RockInventoryConfig.h"

#include "Enums/RockLootIntent.h"
#include "GameplayTagContainer.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "RockInventoryConfig"

namespace
{
	/** True when some set of item tags satisfies both the filter (when set) and the preference. Exact for up to MaxTagsForPreferenceCheck tags; true (no complaint) beyond that. */
	bool PreferenceCanMatchFilter(const FGameplayTagQuery& Filter, const FGameplayTagQuery& Preference)
	{
		// GetGameplayTagArray replaces its output, so collect each query separately and merge.
		TArray<FGameplayTag> Tags;
		TArray<FGameplayTag> Mentioned;
		Filter.GetGameplayTagArray(Mentioned);
		Tags.Append(Mentioned);
		Preference.GetGameplayTagArray(Mentioned);
		for (const FGameplayTag& Tag : Mentioned)
		{
			Tags.AddUnique(Tag);
		}
		if (Tags.Num() > URockInventoryConfig::MaxTagsForPreferenceCheck)
		{
			return true;
		}

		const uint32 Combinations = 1u << Tags.Num();
		for (uint32 Mask = 0; Mask < Combinations; ++Mask)
		{
			FGameplayTagContainer Candidate;
			for (int32 Bit = 0; Bit < Tags.Num(); ++Bit)
			{
				if (Mask & (1u << Bit))
				{
					Candidate.AddTag(Tags[Bit]);
				}
			}
			if ((Filter.IsEmpty() || Filter.Matches(Candidate)) && Preference.Matches(Candidate))
			{
				return true;
			}
		}
		return false;
	}
}

void URockInventoryConfig::CollectLootIssues(const TArray<FRockInventorySectionInfo>& Sections, TArray<FRockConfigIssue>& OutIssues)
{
	bool bHasStorage = false;
	TSet<FGameplayTag> SeenTags;
	TSet<FGameplayTag> ReportedDuplicates;

	for (int32 Index = 0; Index < Sections.Num(); ++Index)
	{
		const FRockInventorySectionInfo& Section = Sections[Index];

		if (Section.GetNumSlots() > 0 && Section.AcceptsLootIntent(static_cast<int32>(ERockLootIntent::Store)))
		{
			bHasStorage = true;
		}

		const FGameplayTag Tag = Section.GetSectionTag();
		if (Tag.IsValid() && SeenTags.Contains(Tag) && !ReportedDuplicates.Contains(Tag))
		{
			ReportedDuplicates.Add(Tag);
			OutIssues.Add({ERockConfigIssue::DuplicateSectionTag, Index, FText::Format(
				LOCTEXT("DuplicateSectionTag", "Section tag {0} is used by more than one section (first repeated at index {1})."),
				FText::FromName(Tag.GetTagName()), Index)});
		}
		SeenTags.Add(Tag);

		const FGameplayTagQuery& Preference = Section.GetLootPreference();
		if (!Preference.IsEmpty() && !PreferenceCanMatchFilter(Section.GetSectionFilter(), Preference))
		{
			OutIssues.Add({ERockConfigIssue::UnreachableLootPreference, Index, FText::Format(
				LOCTEXT("UnreachableLootPreference", "Section {0} (index {1}) has a LootPreference that no item its SectionFilter accepts can match, so the preference never applies."),
				FText::FromName(Tag.GetTagName()), Index)});
		}
	}

	if (!bHasStorage)
	{
		OutIssues.Add({ERockConfigIssue::NoStorageSection, INDEX_NONE,
			LOCTEXT("NoStorageSection", "No section with slots accepts the Store loot intent, so picked-up items have nowhere to go. Set AcceptedLootIntents to include Store on at least one section.")});
	}
}

#if WITH_EDITOR
EDataValidationResult URockInventoryConfig::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TArray<FRockConfigIssue> Issues;
	CollectLootIssues(InventoryTabs, Issues);
	for (const FRockConfigIssue& Issue : Issues)
	{
		Context.AddWarning(Issue.Message);
	}

	if (Result == EDataValidationResult::NotValidated)
	{
		Result = EDataValidationResult::Valid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
