
using System;
using UnrealBuildTool;

public class VRM4UImporter : ModuleRules
{
	public VRM4UImporter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"VRM4U",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"InputCore",
				"EditorStyle",
				"ApplicationCore",
				"UnrealEd",
				"Slate",
				"SlateCore",
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
			PrivateDependencyModuleNames.Add("ToolMenus");

			if (Version.MajorVersion == 4 || (Version.MajorVersion == 5 && Version.MinorVersion <= 1))
			{
			}
			else
			{
				PrivateDependencyModuleNames.Add("AssetDefinition");
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
