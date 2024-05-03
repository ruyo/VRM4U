
using UnrealBuildTool;

public class VRM4UMisc : ModuleRules
{
	public VRM4UMisc(ReadOnlyTargetRules Target) : base(Target)
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
				"UnrealEd",
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
