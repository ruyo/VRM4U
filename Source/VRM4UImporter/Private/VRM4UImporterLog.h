

#pragma once

#include "CoreMinimal.h"
//#include "VRMImporterModule.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVRM4UImporter, Verbose, All);

#define SPRITER_IMPORT_ERROR(FormatString, ...) \
	if (!bSilent) { UE_LOG(LogVRMImporter, Warning, FormatString, __VA_ARGS__); }
#define SPRITER_IMPORT_WARNING(FormatString, ...) \
	if (!bSilent) { UE_LOG(LogVRMImporter, Warning, FormatString, __VA_ARGS__); }

