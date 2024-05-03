// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

using UnrealBuildTool;

public class VRM4UEditor : ModuleRules
{
	public VRM4UEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
                "EditorStyle",
                "Engine",
				"UnrealEd",
                "Slate",
                "SlateCore",

                "MovieSceneCapture",

				"MovieScene",
				"MovieSceneTracks",

				//"ControlRigEditor",

				"VRM4U",
			});

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry",
            });

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry"
			});

        PrivateIncludePaths.AddRange(
        new string[] {
			//"../Runtime/Renderer/Private",
        });
    }
}
