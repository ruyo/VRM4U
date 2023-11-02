
#include "VRM4UMisc.h"
#include "CoreMinimal.h"
#include "VRM4UMiscLog.h"
#include "Modules/ModuleManager.h"
#include "Internationalization/Internationalization.h"

#define LOCTEXT_NAMESPACE "VRM4UMisc"

DEFINE_LOG_CATEGORY(LogVRM4UMisc);

//////////////////////////////////////////////////////////////////////////
// FSpriterImporterModule

class FVRM4UMiscModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FVRM4UMiscModule, VRM4UMisc);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
