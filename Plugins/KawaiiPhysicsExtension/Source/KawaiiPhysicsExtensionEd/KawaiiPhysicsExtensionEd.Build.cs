// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

using UnrealBuildTool;

public class KawaiiPhysicsExtensionEd : ModuleRules
{
	public KawaiiPhysicsExtensionEd(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"KawaiiPhysicsExtension",
			"AnimGraph",
			"BlueprintGraph",
			"Persona",
			"UnrealEd",
			"AnimGraphRuntime",
			"Slate",
			"SlateCore"
		});

		if(Target.Version.MajorVersion >= 5)
		{
			PrivateDependencyModuleNames.Add("EditorFramework");
			if (Target.Version.MinorVersion >= 1)
			{
				PrivateDependencyModuleNames.Add("AnimationEditMode");
			}

			// StructUtils plugin has been integrated into the engine starting from 5.5
			if (Target.Version.MinorVersion <= 4)
			{
				PrivateDependencyModuleNames.Add("StructUtils");
			}
		}
	}
}