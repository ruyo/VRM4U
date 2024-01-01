
using UnrealBuildTool;

public class VRM4URender : ModuleRules
{
	public VRM4URender(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
                "EditorStyle",
                "ApplicationCore",
                "Engine",
				"Json",
				"UnrealEd",
                "Slate",
                "SlateCore",
                "MainFrame",
                "Settings",
				"Projects",
				"RenderCore",

                "RHI",
                //"ShaderCore",
                "Renderer",
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
			// Relative to Engine\Plugins\Runtime\Oculus\OculusVR\Source
			//"../Runtime/Renderer/Private",
        });
    }
}
