// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

using UnrealBuildTool;

public class RockInventoryEditor : ModuleRules
{
	public RockInventoryEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"RockInventoryRuntime",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"ToolMenus",
				"ContentBrowser"
			}
		);
	}
}