// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

using UnrealBuildTool;

public class RockInventoryUI : ModuleRules
{
	public RockInventoryUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		SetupIrisSupport(Target);
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"RockInventoryRuntime",
				"UMG"
			}
		);

		// Only available to the /Private folder C++ files.
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
		);
	}
}