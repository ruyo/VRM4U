// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VRM4UCaptureEditor.h"
#include "CoreMinimal.h"
#include "VRM4UCaptureEditorLog.h"
#include "Modules/ModuleManager.h"
#include "Internationalization/Internationalization.h"

#define LOCTEXT_NAMESPACE "VRM4UMisc"

DEFINE_LOG_CATEGORY(LogVRM4UCaptureEditor);

//////////////////////////////////////////////////////////////////////////
// FSpriterImporterModule

class FVRM4UCaptureEditorModule : public FDefaultModuleImpl
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

IMPLEMENT_MODULE(FVRM4UCaptureEditorModule, VRM4UCaptureEditor);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
