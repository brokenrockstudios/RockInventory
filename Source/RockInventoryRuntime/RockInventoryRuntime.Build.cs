﻿// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

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
				// ... add other public dependencies that you statically link with here ...
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks",
				"UMG",
				"DeveloperSettings",
				"StructUtils", 
				"RockGameplayTags",
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
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		SetupIrisSupport(Target);
	}
}