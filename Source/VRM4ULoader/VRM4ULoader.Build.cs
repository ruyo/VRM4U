// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

using UnrealBuildTool;
using System.IO;


public class VRM4ULoader : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

	public VRM4ULoader(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// for thirdparty header
		//bUseUnityBuild = false;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ThirdPartyPath, "assimp/include"),
				Path.Combine(ThirdPartyPath, "rapidjson/include")
                // ... add public include paths required here ...
            }
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Slate",
				"SlateCore",
				"Core",
				"CoreUObject",
				"Engine",
				"RHI",
				"RenderCore",
				"CinematicCamera",
				"AnimGraphRuntime",
				"Projects",
				"VRM4U",
			});
		PrivateDependencyModuleNames.Add("TimeManagement");

		if (Target.bBuildEditor) {
			PrivateDependencyModuleNames.Add("Persona");
		}

		BuildVersion Version;
		if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
		{
			//if (Version.MajorVersion == X && Version.MinorVersion == Y)
			if (Version.MajorVersion == 5)
			{
				PrivateDependencyModuleNames.Add("IKRig");
				if (Target.bBuildEditor)
				{
					PrivateDependencyModuleNames.Add("IKRigEditor");
				}
			}
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);

		RuntimeDependencies.Add(Path.Combine(ThirdPartyPath, "vrm_specification", "vrm0", "schema", "*"));
		RuntimeDependencies.Add(Path.Combine(ThirdPartyPath, "vrm_specification", "vrm1", "*", "schema", "*"));

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
			//PrivateDependencyModuleNames.Add("VRM4UImporter");
			//PrivateIncludePaths.Add("VRM4UImporter/Private");

			//CircularlyReferencedDependentModules.Add("VRM4UImporter");
		}

		if ((Target.Platform == UnrealTargetPlatform.Win64))
		{
			string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";

			bool bDebug = false;

			if (bDebug){
				PublicDefinitions.Add("WITH_VRM4U_ASSIMP_DEBUG=1");

				string BuildString = (Target.Configuration != UnrealTargetConfiguration.Debug) ? "Debug" : "Debug";
				PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, BuildString, "assimp-vc141-mtd.lib"));

				PublicDelayLoadDLLs.Add("assimp-vc141-mtd.dll");
				RuntimeDependencies.Add(Path.Combine(ThirdPartyPath, "assimp", "bin", PlatformString, "assimp-vc141-mtd.dll"));
			}
			else
			{
				PublicDefinitions.Add("WITH_VRM4U_ASSIMP_DEBUG=0");

				string BuildString = (Target.Configuration != UnrealTargetConfiguration.Debug) ? "Release" : "Release";
				PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, BuildString, "assimp-vc141-mt.lib"));

				PublicDelayLoadDLLs.Add("assimp-vc141-mt.dll");
				RuntimeDependencies.Add(Path.Combine(ThirdPartyPath, "assimp", "bin", PlatformString, "assimp-vc141-mt.dll"));

			}
		}
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// static link
			{
				string PlatformString = "armeabi-v7a";
				PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, "libassimp.a"));
			}

			/*
			// dynamic link
			{
				string PlatformString = "armeabi-v7a";
				PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib", PlatformString, "libassimp.so"));

				string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
				AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "VRM4ULoader_APL.xml"));
			}
			*/
		}
		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			string PlatformString = "IOS";
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, "libassimp.a"));
		}
		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			// static lib
			string PlatformString = "Mac";
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, "libassimp.a"));
		}

	}
}
