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
	}
};

IMPLEMENT_MODULE(FRockInventoryTestsModule, RockInventoryTests)
