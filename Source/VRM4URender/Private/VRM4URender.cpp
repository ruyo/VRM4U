
#include "VRM4URender.h"
#include "CoreMinimal.h"
#include "VRM4URenderLog.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Internationalization/Internationalization.h"
#define LOCTEXT_NAMESPACE "VRM4URender"

DEFINE_LOG_CATEGORY(LogVRM4URender);

//////////////////////////////////////////////////////////////////////////
// FSpriterImporterModule

class FVRM4URenderModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("VRM4U"))->GetBaseDir(), TEXT("Shaders"));

		AddShaderSourceDirectoryMapping(TEXT("/VRM4UShaders"), PluginShaderDir);
	}

	virtual void ShutdownModule() override
	{
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FVRM4URenderModule, VRM4URender);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
