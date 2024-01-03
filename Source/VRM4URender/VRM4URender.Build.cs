
using UnrealBuildTool;

public class VRM4URender : ModuleRules
{
	public VRM4URender(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		BuildVersion Version;
		if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
		{
			//if (Version.MajorVersion == X && Version.MinorVersion == Y)
			if (Version.MajorVersion == 5 && Version.MinorVersion >= 1) {
				PrivateIncludePaths.AddRange(
					new string[] {
						System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Private"), //required for FPostProcessMaterialInputs
					});
			}
		}



		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "Engine",
				"Projects",
				"RenderCore",

                "RHI",
                //"ShaderCore",
                "Renderer",
            });
    }
}
