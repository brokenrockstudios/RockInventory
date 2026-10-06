// Copyright Broken Rock Studios LLC. All Rights Reserved.

using UnrealBuildTool;

// Automation tests for RockInventoryRuntime. Developer module: not built for Shipping, so CQTest never leaks into a shipped target.
public class RockInventoryTests : ModuleRules
{
	public RockInventoryTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		SetupIrisSupport(Target);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"NetCore",
				"Iris",
				"IrisCore",
				"GameplayTags",
				"CQTest",
				"RockInventoryRuntime",
			}
		);

		// The PIE network tests (CQTest's FPIENetworkComponent) need the editor.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
