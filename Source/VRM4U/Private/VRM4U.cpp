// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "VRM4U.h"
#include "Modules/ModuleManager.h"
#include "VrmRuntimeSettings.h"
#include "Developer/Settings/Public/ISettingsModule.h"

//#include "ISettingsModule.h"


#define LOCTEXT_NAMESPACE "FVRM4UModule"

DEFINE_LOG_CATEGORY(LogVRM4U);


void FVRM4UModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	{
	
	}
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "VRM4U",
			LOCTEXT("RuntimeSettingsName", "VRM4U"),
			LOCTEXT("RuntimeSettingsDescription", "Configure the VRM4U plugin"),
			GetMutableDefault<UVrmRuntimeSettings>());
	}

}

void FVRM4UModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "VRM4U");
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVRM4UModule, VRM4U)