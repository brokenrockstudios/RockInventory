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
				"UMG",
				"RockInventoryRuntime", 
				"RockDragDrop", 
				"CommonUI",
			}
		);

		// Only available to the /Private folder C++ files.
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"UMG",
				"Slate",
				"SlateCore",
				"InputCore",
				"GameplayTags",
				"DeveloperSettings",
				"CommonUI", 
				"RockClassification",
				"RockDragDrop",
				
				
				// test
				// "UMGEditor",  Can't be in shipping because UnrealEd
				// Probably don't need anymore?
				//"ToolMenus", 
				//"EditorWidgets" 
			}
		);
	}
}