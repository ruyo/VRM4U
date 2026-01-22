// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertTexture.h"
#include "VrmConvert.h"
#include "VrmUtil.h"

#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "PixelFormat.h"
#include "RenderUtils.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Engine/SubsurfaceProfile.h"
#include "Materials/MaterialInstanceConstant.h"
#include "VrmAssetListObject.h"
#include "Async/ParallelFor.h"
#include "UObject/UObjectHash.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "Factories.h"
#include "Factories/TextureFactory.h"

#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>

#if	UE_VERSION_OLDER_THAN(4,23,0)
#define TRACE_CPUPROFILER_EVENT_SCOPE(a)
#else
#endif


namespace {

	bool bDefaultVrmMaterial = false;
	bool LocalIsOriginalVrmMaterial() {
		return bDefaultVrmMaterial;
	}
	void LocalSetOriginalVrmMaterial(bool b) {
		bDefaultVrmMaterial = b;
	}

	void LocalMaterialSetParent(UMaterialInstanceConstant *material, UMaterialInterface *parent) {
#if WITH_EDITOR
		if (VRMConverter::IsImportMode()) {
			material->SetParentEditorOnly(parent);
		} else {
			material->Parent = parent;
		}
		FMaterialUpdateContext UpdateContext(FMaterialUpdateContext::EOptions::Default, GMaxRHIShaderPlatform);
		UpdateContext.AddMaterialInstance(material);
#else
		material->Parent = parent;
#endif
	}

	void LocalTextureSet(UMaterialInstanceConstant *dm, FName name, UTexture2D * tex) {
#if WITH_EDITOR
		if (VRMConverter::IsImportMode()) {
			dm->SetTextureParameterValueEditorOnly(name, tex);
		}else{
			FTextureParameterValue *v = new (dm->TextureParameterValues) FTextureParameterValue();
			v->ParameterInfo.Index = INDEX_NONE;
			v->ParameterInfo.Name = name;
			v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
			v->ParameterValue = tex;
		}
#else
		FTextureParameterValue *v = new (dm->TextureParameterValues) FTextureParameterValue();
		v->ParameterInfo.Index = INDEX_NONE;
		v->ParameterInfo.Name = name;
		v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
		v->ParameterValue = tex;
#endif
	}

	void LocalScalarParameterSet(UMaterialInstanceConstant *dm, FName name, float f) {
		bool bSet = false;
#if WITH_EDITOR
		if (VRMConverter::IsImportMode()) {
			dm->SetScalarParameterValueEditorOnly(name, f);
			bSet = true;
		}
#endif
		if (bSet == false) {
			FScalarParameterValue *v = nullptr;
			for (auto &a : dm->ScalarParameterValues) {
				if (a.ParameterInfo.Name == name) {
					v = &a;
				}
			}
			if (v == nullptr) {
				v = new (dm->ScalarParameterValues) FScalarParameterValue();
			}
			v->ParameterInfo.Index = INDEX_NONE;
			v->ParameterInfo.Name = name;
			v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
			v->ParameterValue = f;
		}
	}

	void LocalVectorParameterSet(UMaterialInstanceConstant *dm, FName name, FLinearColor c) {
		bool bSet = false;
#if WITH_EDITOR
		if (VRMConverter::IsImportMode()) {
			dm->SetVectorParameterValueEditorOnly(name, c);
			bSet = true;
		}
#endif
		if (bSet == false) {
			FVectorParameterValue *v = nullptr;
			for (auto &a : dm->VectorParameterValues) {
				if (a.ParameterInfo.Name == name) {
					v = &a;
				}
			}
			if (v == nullptr) {
				v = new (dm->VectorParameterValues) FVectorParameterValue();
			}

			v->ParameterInfo.Index = INDEX_NONE;
			v->ParameterInfo.Name = name;
			v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
			v->ParameterValue = c;
		}
	}


	void LocalMaterialFinishParam(UMaterialInstanceConstant *material) {
#if WITH_EDITOR
		if (VRMConverter::IsImportMode()) {
			material->PreEditChange(NULL);
			material->PostEditChange();
		} else {
			material->PostLoad();
		}
#else
		material->PostLoad();
#endif
	}


	void createSmallThumbnail(UVrmAssetListObject *vrmAssetList, const aiScene *aiData) {
#if WITH_EDITORONLY_DATA
		UTexture2D *src = nullptr;

		VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(aiData->mVRMMeta);
		if (meta == nullptr) {
			return;
		}

		int TextureID = -1;
		for (int i = 0; i < meta->license.licensePairNum; ++i) {
			auto &p = meta->license.licensePair[i];

			if (FString(TEXT("texture")) == p.Key.C_Str()) {

				int t = FCString::Atoi(*FString(p.Value.C_Str()));
				if (t >= 0 && t < vrmAssetList->Textures.Num()) {
					src = vrmAssetList->Textures[t];
					TextureID = t;
					break;
				}
			}
		}
		if (src == nullptr) {
			// todo small texture vrm1license
		}
		if (src == nullptr) {
			return;
		}

		
		const int W = src->GetSurfaceWidth();
		const int H = src->GetSurfaceHeight();

		int dW = FMath::Min(256, W);
		int dH = FMath::Min(256, H);

		vrmAssetList->SmallThumbnailTexture = src;
		if (W == dW && H == dH) {
			return;
		}
		if (W != H) {
			return;
		}
		if (TextureID < 0 || TextureID >= (int)aiData->mNumTextures) {
			return;
		}
		if (aiData->mTextures[TextureID] == nullptr) {
			return;
		}


		TArray<uint8> sData;
		sData.SetNum(W * H * 4);
		TArray<uint8> dData;
		dData.SetNum(256*256 * 4);

		FString baseName = (src->GetFName()).ToString();
		baseName += TEXT("_small");

		{
			VRMUtil::FImportImage img;
			auto *a = aiData->mTextures[TextureID];
			if (VRMLoaderUtil::LoadImageFromMemory(a->pcData, a->mWidth, img) == false) {
				return;
			}
			if (img.RawData.GetData() == nullptr) {
				return;
			}
			FMemory::Memcpy(sData.GetData(), img.RawData.GetData(), img.RawData.Num());
		}


		auto* pkg = vrmAssetList->Package;
		if (VRMConverter::Options::Get().IsSingleUAssetFile() == false) {
			pkg = VRM4U_CreatePackage(vrmAssetList->Package, *baseName);
		}
		UTexture2D* NewTexture2D = VRMLoaderUtil::CreateTexture(W, H, baseName, pkg);

		// scale texture
		{
			ParallelFor(dH, [&](int32 y){
			//for (int32 y = 0; y < dH; y++){
				float s = (float)H / dH;

				for (int32 x = 0; x < dW; x++){
					int32 xx = (s * (0.5f + x));
					int32 yy = (s * (0.5f + y));
					uint8* dp = &dData[(y * dW + x) * sizeof(uint8) * 4];

					int c = 0;
					int tmp[4] = {};
					int n = (float)W / dW / 2;
					for (int ry = yy - n; ry <= yy + n; ry++) {
						for (int rx = xx - n; rx <= xx + n; rx++) {
							if (rx < 0 || ry < 0) continue;
							if (rx >= W || ry >= H) continue;

							const uint8* rp = &sData[(ry * W + rx) * sizeof(uint8) * 4];
							tmp[0] += rp[0];
							tmp[1] += rp[1];
							tmp[2] += rp[2];
							tmp[3] += rp[3];

							++c;
						}
					}
					dp[0] = tmp[0] / c;
					dp[1] = tmp[1] / c;
					dp[2] = tmp[2] / c;
					dp[3] = tmp[3] / c;
				}
			});

			// Set options
			NewTexture2D->SRGB = true;// bUseSRGB;
			NewTexture2D->CompressionSettings = TC_Default;

			NewTexture2D->AddressX = TA_Wrap;
			NewTexture2D->AddressY = TA_Wrap;

			NewTexture2D->CompressionNone = false;
			NewTexture2D->DeferCompression = true;
			if (VRMConverter::Options::Get().IsMipmapGenerateMode() && FMath::IsPowerOfTwo(dW) && FMath::IsPowerOfTwo(dH)) {
				NewTexture2D->MipGenSettings = TMGS_FromTextureGroup;
			} else {
				NewTexture2D->MipGenSettings = TMGS_NoMipmaps;
			}
			NewTexture2D->Source.Init(dW, dH, 1, 1, ETextureSourceFormat::TSF_BGRA8, dData.GetData());
			//NewTexture2D->Source.Compress();

			// Update the remote texture data
			NewTexture2D->UpdateResource();
			NewTexture2D->PostEditChange();
			vrmAssetList->SmallThumbnailTexture = NewTexture2D;
		}
#endif
	}

	bool createAndAddMaterial(UMaterialInstanceConstant *dm, int matIndex, UVrmAssetListObject *vrmAssetList, const VRMConverter *vc,
		const TArray<int> &TextureTypeToIndex
	) {
		auto i = matIndex;

		// default set function
		auto SetLocalParamsForVector = [](auto* dm) {
			LocalVectorParameterSet(dm, TEXT("mtoon_Color"), FLinearColor(1, 1, 1, 1));
			LocalVectorParameterSet(dm, TEXT("mtoon_ShadeColor"), FLinearColor(1, 1, 1, 1));
			LocalVectorParameterSet(dm, TEXT("mtoon_OutlineColor"), FLinearColor(0, 0, 0, 1));
		};
		auto SetLocalParamsForScaler = [](auto* dm) {
			LocalScalarParameterSet(dm, TEXT("mtoon_BumpScale"), 1.f);
			LocalScalarParameterSet(dm, TEXT("mtoon_NormalScale"), 1.f);
			LocalScalarParameterSet(dm, TEXT("mtoon_ReceiveShadowRate"), 1.f);

			LocalScalarParameterSet(dm, TEXT("mtoon_OutlineLightingMix"), 1.f);
			LocalScalarParameterSet(dm, TEXT("mtoon_OutlineWidth"), 0.1f);
			LocalScalarParameterSet(dm, TEXT("mtoon_OutlineWidthMode"), 1.f);
		};

		// default for not vrm material
		if (dm) {
			if (LocalIsOriginalVrmMaterial() == false) {
				SetLocalParamsForVector(dm);
				SetLocalParamsForScaler(dm);
			}
		}

		// VRM1
		if (VRMConverter::Options::Get().IsVRM10Model()) {
			LocalScalarParameterSet(dm, TEXT("bVRM10Mode"), 1.f);
		}

		VRM::VRMMaterial vrmMat;
		if (vc->GetMatParam(vrmMat, i) == false) {
			return false;
		}
		{
			struct TT {
				FString key;
				float* value;
			};
			TT tableParam[] = {
				{TEXT("_Color"),			vrmMat.vectorProperties._Color},
				{TEXT("_ShadeColor"),	vrmMat.vectorProperties._ShadeColor},
				{TEXT("_MainTex"),		vrmMat.vectorProperties._MainTex},
				{TEXT("_ShadeTexture"),	vrmMat.vectorProperties._ShadeTexture},
				{TEXT("_BumpMap"),				vrmMat.vectorProperties._BumpMap},
				{TEXT("_ReceiveShadowTexture"),	vrmMat.vectorProperties._ReceiveShadowTexture},
				{TEXT("_ShadingGradeTexture"),		vrmMat.vectorProperties._ShadingGradeTexture},
				{TEXT("_RimColor"),					vrmMat.vectorProperties._RimColor},
				{TEXT("_RimTexture"),				vrmMat.vectorProperties._RimTexture},
				{TEXT("_SphereAdd"),				vrmMat.vectorProperties._SphereAdd},
				{TEXT("_EmissionColor"),			vrmMat.vectorProperties._EmissionColor},
				{TEXT("_EmissionMap"),			vrmMat.vectorProperties._EmissionMap},
				{TEXT("_OutlineWidthTexture"),	vrmMat.vectorProperties._OutlineWidthTexture},
				{TEXT("_OutlineColor"),			vrmMat.vectorProperties._OutlineColor},
				{TEXT("_UvAnimMaskTexture"),	vrmMat.vectorProperties._UvAnimMaskTexture},
			};
			for (auto &t : tableParam) {
				LocalVectorParameterSet(dm, *(TEXT("mtoon") + t.key), FLinearColor(t.value[0], t.value[1], t.value[2], t.value[3]));
			}

			// default for not vrm material
			if (LocalIsOriginalVrmMaterial() == false) {
				SetLocalParamsForVector(dm);
			}
		}
		{
			struct TT {
				FString key;
				float& value;
			};
			TT tableParam[] = {
				{TEXT("_Cutoff"),		vrmMat.floatProperties._Cutoff},
				{TEXT("_BumpScale"),	vrmMat.floatProperties._BumpScale},
				{TEXT("_NormalScale"),	vrmMat.floatProperties._BumpScale},	// VRM4U Custom
				{TEXT("_ReceiveShadowRate"),	vrmMat.floatProperties._ReceiveShadowRate},
				{TEXT("_ShadeShift"),			vrmMat.floatProperties._ShadeShift},
				{TEXT("_ShadeToony"),			vrmMat.floatProperties._ShadeToony},
				{TEXT("_LightColorAttenuation"),	vrmMat.floatProperties._LightColorAttenuation},
				{TEXT("_IndirectLightIntensity"),	vrmMat.floatProperties._IndirectLightIntensity},
				{TEXT("_RimLightingMix"),			vrmMat.floatProperties._RimLightingMix},
				{TEXT("_RimFresnelPower"),			vrmMat.floatProperties._RimFresnelPower},
				{TEXT("_RimLift"),					vrmMat.floatProperties._RimLift},
				{TEXT("_OutlineWidth"),			vrmMat.floatProperties._OutlineWidth},
				{TEXT("_OutlineScaledMaxDistance"),	vrmMat.floatProperties._OutlineScaledMaxDistance},
				{TEXT("_OutlineLightingMix"),			vrmMat.floatProperties._OutlineLightingMix},
				{TEXT("_UvAnimScrollX"),			vrmMat.floatProperties._UvAnimScrollX},
				{TEXT("_UvAnimScrollY"),			vrmMat.floatProperties._UvAnimScrollY},
				{TEXT("_UvAnimRotation"),			vrmMat.floatProperties._UvAnimRotation},
				{TEXT("_MToonVersion"),				vrmMat.floatProperties._MToonVersion},
				{TEXT("_DebugMode"),				vrmMat.floatProperties._DebugMode},
				{TEXT("_BlendMode"),				vrmMat.floatProperties._BlendMode},
				{TEXT("_OutlineWidthMode"),		vrmMat.floatProperties._OutlineWidthMode},
				{TEXT("_OutlineColorMode"),	vrmMat.floatProperties._OutlineColorMode},
				{TEXT("_CullMode"),			vrmMat.floatProperties._CullMode},
				{TEXT("_OutlineCullMode"),		vrmMat.floatProperties._OutlineCullMode},
				{TEXT("_SrcBlend"),			vrmMat.floatProperties._SrcBlend},
				{TEXT("_DstBlend"),			vrmMat.floatProperties._DstBlend},
				{TEXT("_ZWrite"),			vrmMat.floatProperties._ZWrite},
			};

			for (auto &t : tableParam) {
				LocalScalarParameterSet(dm, *(TEXT("mtoon") + t.key), t.value);
			}

			// default for not vrm material
			if (LocalIsOriginalVrmMaterial() == false) {
				SetLocalParamsForScaler(dm);
			}

			//if (vrmMat.floatProperties._CullMode == 0.f) {
			//	dm->BasePropertyOverrides.bOverride_TwoSided = true;
			//	dm->BasePropertyOverrides.TwoSided = 1;
			//}
			if (vrmMat.floatProperties._Cutoff != 0.f) {
				dm->BasePropertyOverrides.bOverride_OpacityMaskClipValue = true;
				dm->BasePropertyOverrides.OpacityMaskClipValue = vrmMat.floatProperties._Cutoff;
			}

			{
				switch ((int)(vrmMat.floatProperties._BlendMode)) {
				case 0:
				case 1:
					break;
				case 2:
				case 3:
					//dm->BasePropertyOverrides.bOverride_BlendMode = true;
					//dm->BasePropertyOverrides.BlendMode = EBlendMode::BLEND_Translucent;
					break;
				}
			}
		}
		{
			struct TT {
				FString key;
				int value;
			};
			TT tableParam[] = {
				{TEXT("mtoon_tex_MainTex"),		vrmMat.textureProperties._MainTex},
				{TEXT("mtoon_tex_ShadeTexture"),	vrmMat.textureProperties._ShadeTexture},
				{TEXT("mtoon_tex_Shade"),			vrmMat.textureProperties._ShadeTexture},	// vrm1
				{TEXT("mtoon_tex_BumpMap"),		vrmMat.textureProperties._BumpMap},
				{TEXT("mtoon_tex_ReceiveShadowTexture"),	vrmMat.textureProperties._ReceiveShadowTexture},
				{TEXT("mtoon_tex_ShadingGradeTexture"),	vrmMat.textureProperties._ShadingGradeTexture},
				{TEXT("mtoon_tex_RimTexture"),			vrmMat.textureProperties._RimTexture},
				{TEXT("mtoon_tex_RimMultiply"),					vrmMat.textureProperties._RimTexture},	// vrm1
				{TEXT("mtoon_tex_SphereAdd"),	vrmMat.textureProperties._SphereAdd},
				{TEXT("mtoon_tex_MatCap"),		vrmMat.textureProperties._SphereAdd},	// vrm1
				{TEXT("mtoon_tex_EmissionMap"),	vrmMat.textureProperties._EmissionMap},
				{TEXT("mtoon_tex_Emissive"),	vrmMat.textureProperties._EmissionMap},	// vrm1
				{TEXT("mtoon_tex_OutlineWidthTexture"),			vrmMat.textureProperties._OutlineWidthTexture},
				{TEXT("mtoon_tex_OutlineWidthMultiply"),		vrmMat.textureProperties._OutlineWidthTexture},		// vrm1
				{TEXT("mtoon_tex_UvAnimMaskTexture")	,	vrmMat.textureProperties._UvAnimMaskTexture},
				{TEXT("mtoon_tex_UvAnimationMask"),			vrmMat.textureProperties._UvAnimMaskTexture},		// vrm1
			};

			if (VRMConverter::Options::Get().IsVRM10Model()) {
				FSoftObjectPath r(TEXT("/VRM4U/MaterialUtil/T_DummyBlack.T_DummyBlack"));
				UObject* u = r.TryLoad();
				if (u) {
					auto r2 = Cast<UTexture2D>(u);
					if (r2) {
						LocalTextureSet(dm, "mtoon_tex_EmissionMap", r2);
					}
				}
			}

			// default texture
			{
				auto n = TextureTypeToIndex[aiTextureType_DIFFUSE];
				if (n >= 0) {
					LocalTextureSet(dm, TEXT("mtoon_tex_MainTex"), vrmAssetList->Textures[n]);
					LocalTextureSet(dm, TEXT("gltf_tex_diffuse"), vrmAssetList->Textures[n]);
					LocalTextureSet(dm, TEXT("mtoon_tex_Shade"), vrmAssetList->Textures[n]);
				}
			}

			// mtoon texture
			int count = 0;
			for (auto &t : tableParam) {
				++count;
				if (t.value < 0 || t.value >= vrmAssetList->Textures.Num()) {
					continue;
				}
				LocalTextureSet(dm, *t.key, vrmAssetList->Textures[t.value]);
				if (count == 1) {
					// main => shade tex
					LocalTextureSet(dm, *tableParam[1].key, vrmAssetList->Textures[t.value]);
					LocalTextureSet(dm, *tableParam[2].key, vrmAssetList->Textures[t.value]);
				}

				//FTextureParameterValue *v = new (dm->TextureParameterValues) FTextureParameterValue();
				//v->ParameterInfo.Index = INDEX_NONE;
				//v->ParameterInfo.Name = *t.key;
				//v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
				//v->ParameterValue = vrmAssetList->Textures[t.value];
			}

			// gltf default texture
			{
				auto n = TextureTypeToIndex[aiTextureType_NORMALS];
				if (0 <= n && n < vrmAssetList->Textures.Num()) {
					vrmAssetList->Textures[n]->SRGB = false;
					vrmAssetList->Textures[n]->CompressionSettings = TC_Normalmap;
					vrmAssetList->Textures[n]->UpdateResource();
					LocalTextureSet(dm, TEXT("mtoon_tex_Normal"), vrmAssetList->Textures[n]);
				}
			}
			{
				auto n = TextureTypeToIndex[aiTextureType_EMISSIVE];
				if (0 <= n && n < vrmAssetList->Textures.Num()) {
					LocalTextureSet(dm, TEXT("mtoon_tex_Emissive"), vrmAssetList->Textures[n]);
				}
			}


		}

		return true;
	}

	UMaterial* CreateDefaultMaterial(UVrmAssetListObject *vrmAssetList) {

#if	UE_VERSION_OLDER_THAN(4,20,0)
		UMaterial* UnrealMaterial = VRM4U_NewObject<UMaterial>(vrmAssetList->Package, TEXT("M_BaseMaterial"), RF_Standalone | RF_Public);
#else
		UMaterial* UnrealMaterial = nullptr;
		if (vrmAssetList->Package == GetTransientPackage()) {
			UnrealMaterial = VRM4U_NewObject<UMaterial>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient, nullptr);
		}
		else {
			UnrealMaterial = VRM4U_NewObject<UMaterial>(vrmAssetList->Package, TEXT("M_BaseMaterial"), RF_Standalone | RF_Public, nullptr);
		}
#endif

		if (UnrealMaterial != NULL)
		{
			//UnrealMaterialFinal = UnrealMaterial;
			// Notify the asset registry
			//FAssetRegistryModule::AssetCreated(UnrealMaterial);

			if(true)
			{
				bool bNeedsRecompile = true;
				UnrealMaterial->GetMaterial()->SetMaterialUsage(bNeedsRecompile, MATUSAGE_SkeletalMesh);
				UnrealMaterial->GetMaterial()->SetMaterialUsage(bNeedsRecompile, MATUSAGE_MorphTargets);
			}

			// Set the dirty flag so this package will get saved later
			//Package->SetDirtyFlag(true);

			// textures and properties

			{
#if WITH_EDITORONLY_DATA
				UMaterialExpressionTextureSampleParameter2D* UnrealTextureExpression = NewObject<UMaterialExpressionTextureSampleParameter2D>(UnrealMaterial);

#if	UE_VERSION_OLDER_THAN(5,1,0)
				UnrealMaterial->Expressions.Add(UnrealTextureExpression);
#else
				UnrealMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(UnrealTextureExpression);
#endif
				
				UnrealTextureExpression->SamplerType = SAMPLERTYPE_Color;
				UnrealTextureExpression->ParameterName = TEXT("gltf_tex_diffuse");

#if	UE_VERSION_OLDER_THAN(5,1,0)
				UnrealMaterial->BaseColor.Expression = UnrealTextureExpression;
#else
				UnrealMaterial->GetEditorOnlyData()->BaseColor.Expression = UnrealTextureExpression;
#endif

#endif
			}

			UnrealMaterial->BlendMode = BLEND_Masked;
			UnrealMaterial->SetShadingModel(MSM_DefaultLit);
		}
		return UnrealMaterial;
	}


	bool isSameMaterial(const UMaterialInterface *mi1, const UMaterialInterface *mi2) {
		const UMaterialInstanceConstant *m1 = Cast<UMaterialInstanceConstant>(mi1);
		const UMaterialInstanceConstant *m2 = Cast<UMaterialInstanceConstant>(mi2);

		if (m1 == nullptr || m2 == nullptr) {
			return false;
		}
		// tex
		{
			if (m1->TextureParameterValues.Num() != m2->TextureParameterValues.Num()) {
				return false;
			}
			for (int i = 0; i < m1->TextureParameterValues.Num(); ++i) {
				if (m1->TextureParameterValues[i].ParameterValue != m2->TextureParameterValues[i].ParameterValue) {
					return false;
				}
				if (m1->TextureParameterValues[i].ParameterInfo.Name != m2->TextureParameterValues[i].ParameterInfo.Name) {
					return false;
				}
			}
		}

		// scalar
		{
			if (m1->ScalarParameterValues.Num() != m2->ScalarParameterValues.Num()) {
				return false;
			}
			for (int i = 0; i < m1->ScalarParameterValues.Num(); ++i) {
				if (m1->ScalarParameterValues[i].ParameterValue != m2->ScalarParameterValues[i].ParameterValue) {
					return false;
				}
				if (m1->ScalarParameterValues[i].ParameterInfo.Name != m2->ScalarParameterValues[i].ParameterInfo.Name) {
					return false;
				}
			}
		}

		// vector
		{
			if (m1->VectorParameterValues.Num() != m2->VectorParameterValues.Num()) {
				return false;
			}
			for (int i = 0; i < m1->VectorParameterValues.Num(); ++i) {
				if (m1->VectorParameterValues[i].ParameterValue != m2->VectorParameterValues[i].ParameterValue) {
					return false;
				}
				if (m1->VectorParameterValues[i].ParameterInfo.Name != m2->VectorParameterValues[i].ParameterInfo.Name) {
					return false;
				}
			}
		}

		if (m1->BasePropertyOverrides != m2->BasePropertyOverrides) {
			return false;
		}

#define VRM4U_TMP_COMPARE(a) if (m1->a != m2->a) return false;
		VRM4U_TMP_COMPARE(OpacityMaskClipValue);
		VRM4U_TMP_COMPARE(BlendMode);
		VRM4U_TMP_COMPARE(TwoSided);
		VRM4U_TMP_COMPARE(DitheredLODTransition);
		VRM4U_TMP_COMPARE(bCastDynamicShadowAsMasked);
#if	UE_VERSION_OLDER_THAN(4,23,0)
		VRM4U_TMP_COMPARE(GetShadingModel());
#else
		VRM4U_TMP_COMPARE(GetShadingModels());
#endif
#undef VRM4U_TMP_COMPARE

		return true;
	}
}// namespace



bool VRMConverter::ConvertTextureAndMaterial(UVrmAssetListObject *vrmAssetList) {
	if (vrmAssetList == nullptr || aiData == nullptr) {
		return false;
	}
	if (VRMConverter::Options::Get().IsNoMesh()) {
		return true;
	}


	const bool bGenerateMips = VRMConverter::Options::Get().IsMipmapGenerateMode();

	// skip for preload
	// skip vrmAssetList->Textures.Reset(0);
	vrmAssetList->Materials.Reset(0);
	vrmAssetList->OutlineMaterials.Reset(0);

	TArray<bool> NormalBoolTable;
	NormalBoolTable.SetNum(aiData->mNumTextures);

	TArray<bool> MaskBoolTable;
	MaskBoolTable.SetNum(aiData->mNumTextures);

	TArray<UMaterialInterface*> matArray;
	TArray<bool> matFlagTranslucentArray;
	TArray<bool> matFlagTwoSidedArray;
	TArray<bool> matFlagOpaqueArray;
	TArray<EVRMImportTextureCompressType> textureCompressTypeArray;

	{
		const VRM::VRMMetadata *meta = static_cast<const VRM::VRMMetadata*>(aiData->mVRMMeta);
		if (meta) {
			for (int i = 0; i < meta->materialNum; ++i) {
				int t = 0;
				
				t = meta->material[i].textureProperties._BumpMap;
				if (NormalBoolTable.IsValidIndex(t)){
					NormalBoolTable[t] = true;
				}

				t = meta->material[i].textureProperties._OutlineWidthTexture;
				if (MaskBoolTable.IsValidIndex(t)){
					MaskBoolTable[t] = true;
				}
			}
		}
	}


	{
		if (aiData->HasTextures() && vrmAssetList->Textures.Num() != aiData->mNumTextures) {
			TArray<UTexture2D*> texArray;
			texArray.Reserve(aiData->mNumTextures);
			// Note: PNG format.  Other formats are supported

			for (uint32_t i = 0; i < aiData->mNumTextures; ++i) {
				auto& t = *aiData->mTextures[i];
				int Width = t.mWidth;
				int Height = t.mHeight;

				FString baseName = VRMConverter::NormalizeFileName(t.mFilename.C_Str());
				if (baseName.Len() == 0) {
					baseName = TEXT("texture") + FString::FromInt(i);
				}
				bool bNormalGreenFlip = NormalBoolTable[i];
				if (bNormalGreenFlip) {
					baseName += TEXT("_N");
				}

				FString name = FString(TEXT("T_")) + baseName;
				auto* pkg = vrmAssetList->Package;
				if (VRMConverter::Options::Get().IsSingleUAssetFile() == false) {
					pkg = VRM4U_CreatePackage(vrmAssetList->Package, *name);
				}
				UTexture2D* NewTexture2D = VRMLoaderUtil::CreateTextureFromImage(name, pkg, t.pcData, t.mWidth, bGenerateMips, NormalBoolTable[i], bNormalGreenFlip&&(VRMConverter::IsImportMode()==false));
#if WITH_EDITOR
				NewTexture2D->DeferCompression = false;
#endif

				bool bIsBC7 = false;
				// Set options
				if (NormalBoolTable[i]) {
					NewTexture2D->SRGB = 0;
#if WITH_EDITOR
					NewTexture2D->CompressionNoAlpha = true;
					if (VRMConverter::IsImportMode()) {
						NewTexture2D->bFlipGreenChannel = true;
					}
#endif
				} else {
					if (VRMConverter::Options::Get().IsBC7Mode()) {
						NewTexture2D->CompressionSettings = TC_BC7;
						bIsBC7 = true;
					}
				}
				if (MaskBoolTable[i]) {
					// comment for material warning...
					//NewTexture2D->SRGB = 0;
				}

				if (bIsBC7) {
					textureCompressTypeArray.Add(EVRMImportTextureCompressType::VRMITC_BC7);
				} else {
					textureCompressTypeArray.Add(EVRMImportTextureCompressType::VRMITC_DXT1);
				}


				NewTexture2D->UpdateResource();
#if WITH_EDITOR
				NewTexture2D->PostEditChange();
#endif

				if (NormalBoolTable[i]) {
					// UE5.5でクラッシュするので update後に再度更新
					NewTexture2D->CompressionSettings = TC_Normalmap;
					NewTexture2D->UpdateResource();
#if WITH_EDITOR
					NewTexture2D->PostEditChange();
#endif
				}

				texArray.Push(NewTexture2D);
			}
			vrmAssetList->Textures = texArray;

			// small thumbnail
			{
				createSmallThumbnail(vrmAssetList, aiData);
			}
		}
	} // texture

	TArray<FString> pmxTexNameList;
	if (Options::Get().IsPMXModel()) {
		TArray<UTexture2D*> texArray;

		int MatNum = aiData->mNumMaterials;
		vrmAssetList->Materials.SetNum(MatNum);
		for (int32_t iMat = 0; iMat < MatNum; ++iMat) {
			auto& aiMat = *aiData->mMaterials[iMat];

			//
			aiString ais;
			if (aiMat.Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), ais) != AI_SUCCESS) {
				continue;
			}

			FString TextureFullPath = FPaths::GetPath(vrmAssetList->FileFullPathName) + TEXT("/") + UTF8_TO_TCHAR(ais.C_Str());
			
			FString baseName = FPaths::GetBaseFilename(UTF8_TO_TCHAR(ais.C_Str()));
			baseName = TEXT("T_") + FPaths::GetBaseFilename(baseName);
			baseName = NormalizeFileName(baseName);

			if (pmxTexNameList.Find(baseName) >= 0) {
				continue;
			}
			pmxTexNameList.Push(baseName);


			TArray<uint8> RawFileData;
			if (FFileHelper::LoadFileToArray(RawFileData, *TextureFullPath)){

				auto* pkg = vrmAssetList->Package;
				if (VRMConverter::Options::Get().IsSingleUAssetFile() == false) {
					pkg = VRM4U_CreatePackage(vrmAssetList->Package, *baseName);
				}
				UTexture2D* NewTexture2D = VRMLoaderUtil::CreateTextureFromImage(baseName, pkg, RawFileData.GetData(), RawFileData.Num(), bGenerateMips);

				texArray.Push(NewTexture2D);
			}
		}
		vrmAssetList->Textures = texArray;
	}

	if (aiData->HasMaterials()) {
		//TArray<FString> MatNameList;
		TMap<FString, int> MatNameList;

		int MatNum = FMath::Max(1, (int)aiData->mNumMaterials);

		// lastmaterial = gltf default material
		if (Options::Get().IsVRMModel()) {
			MatNum = FMath::Max(1, (int)aiData->mNumMaterials - 1);
		}

		vrmAssetList->Materials.SetNum(MatNum);
		for (int32_t iMat = 0; iMat < MatNum; ++iMat) {
			auto &aiMat = *aiData->mMaterials[iMat];

			UMaterialInterface *baseM = nullptr;
			UVrmImportMaterialSet *mset = nullptr;

			bool bMToon = false;
			{
				FString ShaderName = aiMat.mShaderName.C_Str();

				auto MaterialType = Options::Get().GetMaterialType();

				vrmAssetList->ImportMode = MaterialType;

				// native mtoon model ?
				LocalSetOriginalVrmMaterial(false);
				if (ShaderName.Find(TEXT("MToon")) >= 0) {
					// native
					LocalSetOriginalVrmMaterial(true);
				}
				if (VRMConverter::Options::Get().IsVRM10Model()) {
					// native
					LocalSetOriginalVrmMaterial(true);
				}

				if (Options::Get().GetMaterialType() == EVRMImportMaterialType::VRMIMT_Auto) {
					// default unlit
					MaterialType = EVRMImportMaterialType::VRMIMT_Unlit;

					if (ShaderName.Find(TEXT("MToon")) >= 0) {
						MaterialType = EVRMImportMaterialType::VRMIMT_MToonUnlit;
					}
					if (Options::Get().IsPMXModel()) {
						MaterialType = EVRMImportMaterialType::VRMIMT_MToonUnlit;
					}
					if (Options::Get().IsVRM10Model()) {
						MaterialType = EVRMImportMaterialType::VRMIMT_MToonUnlit;
					}
				}

				// select materialset
				switch (MaterialType) {
				case EVRMImportMaterialType::VRMIMT_MToon:
					mset = vrmAssetList->MtoonLitSet;
					bMToon = true;
					break;
				case EVRMImportMaterialType::VRMIMT_MToonUnlit:
					mset = vrmAssetList->MtoonUnlitSet;
					bMToon = true;
					break;
				case EVRMImportMaterialType::VRMIMT_SSS:
					mset = vrmAssetList->SSSSet;
					bMToon = true;	// MTOON TRUE!
					break;
				case EVRMImportMaterialType::VRMIMT_SSSProfile:
					mset = vrmAssetList->SSSProfileSet;
					bMToon = true;	// MTOON TRUE!
					break;
				case EVRMImportMaterialType::VRMIMT_Unlit:
					mset = vrmAssetList->UnlitSet;
					bMToon = false;
					break;
				case EVRMImportMaterialType::VRMIMT_glTF:
					mset = vrmAssetList->GLTFSet;
					bMToon = false;
					break;
				case EVRMImportMaterialType::VRMIMT_UEFNUnlit:
					mset = vrmAssetList->UEFNUnlitSet;
					bMToon = false;
					break;
				case EVRMImportMaterialType::VRMIMT_UEFNLit:
					mset = vrmAssetList->UEFNLitSet;
					bMToon = false;
					break;
				case EVRMImportMaterialType::VRMIMT_UEFNSSSProfile:
					mset = vrmAssetList->UEFNSSSProfileSet;
					bMToon = false;
					break;
				case EVRMImportMaterialType::VRMIMT_Custom:
					mset = vrmAssetList->CustomSet;
					bMToon = false;
					break;
				case EVRMImportMaterialType::VRMIMT_Auto:
				default:
					break;
				}
				if (mset == nullptr) {
					continue;
				}


				bool bTwoSided = false;
				bool bTranslucent = false;
				bool bOpaque = false;

				{
					aiString alphaMode;
					aiReturn result = aiMat.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode);
					FString alpha = alphaMode.C_Str();
					if (alpha == TEXT("BLEND")) {
						// check also _ZWrite
						bTranslucent = true;
					}
					if (alpha == TEXT("MASK")) {
					}
					if (alpha == TEXT("OPAQUE")) {
						bOpaque = true;
						if (vrmAssetList->MaterialHasAlphaCutoff.IsValidIndex(iMat)) {
							if (vrmAssetList->MaterialHasAlphaCutoff[iMat]) {
								// force mask mode
								bOpaque = false;
							}
						}
					}
					bool b = false;
					aiReturn ret = aiMat.Get(AI_MATKEY_TWOSIDED, b);
					if (ret == AI_SUCCESS) {
						if (b) bTwoSided = true;
					}
				}

				if (ShaderName.Find(TEXT("MToon")) >= 0) {
					const VRM::VRMMetadata *meta = static_cast<const VRM::VRMMetadata*>(aiData->mVRMMeta);
					if (meta) {
						if ((int)iMat < meta->materialNum) {
							auto &vrmMat = meta->material[iMat];
							if (vrmMat.floatProperties._CullMode == 0.f || vrmMat.floatProperties._CullMode== 1.f) {
								bTwoSided = true;
							}
							if (vrmMat.floatProperties._ZWrite == 1.f) {
								bTranslucent = false;
							}
						}
					}
				}else{
					if (ShaderName.Find(TEXT("UnlitTexture")) >= 0) {
					}
					if (ShaderName.Find(TEXT("UnlitTransparent")) >= 0) {
						bTranslucent = true;
					}
				}
				if (Options::Get().IsForceOpaque()) {
					bTranslucent = false;
				}
				if (Options::Get().IsForceTwoSided()) {
					bTwoSided = true;
				}


				// material set

				if (bMToon) {
					// opaque/translucent, twoside
					UMaterialInterface *table_param[2][2] = {
						{
							mset->Opaque,
							mset->OpaqueTwoSided,
						},
						{
							mset->Translucent,
							mset->TranslucentTwoSided,
						},
					};

					int c[2] = {
						bTranslucent ? 1 : 0,
						bTwoSided ? 1 : 0,
					};
					baseM = table_param[c[0]][c[1]];

					if (matArray.Num() == matFlagTranslucentArray.Num()) {
						matFlagTranslucentArray.Add(c[0] != 0);
						matFlagTwoSidedArray.Add(c[1] != 0);
						matFlagOpaqueArray.Add(bOpaque);
					}

				}else{
					// not mtoon
					baseM = mset->Opaque;
					if (bTranslucent){
						baseM = mset->Translucent;
					}
				}
			}

			if (baseM == nullptr) {
				continue;
			}

			TArray<int> TextureTypeToIndex;
			{
				TextureTypeToIndex.SetNum(AI_TEXTURE_TYPE_MAX);
				for (auto& a : TextureTypeToIndex) {
					a = -1;
				}

				TArray<aiString> texName;
				texName.SetNum(AI_TEXTURE_TYPE_MAX);

				for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t) {
					uint32_t n = aiMat.GetTextureCount((aiTextureType)t);
					for (uint32_t y = 0; y < FMath::Min((uint32_t)1, n); ++y) {
						aiMat.GetTexture((aiTextureType)t, y, &texName[t]);
					}
				}

				for (uint32_t i = 0; i < aiData->mNumTextures; ++i) {
					for (int32_t t = 0; t < texName.Num(); ++t) {
						if (aiData->mTextures[i]->mFilename == texName[t]) {
							TextureTypeToIndex[t] = i;
							break;
						}
					}
				}
			}

			for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t) {
				aiString path;
				aiReturn r = aiMat.GetTexture(aiTextureType(t), 0, &path);
				if (r == AI_SUCCESS) {
					std::string s = path.C_Str();
					s = s.substr(s.find_last_of('*') + 1);
					TextureTypeToIndex[t] = atoi(s.c_str());

					if (Options::Get().IsPMXModel()) {
						for (int i = 0; i < pmxTexNameList.Num(); ++i) {

							FString baseName = FPaths::GetBaseFilename(UTF8_TO_TCHAR(path.C_Str()));
							baseName = TEXT("T_") + FPaths::GetBaseFilename(baseName);
							baseName = NormalizeFileName(baseName);

							if (pmxTexNameList[i] == baseName) {
								TextureTypeToIndex[t] = i;
								break;
							}
						}
					}
				}
			}

			//UMaterialInstanceDynamic* dm = UMaterialInstanceDynamic::Create(baseM, vrmAssetList, m.GetName().C_Str());
			//UMaterialInstanceDynamic* dm = UMaterialInstance::Create(baseM, vrmAssetList, m.GetName().C_Str());
			//MaterialInstance->TextureParameterValues

			//set paramater with Set***ParamaterValue
			//DynMaterial->SetScalarParameterValue("MyParameter", myFloatValue);
			//MyComponent1->SetMaterial(0, DynMaterial);
			//MyComponent2->SetMaterial(0, DynMaterial);

			// ALL!! no texture material.
			//if (indexDiffuse >= 0 && indexDiffuse < vrmAssetList->Textures.Num()) {
			//if (indexDiffuse >= 0) {
			{
				UMaterialInstanceConstant* dm = nullptr;
				{
					const FString origname = (FString(TEXT("MI_")) + NormalizeFileName(aiMat.GetName().C_Str()));
					FString name = origname;

					if (MatNameList.Find(origname)) {
						name += FString::Printf(TEXT("_%03d"), MatNameList[name]);
					}
					MatNameList.FindOrAdd(origname)++;

					if (vrmAssetList->Package == GetTransientPackage()) {
						dm = VRM4U_NewObject<UMaterialInstanceConstant>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
					} else {
						TArray<UObject*> ret;
						GetObjectsWithOuter(vrmAssetList->Package, ret);
						for (auto *a : ret) {
							auto s = a->GetName().ToLower();
							if(s.IsEmpty()) continue;

							if (s == name.ToLower()) {
								//a->ClearFlags(EObjectFlags::RF_Standalone);
								//a->SetFlags(EObjectFlags::RF_Public | RF_Transient);
								//a->ConditionalBeginDestroy();
								static int ccc = 0;
								++ccc;
								a->Rename(*(FString(TEXT("need_reload_tex_VRM"))+FString::FromInt(ccc)), GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);

								break;
							}
						}
						dm = VRM4U_NewObject<UMaterialInstanceConstant>(vrmAssetList->Package, *name, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
						vrmAssetList->MaterialNameOrigToAsset.Add(NormalizeFileName(aiMat.GetName().C_Str()), name);
					}
				}
				LocalMaterialSetParent(dm, baseM);

				if (dm) {
					{
						FVectorParameterValue *v = new (dm->VectorParameterValues) FVectorParameterValue();
						v->ParameterInfo.Index = INDEX_NONE;
						v->ParameterInfo.Name = TEXT("gltf_basecolor");
						v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;

						aiColor4D col(1.f, 1.f, 1.f, 1.f);
						aiReturn result = aiMat.Get(AI_MATKEY_BASE_COLOR, col);
						if (result == 0) {
						}
						v->ParameterValue = FLinearColor(col.r, col.g, col.b, col.a);
					}

					{
						float f[2] = { 1,1 };
						aiReturn result0 = aiMat.Get(AI_MATKEY_ROUGHNESS_FACTOR, f[0]);
						aiReturn result1 = aiMat.Get(AI_MATKEY_METALLIC_FACTOR, f[1]);
						if (result0 == AI_SUCCESS || result1 == AI_SUCCESS) {
							f[0] = (result0 == AI_SUCCESS) ? f[0] : 1;
							f[1] = (result1 == AI_SUCCESS) ? f[1] : 1;
							if (f[0] == 0 && f[1] == 0) {
								f[0] = f[1] = 1.f;
							}
							FVectorParameterValue *v = new (dm->VectorParameterValues) FVectorParameterValue();
							v->ParameterInfo.Index = INDEX_NONE;
							v->ParameterInfo.Name = TEXT("gltf_RM");
							v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;;
							v->ParameterValue = FLinearColor(f[0], f[1], 0, 0);
						}
					}
					int indexDiffuse = TextureTypeToIndex[aiTextureType::aiTextureType_DIFFUSE];
					if (indexDiffuse >= 0 && indexDiffuse < vrmAssetList->Textures.Num()) {
						LocalTextureSet(dm, TEXT("gltf_tex_diffuse"), vrmAssetList->Textures[indexDiffuse]);
						{
							FString str = TEXT("mtoon_tex_ShadeTexture");
							bool bFindShadeTex = false;
							for (auto &t : dm->TextureParameterValues) {
								if (str.Compare(t.ParameterInfo.Name.ToString(), ESearchCase::IgnoreCase) == 0) {
									if (t.ParameterValue) {
										bFindShadeTex = true;

										if (IsValid(vrmAssetList->Textures[indexDiffuse]) == false) {
#if UE_VERSION_OLDER_THAN(5,0,0)
											UTexture2D *tmp = Cast<UTexture2D>(t.ParameterValue);
#else
											UTexture2D* tmp = Cast<UTexture2D>(t.ParameterValue.Get());
#endif
											if (tmp) {
												LocalTextureSet(dm, TEXT("gltf_tex_diffuse"), tmp);
											}
										}
									}
								}
							}
							if (bFindShadeTex == false) {
								LocalTextureSet(dm, *str, vrmAssetList->Textures[indexDiffuse]);
							}
						}
					} else {
						if (Options::Get().IsDefaultGridTextureMode() == false) {
							// set white texture for default
							UTexture2D* tex = LoadObject<UTexture2D>(nullptr, TEXT("/VRM4U/MaterialUtil/T_DummyWhite.T_DummyWhite"));
							if (tex) {
								LocalTextureSet(dm, TEXT("gltf_tex_diffuse"), tex);
								LocalTextureSet(dm, TEXT("mtoon_tex_ShadeTexture"), tex);
								LocalTextureSet(dm, TEXT("mtoon_tex_Shade"), tex);
							}
						}
					}
					if (bMToon == false){
						{
							aiString alphaMode;
							aiReturn result = aiMat.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode);
							FString alpha = alphaMode.C_Str();
							if (alpha == TEXT("BLEND")) {
								if (Options::Get().IsForceOpaque() == false) {
									dm->BasePropertyOverrides.bOverride_BlendMode = true;;
									dm->BasePropertyOverrides.BlendMode = EBlendMode::BLEND_Translucent;
								}
							}
						}
					}


					// mtoon
					if (bMToon || VRMConverter::Options::Get().IsVRM10Model()) {
						createAndAddMaterial(dm, iMat, vrmAssetList, this, TextureTypeToIndex);

						if (matFlagOpaqueArray.IsValidIndex(iMat)) {
							if (matFlagOpaqueArray[iMat]) {
								LocalScalarParameterSet(dm, TEXT("bOpaque"), 1.f);
							}
						}
						if (vrmAssetList->MaterialHasMToon.IsValidIndex(iMat)) {
							if (vrmAssetList->MaterialHasMToon[iMat] == false) {
								for (auto& a : dm->VectorParameterValues) {
									if (a.ParameterInfo.Name == TEXT("mtoon_Color")) {
										auto c = a.ParameterValue;
										LocalVectorParameterSet(dm, TEXT("mtoon_ShadeColor"), c);
										break;
									}
								}
							}
						}
					} else {
						// gltf texture
						TArray<FString> materialParamName;
						materialParamName.SetNum(AI_TEXTURE_TYPE_MAX);
						materialParamName[aiTextureType::aiTextureType_DIFFUSE] = TEXT("gltf_tex_diffuse");
						materialParamName[aiTextureType::aiTextureType_NORMALS] = TEXT("gltf_tex_normal");
						materialParamName[aiTextureType::aiTextureType_EMISSIVE] = TEXT("gltf_tex_Emission");

						materialParamName[aiTextureType::aiTextureType_BASE_COLOR] = TEXT("gltf_tex_diffuse");
						materialParamName[aiTextureType::aiTextureType_EMISSION_COLOR] = TEXT("gltf_tex_Emission");
						materialParamName[aiTextureType::aiTextureType_METALNESS] = TEXT("gltf_tex_metalness");
						materialParamName[aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS] = TEXT("gltf_tex_roughness");

						for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t) {
							if (materialParamName[t] == "") continue;

							int index = TextureTypeToIndex[t];
							if (index < 0) continue;
							if (vrmAssetList->Textures.IsValidIndex(index) == false) continue;
							if (IsValid(vrmAssetList->Textures[index]) == false) continue;

							LocalTextureSet(dm, *(materialParamName[t]), vrmAssetList->Textures[index]);
						}
					}

					LocalMaterialFinishParam(dm);

					//dm->InitStaticPermutation();
					matArray.Add(dm);

					if (matArray.Num() != matFlagTranslucentArray.Num()) {
						matFlagTranslucentArray.SetNumZeroed(matArray.Num());
						matFlagTwoSidedArray.SetNumZeroed(matArray.Num());
						matFlagOpaqueArray.SetNumZeroed(matArray.Num());
					}

					TArray<FString> t;
					if (vrmAssetList->MaterialNameOrigToAsset.Num()) {
						vrmAssetList->MaterialNameOrigToAsset.GenerateValueArray(t);
						vrmAssetList->MaterialNameAssetToMatNo.Add(t[t.Num() - 1], matArray.Num() - 1);
					}
				}
			}
		}


		if (VRMConverter::Options::Get().IsBC7Mode()) {
			vrmAssetList->Texture_CompressType = EVRMImportTextureCompressType::VRMITC_BC7;
		}
		vrmAssetList->Texture_CompressTypeList = textureCompressTypeArray;
		if (Options::Get().IsMergeMaterial() == false) {
			vrmAssetList->Materials = matArray;
			vrmAssetList->MaterialFlag_Translucent = matFlagTranslucentArray;
			vrmAssetList->MaterialFlag_TwoSided = matFlagTwoSidedArray;
			vrmAssetList->MaterialFlag_Opaque = matFlagOpaqueArray;
		} else {
			TArray<UMaterialInterface*> tmp;
			TArray<bool> tmpTranslucent;
			TArray<bool> tmpTwoSided;
			TArray<bool> tmpOpaque;

			vrmAssetList->MaterialMergeTable.Reset();

			for (int i = 0; i < matArray.Num(); ++i) {

				vrmAssetList->MaterialMergeTable.Add(i, 0);

				bool bFind = false;
				for (int j = 0; j < tmp.Num(); ++j) {
					if (isSameMaterial(matArray[i], tmp[j]) == false) {
						continue;
					}
					bFind = true;
					vrmAssetList->MaterialMergeTable[i] = j;

					matArray[i]->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);

					break;
				}
				if (bFind == false) {
					int t = tmp.Add(matArray[i]);
					vrmAssetList->MaterialMergeTable[i] = t;

					tmpTranslucent.Add(matFlagTranslucentArray[i]);
					tmpTwoSided.Add(matFlagTwoSidedArray[i]);
					tmpOpaque.Add(matFlagOpaqueArray[i]);
				}
			}
			vrmAssetList->Materials = tmp;
			vrmAssetList->MaterialFlag_Translucent = tmpTranslucent;
			vrmAssetList->MaterialFlag_TwoSided = tmpTwoSided;
			vrmAssetList->MaterialFlag_Opaque = tmpOpaque;

			for (auto& a : vrmAssetList->MaterialNameAssetToMatNo) {
				a.Value = vrmAssetList->MaterialMergeTable[a.Value];
			}
		}

		// ouline Material
		if (VRMConverter::Options::Get().IsGenerateOutlineMaterial()) {
			if (vrmAssetList->OptMToonOutlineMaterial){
				for (const auto aa : vrmAssetList->Materials) {
					const UMaterialInstanceConstant *a = Cast<UMaterialInstanceConstant>(aa);

					FString s = a->GetName() + TEXT("_outline");
					//UMaterialInstanceConstant *m = Cast<UMaterialInstanceConstant>(StaticDuplicateObject(a->GetOuter(), a, *s, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, UMaterialInstanceConstant::StaticClass()));
					UMaterialInstanceConstant *m = VRM4U_NewObject<UMaterialInstanceConstant>(vrmAssetList->Package, *s, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

					if (m) {
						LocalMaterialSetParent(m, vrmAssetList->OptMToonOutlineMaterial);

						m->VectorParameterValues = a->VectorParameterValues;
						m->ScalarParameterValues = a->ScalarParameterValues;
						m->TextureParameterValues = a->TextureParameterValues;

						LocalMaterialFinishParam(m);
						//m->InitStaticPermutation();
						vrmAssetList->OutlineMaterials.Add(m);
					}
				}
			}
		}

		{
			UPackage* package = GetTransientPackage();
			if (vrmAssetList) {
				package = vrmAssetList->Package;
			}
			auto sp = VRM4U_NewObject<USubsurfaceProfile>(package, *(FString(TEXT("SP_")) + vrmAssetList->BaseFileName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
#if UE_VERSION_OLDER_THAN(5,0,0)
			sp->Settings.ScatterRadius = 50.f;
#else
			// default
#endif

#if WITH_EDITORONLY_DATA
			vrmAssetList->SSSProfile = sp;
#endif

			for (auto& a : vrmAssetList->Materials) {
				a->SubsurfaceProfile = sp;
				auto* c = Cast<UMaterialInstance>(a);
				if (c) {
					c->bOverrideSubsurfaceProfile = true;
				}
			}
		}
	}

	return true;
}

VrmConvertTexture::VrmConvertTexture()
{
}

VrmConvertTexture::~VrmConvertTexture()
{
}
