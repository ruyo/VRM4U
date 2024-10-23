// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmAssetListThumbnailRenderer.h"
#include "Engine/EngineTypes.h"
#include "CanvasItem.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "CanvasTypes.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Brushes/SlateColorBrush.h"
#include "EditorFramework/AssetImportData.h"

#include "ThumbnailRendering/SkeletalMeshThumbnailRenderer.h"

#include "VrmAssetListObject.h"
#include "VrmLicenseObject.h"
#include "Vrm1LicenseObject.h"
#include "VrmMetaObject.h"

//////////////////////////////////////////////////////////////////////////
// UPaperSpriteThumbnailRenderer

UClass* FAssetTypeActions_VrmAssetList::GetSupportedClass() const {
	return UVrmAssetListObject::StaticClass();
}
FText FAssetTypeActions_VrmAssetList::GetName() const {
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_VrmAssetList", "Vrm Asset List");
}

UClass* FAssetTypeActions_VrmLicense::GetSupportedClass() const {
	return UVrmLicenseObject::StaticClass();
}
FText FAssetTypeActions_VrmLicense::GetName() const {
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_VrmLicense", "Vrm License");
}

UClass* FAssetTypeActions_Vrm1License::GetSupportedClass() const {
	return UVrm1LicenseObject::StaticClass();
}
FText FAssetTypeActions_Vrm1License::GetName() const {
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_Vrm1License", "Vrm1 License");
}

UClass* FAssetTypeActions_VrmMeta::GetSupportedClass() const {
	return UVrmMetaObject::StaticClass();
}
FText FAssetTypeActions_VrmMeta::GetName() const {
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_VrmMeta", "Vrm Meta");
}

TSharedPtr<SWidget> FAssetTypeActions_VrmBase::GetThumbnailOverlay(const FAssetData& AssetData) const {

	FString str;
	FColor col(0, 0, 0, 0);

	if (str.Len() == 0) {
		TWeakObjectPtr<UVrmAssetListObject> a = Cast<UVrmAssetListObject>(AssetData.GetAsset());
		if (a.Get()) {
			str = TEXT(" AssetList ");
		}
	}
	if (str.Len() == 0){
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
			.HighlightColor(FColor(64,64,64))
			//.ShadowOffset(FVector2D(1.0f, 1.0f))
		];

	//return FAssetTypeActions_Base::GetThumbnailOverlay(AssetData);
}


UVrmAssetListThumbnailRenderer::UVrmAssetListThumbnailRenderer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UVrmAssetListThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const {
	UVrmAssetListObject* a = Cast<UVrmAssetListObject>(Object);

	if (a) {
		if (a->VrmLicenseObject) {
			auto tex = a->SmallThumbnailTexture;
			if (tex == nullptr) {
				tex = a->VrmLicenseObject->thumbnail;
			}
			if (tex) {
				return Super::GetThumbnailSize(tex, Zoom, OutWidth, OutHeight);
			}
		}
		if (a->Vrm1LicenseObject) {
			auto tex = a->SmallThumbnailTexture;
			if (tex == nullptr) {
				tex = a->Vrm1LicenseObject->thumbnail;
			}
			if (tex) {
				return Super::GetThumbnailSize(tex, Zoom, OutWidth, OutHeight);
		}
	}
}
	Super::GetThumbnailSize(Object, Zoom, OutWidth, OutHeight);
}


#if	UE_VERSION_OLDER_THAN(4,25,0)
void UVrmAssetListThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
#else
void UVrmAssetListThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
#endif
{
	UTexture2D *tex = nullptr;
	USkeletalMesh *sk = nullptr;

	if (tex == nullptr){
		UVrmAssetListObject* a = Cast<UVrmAssetListObject>(Object);
		if (a) {
			tex = a->SmallThumbnailTexture;
			if (tex == nullptr) {
				if (a->VrmLicenseObject) {
					tex = a->VrmLicenseObject->thumbnail;
				}
				if (a->Vrm1LicenseObject) {
					tex = a->Vrm1LicenseObject->thumbnail;
				}
			}
			if (a->SkeletalMesh) {
				sk = a->SkeletalMesh;
			}
		}
	}
	if (tex == nullptr) {
		TArray<UObject*> ret;
		{
			UVrmMetaObject* a = Cast<UVrmMetaObject>(Object);
			if (a) {
				sk = a->SkeletalMesh;
				if (a->VrmAssetListObject) {
					tex = a->VrmAssetListObject->SmallThumbnailTexture;
				}
			}
		}
		{
			UVrmLicenseObject* a = Cast<UVrmLicenseObject>(Object);
			if (a) {
				UPackage *pk = a->GetOutermost();
				GetObjectsWithOuter(pk, ret);
				// no sk
				tex = a->thumbnail;
			}
		}
		{
			UVrm1LicenseObject* a = Cast<UVrm1LicenseObject>(Object);
			if (a) {
				UPackage* pk = a->GetOutermost();
				GetObjectsWithOuter(pk, ret);
				// no sk
				tex = a->thumbnail;
			}
		}

		for (auto *obj : ret) {
			UVrmAssetListObject* t = Cast<UVrmAssetListObject>(obj);
			if (t == nullptr) {
				continue;
			}
			sk = t->SkeletalMesh;
			if (t->SmallThumbnailTexture) tex = t->SmallThumbnailTexture;
			break;
		}
	}


	// skeleton thumbnail
	if (tex == nullptr) {
		if (IsValid(meshThumbnail) == false) {
			meshThumbnail = NewObject<USkeletalMeshThumbnailRenderer>(this);
		}
		if (sk) {
			if (VRMGetSkeleton(sk)) {
#if	UE_VERSION_OLDER_THAN(4,25,0)
				meshThumbnail->Draw((UObject*)(sk), X, Y, Width, Height, RenderTarget, Canvas);
#else
				meshThumbnail->Draw((UObject*)(sk), X, Y, Width, Height, RenderTarget, Canvas, bAdditionalViewFamily);
#endif
				return;
			}
		}
	}

	auto obj = Object;

	if (tex) {
		obj = tex;
	}

#if	UE_VERSION_OLDER_THAN(4,25,0)
	return Super::Draw(obj, X, Y, Width, Height, RenderTarget, Canvas);
#else
	return Super::Draw(obj, X, Y, Width, Height, RenderTarget, Canvas, bAdditionalViewFamily);
#endif

}

#if	UE_VERSION_OLDER_THAN(5,5,0)
#else
bool UVrmAssetListThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{

	if (UVrmLicenseObject* a = Cast<UVrmLicenseObject>(Object)) {
		if (a->thumbnail == nullptr) {
			return false;
		}
	}
	if (UVrm1LicenseObject* a = Cast<UVrm1LicenseObject>(Object)) {
		if (a->thumbnail == nullptr) {
			return false;
		}
	}

	return true;
	//return UTextureThumbnailRenderer::CanVisualizeAsset(Object);
}
#endif

