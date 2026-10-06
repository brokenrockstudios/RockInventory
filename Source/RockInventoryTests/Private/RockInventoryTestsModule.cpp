// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestTags.h"

#include "GameplayTagsManager.h"
#include "Modules/ModuleManager.h"

// Test-only tags. Native tags cannot be defined in a Developer module with UE_DEFINE_GAMEPLAY_TAG (see Docs/TestingConventions.md),
// so they are registered the legacy way at startup and looked up by name.
namespace RockInventoryTestTags
{
	FGameplayTag Weapon;
	FGameplayTag Food;
	FGameplayTag MetaA;
	FGameplayTag MetaB;
	FGameplayTag Sidearm;
	FGameplayTag Wieldable;
	FGameplayTag Headgear;
	FGameplayTag SectionHead;
	FGameplayTag SectionPrimary;
	FGameplayTag SectionSecondary;
	TArray<FGameplayTag> BulkSections;
}

class FRockInventoryTestsModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
		UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
		RockInventoryTestTags::Weapon = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Item.Weapon"), TEXT("RockInventoryTests: item tag"));
		RockInventoryTestTags::Food = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Item.Food"), TEXT("RockInventoryTests: item tag"));
		RockInventoryTestTags::MetaA = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Section.MetaA"), TEXT("RockInventoryTests: section meta tag"));
		RockInventoryTestTags::MetaB = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Section.MetaB"), TEXT("RockInventoryTests: section meta tag"));
		RockInventoryTestTags::Sidearm = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Item.Sidearm"), TEXT("RockInventoryTests: item tag"));
		RockInventoryTestTags::Wieldable = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Item.Wieldable"), TEXT("RockInventoryTests: item tag"));
		RockInventoryTestTags::Headgear = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Item.Headgear"), TEXT("RockInventoryTests: item tag"));
		RockInventoryTestTags::SectionHead = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Section.Head"), TEXT("RockInventoryTests: section tag"));
		RockInventoryTestTags::SectionPrimary = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Section.Primary"), TEXT("RockInventoryTests: section tag"));
		RockInventoryTestTags::SectionSecondary = Manager.AddNativeGameplayTag(TEXT("Test.RockInventory.Section.Secondary"), TEXT("RockInventoryTests: section tag"));
		for (int32 Index = 0; Index < 12; ++Index)
		{
			const FString TagName = FString::Printf(TEXT("Test.RockInventory.Section.Bulk%d"), Index);
			RockInventoryTestTags::BulkSections.Add(Manager.AddNativeGameplayTag(*TagName, TEXT("RockInventoryTests: section tag")));
		}
	}
};

IMPLEMENT_MODULE(FRockInventoryTestsModule, RockInventoryTests)
