// Copyright Broken Rock Studios LLC. All Rights Reserved.

using UnrealBuildTool;

// Bridge between RockData records and the item registry: the `Item` record type and the provider that builds URockItemDefinition objects from it.
// RockInventoryRuntime stays free of RockData.
public class RockInventoryData : ModuleRules
{
	public RockInventoryData(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"RockData",
				"RockInventoryRuntime",
			}
		);
	}
}
