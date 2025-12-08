// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "ContentBrowserMenuContexts.h"


#include "VrmAssetListObject.h"

#include "AssetDefinition_VRMAssetList.generated.h"


UCLASS()
class VRM4UIMPORTER_API UAssetDefinition_VRMAssetList : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetAssetColor() const override { return FLinearColor( FColor(255, 223, 255) ); }
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_VRMAssetList", "VRM4U Base"); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UVrmAssetListObject::StaticClass(); }
	//virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override;
	//virtual bool GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const override;

	virtual UThumbnailInfo* LoadThumbnailInfo(const FAssetData& InAssetData) const override;
};
