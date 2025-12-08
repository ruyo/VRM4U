// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "ContentBrowserMenuContexts.h"

#include "Vrm1LicenseObject.h"

#include "AssetDefinition_VRMAssetList.h"
#include "AssetDefinition_VRM1License.generated.h"


UCLASS()
class VRM4UIMPORTER_API UAssetDefinition_VRM1License : public UAssetDefinition_VRMAssetList
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetAssetColor() const override { return UAssetDefinition_VRMAssetList::GetAssetColor(); }
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_VRM1License", "VRM4U License"); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UVrm1LicenseObject::StaticClass(); }
	//virtual bool GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const override {
	//	return UAssetDefinition_VRMAssetList::GetThumbnailActionOverlay(InAssetData, OutActionOverlayInfo);
	//}
};
