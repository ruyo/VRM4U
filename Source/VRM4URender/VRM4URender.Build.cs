
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
                "Engine",
				"Projects",
				"RenderCore",

                "RHI",
                //"ShaderCore",
                "Renderer",
            });
    }
}
