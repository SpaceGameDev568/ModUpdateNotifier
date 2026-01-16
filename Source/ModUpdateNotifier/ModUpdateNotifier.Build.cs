// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ModUpdateNotifier : ModuleRules
{
	public ModUpdateNotifier(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"FactoryGame",
				"SML",
				"HTTP",
				"Json",
				"JsonUtilities",
				"UMG"
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);
	}
}
