// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

using UnrealBuildTool;

public class VRM4UCaptureEditor : ModuleRules
{
	public VRM4UCaptureEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "Engine",

				"AnimGraphRuntime",
				"AnimGraph",
				"BlueprintGraph",

				"UnrealEd",
				"AnimationEditMode",
				"Persona",

				"VRM4U",
				"VRM4UCapture",
			});

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
            });

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
			});

        PrivateIncludePaths.AddRange(
        new string[] {
			//"../Runtime/Renderer/Private",
        });
    }
}
