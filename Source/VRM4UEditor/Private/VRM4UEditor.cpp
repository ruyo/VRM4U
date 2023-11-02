
#include "VRM4UEditor.h"
#include "CoreMinimal.h"
#include "VRM4UEditorLog.h"
#include "Modules/ModuleManager.h"
#include "Internationalization/Internationalization.h"

#define LOCTEXT_NAMESPACE "VRM4UEditor"

DEFINE_LOG_CATEGORY(LogVRM4UEditor);

//////////////////////////////////////////////////////////////////////////
// FSpriterImporterModule

class FVRM4UEditorModule : public FDefaultModuleImpl
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

IMPLEMENT_MODULE(FVRM4UEditorModule, VRM4UEditor);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
