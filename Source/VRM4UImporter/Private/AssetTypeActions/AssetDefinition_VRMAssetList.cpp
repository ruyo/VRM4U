// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "AssetDefinition_VRMAssetList.h"
#include "ContentBrowserMenuContexts.h"
#include "ToolMenus.h"
#include "Editor.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"

#include "ThumbnailRendering/SceneThumbnailInfo.h"

#include "VrmMetaObject.h"
#include "VrmLicenseObject.h"
#include "Vrm1LicenseObject.h"
#include "VrmAssetListObject.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


UThumbnailInfo* UAssetDefinition_VRMAssetList::LoadThumbnailInfo(const FAssetData& InAssetData) const
{
	return UE::Editor::FindOrCreateThumbnailInfo(InAssetData.GetAsset(), USceneThumbnailInfo::StaticClass());
}

//TSharedPtr<SWidget> UAssetDefinition_VRMAssetList::GetThumbnailOverlay(const FAssetData& AssetData) const {
//	return nullptr;
//}



//bool UAssetDefinition_VRMAssetList::GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const
//{
//	return false;
//}


#undef LOCTEXT_NAMESPACE
