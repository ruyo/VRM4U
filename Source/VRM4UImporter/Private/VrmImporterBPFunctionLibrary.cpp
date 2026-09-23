// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmImporterBPFunctionLibrary.h"

#include "VRM4UImporterFactory.h"
#include "VRM4UImporterLog.h"
#include "VrmAssetListObject.h"

#include "Misc/EngineVersionComparison.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

UVrmAssetListObject* UVrmImporterBPFunctionLibrary::ImportVRMFileWithOptions(
	const FString& SourceFile,
	const FString& DestinationPackagePath,
	const FImportOptionData& Options)
{
	const FString FullSourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
	if (!FPaths::FileExists(FullSourceFile))
	{
		UE_LOG(LogVRM4UImporter, Error, TEXT("VRM4U: source file does not exist: %s"), *FullSourceFile);
		return nullptr;
	}

	if (!FPackageName::IsValidLongPackageName(DestinationPackagePath))
	{
		UE_LOG(
			LogVRM4UImporter,
			Error,
			TEXT("VRM4U: invalid destination package path: %s"),
			*DestinationPackagePath);
		return nullptr;
	}

	UVRM4UImporterFactory* Factory = NewObject<UVRM4UImporterFactory>();
	if (!Factory || !Factory->FactoryCanImport(FullSourceFile))
	{
		UE_LOG(LogVRM4UImporter, Error, TEXT("VRM4U: unsupported source file: %s"), *FullSourceFile);
		return nullptr;
	}

	UPackage* Package = CreatePackage(*DestinationPackagePath);
	if (!Package)
	{
		UE_LOG(
			LogVRM4UImporter,
			Error,
			TEXT("VRM4U: failed to create destination package: %s"),
			*DestinationPackagePath);
		return nullptr;
	}

	Factory->bUseProvidedImportOptions = true;
	Factory->ProvidedImportOptions = Options;

	const uint8* Buffer = nullptr;
	bool bOperationCanceled = false;
	UObject* ImportedObject = Factory->FactoryCreateBinary(
		nullptr,
		Package,
		FName(*FPaths::GetBaseFilename(FullSourceFile)),
		RF_Public | RF_Standalone,
		nullptr,
		*FPaths::GetExtension(FullSourceFile),
		Buffer,
		Buffer,
		GWarn,
		bOperationCanceled);

	return Cast<UVrmAssetListObject>(ImportedObject);
}
