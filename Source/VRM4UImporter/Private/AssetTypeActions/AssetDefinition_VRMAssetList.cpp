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

TSharedPtr<SWidget> UAssetDefinition_VRMAssetList::GetThumbnailOverlay(const FAssetData& AssetData) const {
	return nullptr;
	/*
	FString str;
	FColor col(0, 0, 0, 0);

	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmAssetListObject> a = Cast<UVrmAssetListObject>(AssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" AssetList ");
		}
	}
	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmLicenseObject> a = Cast<UVrmLicenseObject>(AssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" License ");
			col.A = 128;
		}
	}
	if (str.Len() == 0) {
		TWeakObjectPtr<UVrm1LicenseObject> a = Cast<UVrm1LicenseObject>(AssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" License ");
			col.A = 128;
		}
	}
	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmMetaObject> a = Cast<UVrmMetaObject>(AssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" Meta ");
			col.A = 128;
		}
	}

	FText txt = FText::FromString(str);
	return SNew(SBorder)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		//.Padding(FMargin(4))
		//.Padding(FMargin(4))
		//.BorderImage(new FSlateColorBrush(FColor::White))
		.BorderImage(new FSlateColorBrush(col))
		//.AutoWidth()
		[
			SNew(STextBlock)
				.Text(txt)
				.HighlightText(txt)
				.HighlightColor(FColor(64, 64, 64))
				//.ShadowOffset(FVector2D(1.0f, 1.0f))
		];

	//return FAssetTypeActions_Base::GetThumbnailOverlay(AssetData);
	*/
}



bool UAssetDefinition_VRMAssetList::GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const
{
	return false;

	/*
	FString str;
	FColor col(0, 0, 0, 0);

	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmAssetListObject> a = Cast<UVrmAssetListObject>(InAssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" AssetList ");
		}
	}
	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmLicenseObject> a = Cast<UVrmLicenseObject>(InAssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" License ");
			col.A = 128;
		}
	}
	if (str.Len() == 0) {
		TWeakObjectPtr<UVrm1LicenseObject> a = Cast<UVrm1LicenseObject>(InAssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" License ");
			col.A = 128;
		}
	}
	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmMetaObject> a = Cast<UVrmMetaObject>(InAssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" Meta ");
			col.A = 128;
		}
	}

	FText txt = FText::FromString(str);
	OutActionOverlayInfo.ActionImageWidget = SNew(STextBlock)
				.Text(txt)
				.HighlightText(txt)
				.HighlightColor(FColor(64, 64, 64))
				//.ShadowOffset(FVector2D(1.0f, 1.0f))
		;

	//OutActionOverlayInfo.ActionButtonWidget = nullptr;
	//OutActionOverlayInfo.ActionButtonArgs.

	return true;
*/

}


#undef LOCTEXT_NAMESPACE
