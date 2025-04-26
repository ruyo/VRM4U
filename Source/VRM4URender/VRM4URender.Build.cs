
using UnrealBuildTool;

public class VRM4URender : ModuleRules
{
	public VRM4URender(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Private"), //required for FPostProcessMaterialInputs
				System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Internal"), //required for FPostProcessMaterialInputs
			});



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
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
