// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

using UnrealBuildTool;

public class RockInventory : ModuleRules
{
	public RockInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...

				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"NetCore"
				// ... add private dependencies that you statically link with here ...	
			}
		);
	}
}