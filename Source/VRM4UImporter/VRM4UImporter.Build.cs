
using UnrealBuildTool;

public class VRM4UImporter : ModuleRules
{
	public VRM4UImporter(ReadOnlyTargetRules Target) : base(Target)
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
                "VRM4U",
                "VRM4ULoader",

                "AnimGraphRuntime",
                "AnimGraph",
                "BlueprintGraph",

				"PropertyEditor",
				"Persona"
			});

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry"
			});

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry"
			});
	}
}
