// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "ContentBrowserMenuContexts.h"

#include "VrmMetaObject.h"

#include "AssetDefinition_VRMAssetList.h"
#include "AssetDefinition_VRMMeta.generated.h"


UCLASS()
class VRM4UIMPORTER_API UAssetDefinition_VRMMeta : public UAssetDefinition_VRMAssetList
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetAssetColor() const override { return UAssetDefinition_VRMAssetList::GetAssetColor(); }
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_VRMMeta", "VRM4U Meta"); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UVrmMetaObject::StaticClass(); }
	//virtual bool GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const override {
	//	return UAssetDefinition_VRMAssetList::GetThumbnailActionOverlay(InAssetData, OutActionOverlayInfo);
	//}
};
