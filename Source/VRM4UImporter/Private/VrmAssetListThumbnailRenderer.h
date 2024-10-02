// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ThumbnailRendering/TextureThumbnailRenderer.h"
#include "Misc/EngineVersionComparison.h"

#include "AssetTypeActions_Base.h"
#include "VrmAssetListThumbnailRenderer.generated.h"

class FCanvas;
class FRenderTarget;

class FAssetTypeActions_VrmBase : public FAssetTypeActions_Base {

	virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }
	virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& AssetData) const override;
	virtual FColor GetTypeColor() const override { return FColor(255, 223, 255); }
};


class FAssetTypeActions_VrmAssetList : public FAssetTypeActions_VrmBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual bool IsImportedAsset() const override { return true; }
};
class FAssetTypeActions_VrmLicense : public FAssetTypeActions_VrmBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
};
class FAssetTypeActions_Vrm1License : public FAssetTypeActions_VrmBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
};
class FAssetTypeActions_VrmMeta : public FAssetTypeActions_VrmBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
};



UCLASS()
class UVrmAssetListThumbnailRenderer : public UTextureThumbnailRenderer
{
	GENERATED_UCLASS_BODY()

	// UThumbnailRenderer interface
	virtual void GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const override;

#if	UE_VERSION_OLDER_THAN(4,25,0)
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas) override;
#else
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas, bool bAdditionalViewFamily) override;
#endif
	// End of UThumbnailRenderer interface

#if	UE_VERSION_OLDER_THAN(5,5,0)
#else
	virtual bool CanVisualizeAsset(UObject* Object) override;
#endif

protected:
	//void DrawFrame(class UPaperSprite* Sprite, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas, FBoxSphereBounds* OverrideRenderBounds);

	//void DrawGrid(int32 X, int32 Y, uint32 Width, uint32 Height, FCanvas* Canvas);
	UPROPERTY()
	class USkeletalMeshThumbnailRenderer *meshThumbnail = nullptr;
};
