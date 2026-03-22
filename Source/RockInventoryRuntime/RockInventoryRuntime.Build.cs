// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

using UnrealBuildTool;

public class RockInventoryRuntime : ModuleRules
{
	public RockInventoryRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",
				"RockGameplayTags",
				"UMG",
				"DeveloperSettings",
				// ... add public dependencies that you the /public files need
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"NetCore",
				"AssetRegistry",
				"Iris",
				"IrisCore",
				// ... add private dependencies that /private files need	
			}
		);
		
		SetupIrisSupport(Target);
	}
}