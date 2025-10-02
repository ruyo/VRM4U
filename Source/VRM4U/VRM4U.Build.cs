// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

using UnrealBuildTool;
using System.IO;

public class VRM4U : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

    public VRM4U(ReadOnlyTargetRules Target) : base(Target)
	{
	    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {
                //"VRM4U/Public",
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                //"VRM4U/Private",
                // ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Engine",
                "RHI",
                "RenderCore",
                "AnimGraphRuntime",
				"LiveLinkInterface",

                // ... add other public dependencies that you statically link with here ...
			}
			);


        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
                "Engine",
				"AssetRegistry",
				"CinematicCamera",
				"InputCore",
				"ControlRig",
				"AnimationCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
			PrivateDependencyModuleNames.Add("LevelEditor");

			PrivateDependencyModuleNames.Add("LevelSequence");
			PrivateDependencyModuleNames.Add("LevelSequenceEditor");
			PrivateDependencyModuleNames.Add("Sequencer");

			BuildVersion Version2;
			if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version2))
			{
				if (Version2.MajorVersion == 5)
				{
					PrivateDependencyModuleNames.Add("MovieRenderPipelineEditor");
					PrivateDependencyModuleNames.Add("ControlRigDeveloper");

				}
			}
		}
		{
			BuildVersion Version2;
			if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version2))
			{
				if (Version2.MajorVersion == 5)
				{
					PrivateDependencyModuleNames.Add("MovieRenderPipelineCore");
				}
			}
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
            }
            );

		BuildVersion Version;
		if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
		{
			//if (Version.MajorVersion == X && Version.MinorVersion == Y)
			if (Version.MajorVersion == 5)
			{
				PrivateDependencyModuleNames.Add("RigVM");
				PrivateDependencyModuleNames.Add("IKRig");
				if (Target.bBuildEditor)
				{
					PrivateDependencyModuleNames.Add("IKRigEditor");
				}

				PrivateDependencyModuleNames.Add("InterchangeCore");
			}
		}

		//(Target.Version.MinorVersion >= 25)
		// warning: crash without OculusVR Plugin
		bool bUseQuestTracking = false; 

		if (bUseQuestTracking)
		{
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				PrivateDependencyModuleNames.AddRange(new string[] {
					"OculusHMD",
					"OVRPlugin",
				});
				PublicDelayLoadDLLs.Add("OVRPlugin.dll");
				RuntimeDependencies.Add("$(EngineDir)/Binaries/ThirdParty/Oculus/OVRPlugin/OVRPlugin/" + Target.Platform.ToString() + "/OVRPlugin.dll");

				PublicDefinitions.Add("WITH_VRM4U_HMD_TRACKER=1");
			}
			else
			{
				PublicDefinitions.Add("WITH_VRM4U_HMD_TRACKER=0");
			}
		}
		else {
			PublicDefinitions.Add("WITH_VRM4U_HMD_TRACKER=0");
		}
	}
}
