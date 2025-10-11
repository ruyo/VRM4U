// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmRuntimeSettings.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#include "Misc/MessageDialog.h"
#include "UnrealEdMisc.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformFileManager.h"
#endif
#endif

#define LOCTEXT_NAMESPACE "VRM4U"


UVrmRuntimeSettings::UVrmRuntimeSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDropVRMFileEnable = false;
	AssetListObject.SetPath(TEXT("/VRM4U/VrmAssetListObjectBP.VrmAssetListObjectBP"));
}

#if WITH_EDITOR
void UVrmRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) {

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVrmRuntimeSettings, bAllowAllAssimpFormat)
		&& bAllowAllAssimpFormat)
	{
		FString FullPath = FPaths::ConvertRelativePathToFull(GetDefaultConfigFilename());
		FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*FullPath, false);

		auto DialogText(LOCTEXT("VRM4UImportWarning", "Please check the license of the model before using it.\n\nAssImp対応フォーマットのインポートを有効化しました。\nモデルのライセンスに従ってご利用ください。"));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
	}
}
#endif
