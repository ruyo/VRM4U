
using System;
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
				"UnrealEd",
				"Slate",
				"SlateCore",
				"VRM4U",
				"VRM4ULoader",

				"AnimGraphRuntime",
				"AnimGraph",
				"BlueprintGraph",

				"PropertyEditor",
				"Persona",
				"MainFrame",

				//"ToolMenus",
			});

		BuildVersion Version;
		if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version)) {
			//if (Version.MajorVersion == X && Version.MinorVersion == Y)
			if (Version.MajorVersion == 4 && Version.MinorVersion <= 23)
			{
			}
			else {
				PrivateDependencyModuleNames.Add("ToolMenus");
			}
		}

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
