// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmBPFunctionLibrary.h"
#include "Materials/MaterialInterface.h"
#include "TextureResource.h"

#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SkeletalMesh.h"
#include "Logging/MessageLog.h"
#include "Engine/Canvas.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/MorphTarget.h"
#include "Misc/EngineVersionComparison.h"
#if	UE_VERSION_OLDER_THAN(4,26,0)
#include "AssetRegistryModule.h"
#include "ARFilter.h"
#else
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#endif

#include "Components/SkeletalMeshComponent.h"
#include "Components/LightComponent.h"

#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshModel.h"

#include "VrmRigHeader.h"

#if	UE_VERSION_OLDER_THAN(5,5,0)
#else
#include "Kismet/KismetRenderingLibrary.h"
#endif

#if	UE_VERSION_OLDER_THAN(5,1,0)
#else
#if WITH_EDITOR && PLATFORM_WINDOWS
#define VRM4U_USE_MRQ 1
#endif
#endif

#ifndef VRM4U_USE_MRQ
#define VRM4U_USE_MRQ 0
#endif


#if VRM4U_USE_MRQ
#include "MoviePipelineQueueSubsystem.h"
#endif


#include "Animation/AnimInstance.h"
#include "VrmAnimInstanceCopy.h"
#include "VrmUtil.h"

#if PLATFORM_WINDOWS
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#endif

#include "HAL/ConsoleManager.h"


#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "EditorSupportDelegates.h"
#include "LevelEditorActions.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "AssetNotifications.h"
#endif

#include "Animation/AnimSequence.h"
#include "Kismet/GameplayStatics.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

//#include "VRM4U.h"

void UVrmBPFunctionLibrary::VRMTransMatrix(const FTransform &transform, TArray<FLinearColor> &matrix, TArray<FLinearColor> &matrix_inv){

	FMatrix m = transform.ToMatrixWithScale();
	FMatrix mi = transform.ToMatrixWithScale().Inverse();

	matrix.SetNum(4);
	matrix_inv.SetNum(4);

	for (int i = 0; i < 4; ++i) {
		matrix[i] = FLinearColor(m.M[i][0], m.M[i][1], m.M[i][2], m.M[i][3]);
		matrix_inv[i] = FLinearColor(mi.M[i][0], mi.M[i][1], mi.M[i][2], mi.M[i][3]);
	}

	return;
}

void UVrmBPFunctionLibrary::VRMGetMorphTargetList(const USkeletalMesh *target, TArray<FString> &morphTargetList) {
	morphTargetList.Empty();

	if (target == nullptr) {
		return;
	}
	for (const auto &a : VRMGetMorphTargets(target)) {
		morphTargetList.Add(a->GetName());
	}
}

bool UVrmBPFunctionLibrary::VRMGetRefBoneTransform(const USkeleton *target, const FName boneName, FTransform &boneTransform){
	if (target == nullptr) {
		return false;
	}
	const FReferenceSkeleton& r = target->GetReferenceSkeleton();
	int i = r.FindBoneIndex(boneName);
	if (i < 0) {
		return false;
	}

	boneTransform = r.GetRefBonePose()[i];

	return true;
}

void UVrmBPFunctionLibrary::VRMGetHumanoidBoneNameList(TArray<FString> &boneNameListString, TArray<FName> &boneNameListName) {
	boneNameListString = VRMUtil::vrm_humanoid_bone_list;
	boneNameListName = VRMUtil::vrm_humanoid_bone_list_name;
}

void UVrmBPFunctionLibrary::VRMGetEpicSkeletonBoneNameList(TArray<FString>& boneNameListString, TArray<FName>& boneNameListName) {
	boneNameListString = VRMUtil::ue4_humanoid_bone_list;
	boneNameListName = VRMUtil::ue4_humanoid_bone_list_name;
}

void UVrmBPFunctionLibrary::VRMGetEpicSkeletonToHumanoid(TMap<FString, FString>& table) {
	table.Empty();
	for (auto &a : VRMUtil::table_ue4_vrm) {
		table.Add(a.BoneUE4, a.BoneVRM);
	}
}


bool UVrmBPFunctionLibrary::VRMGetHumanoidParentBone(const FName boneName, FName &parentBoneName) {

	int i = VRMUtil::vrm_humanoid_bone_list_name.Find(boneName);
	if (i < 0) {
		return false;
	}
	parentBoneName = *VRMUtil::vrm_humanoid_parent_list[i];

	if (parentBoneName.IsNone()) {
		return false;
	}

	return true;
}


void UVrmBPFunctionLibrary::VRMInitAnim(USkeletalMeshComponent *target) {
	if (target == nullptr) return;
	auto *anim_orig = target->GetAnimInstance();
	if (anim_orig == nullptr) return;

	auto *anim = Cast<UVrmAnimInstanceCopy>(anim_orig);
	if (anim == nullptr) {
		target->InitAnim(true);
		return;
	}

	auto *p = anim->GetProxy();
	if (p == nullptr) return;

	{
		bool b = p->bIgnoreVRMSwingBone;
		p->bIgnoreVRMSwingBone = true;

		target->InitAnim(true);

		p->bIgnoreVRMSwingBone = b;
	}

	//CalcBoneVertInfos
	//target->GetBoneTransform
}

void UVrmBPFunctionLibrary::VRMUpdateRefPose(USkeletalMeshComponent* target, bool bForceAlignGlobal, bool bForceUE4Humanoid) {
#if WITH_EDITOR
	if (target == nullptr) return;
	if (VRMGetSkinnedAsset(target) == nullptr) return;

	{
		auto *sk = VRMGetSkinnedAsset(target);
		auto *k = VRMGetSkeleton( VRMGetSkinnedAsset(target) );

#if	UE_VERSION_OLDER_THAN(4,23,0)
		const auto &transTable = target->BoneSpaceTransforms;
#else
		const auto &transTable = target->GetBoneSpaceTransforms();
#endif

		TArray<FTransform> origGlobalTransform;
		TArray<FTransform> origGlobalTransformInv;
		TArray<FTransform> destGlobalTransform;

		TArray<FTransform> destGlobalAlignedTransform;
		TArray<FTransform> destLocalAlignedTransform;

		{
			const auto& refPose = k->GetReferenceSkeleton().GetRefBonePose();
			
			// orig all
			for (int boneNo = 0; boneNo < refPose.Num(); ++boneNo) {
				auto t = refPose[boneNo];
				int parent = k->GetReferenceSkeleton().GetParentIndex(boneNo);
				if (parent >= 0) {
					t *= origGlobalTransform[parent];
				}
				origGlobalTransform.Add(t);
			}

			//orig inv
			for (int i = 0; i < origGlobalTransform.Num(); ++i) {
				origGlobalTransformInv.Add(origGlobalTransform[i].Inverse());
			}
		}

		{
			// dest all
			for (int boneNo = 0; boneNo < transTable.Num(); ++boneNo) {
				auto t = transTable[boneNo];
				int parent = k->GetReferenceSkeleton().GetParentIndex(boneNo);
				if (parent >= 0) {
					t *= destGlobalTransform[parent];
				}
				destGlobalTransform.Add(t);

				{
					auto t2 = t;
					t2.SetRotation(FQuat::Identity);
					destGlobalAlignedTransform.Add(t2);
				}

				{
					FVector v = t.GetLocation();
					if (parent >= 0) {
						v -= destGlobalTransform[parent].GetLocation();
					}
					destLocalAlignedTransform.Add(FTransform(v));
				}
			}
		}

		{
			FReferenceSkeletonModifier RefSkelModifier(VRMGetRefSkeleton(sk), k);
			for (int i = 0; i < transTable.Num(); ++i) {
				if (bForceAlignGlobal) {
					auto t = transTable[i];
					t.SetRotation(FQuat::Identity);
					RefSkelModifier.UpdateRefPoseTransform(i, destLocalAlignedTransform[i]);
				} else {
					RefSkelModifier.UpdateRefPoseTransform(i, transTable[i]);
				}
			}
		}

		k->UpdateReferencePoseFromMesh(sk);
		FAssetNotifications::SkeletonNeedsToBeSaved(k);

		VRMSetRefSkeleton(sk, k->GetReferenceSkeleton());
		VRMGetRefSkeleton(sk).RebuildRefSkeleton(k, true);
		sk->CalculateInvRefMatrices();
#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
		sk->UpdateGenerateUpToData();
#endif

		k->RecreateBoneTree(sk);
		{
			TArray<FName> BonesToRemove;
			k->RemoveVirtualBones(BonesToRemove);
		}
		// skeleton end

		//vertex begin

		{
			FSkeletalMeshModel* ImportedResource = sk->GetImportedModel();
			if (ImportedResource->LODModels.Num() == 0)
				return;

			FSkeletalMeshLODModel* LODModel = &ImportedResource->LODModels[0];
			for (int32 SectionIndex = 0; SectionIndex < LODModel->Sections.Num(); SectionIndex++)
			{
				FSkelMeshSection& Section = LODModel->Sections[SectionIndex];

				FSkelMeshSection SectionOrg = Section;
				for (int32 i = 0; i < Section.SoftVertices.Num(); i++)
				{
					FSoftSkinVertex* SoftVert = &Section.SoftVertices[i];
					const FSoftSkinVertex* SoftVertOrg = &SectionOrg.SoftVertices[i];

					{
						for (int32 j = 0; j < MAX_TOTAL_INFLUENCES; j++)
						{
							if (SoftVert->InfluenceWeights[j] > 0)
							{
								int32 BoneIndex = Section.BoneMap[SoftVert->InfluenceBones[j]];
							}
						}
					}

					for (int32 j = 0; j < MAX_TOTAL_INFLUENCES; j++)
					{
						if (SoftVert->InfluenceWeights[j] > 0)
						{
							if (j == 0) {
#if	UE_VERSION_OLDER_THAN(5,0,0)
								SoftVert->Position = FVector::ZeroVector;
								SoftVert->TangentZ = FVector::ZeroVector;
#else
								SoftVert->Position = FVector4f::Zero();
								SoftVert->TangentZ = FVector4f::Zero();
#endif
							}
							int32 BoneIndex = Section.BoneMap[SoftVert->InfluenceBones[j]];

							FVector p = origGlobalTransformInv[BoneIndex].TransformPosition(FVector(SoftVertOrg->Position));
							p = destGlobalTransform[BoneIndex].TransformPosition(p);



#if	UE_VERSION_OLDER_THAN(5,0,0)
							FVector n = origGlobalTransformInv[BoneIndex].TransformVector(SoftVertOrg->TangentZ);
#else
							FVector n = origGlobalTransformInv[BoneIndex].TransformVector(FVector3d(SoftVertOrg->TangentZ));
#endif
							n = destGlobalTransform[BoneIndex].TransformVector(n);

#if	UE_VERSION_OLDER_THAN(4,20,0)
							SoftVert->Position += p * (float)SoftVert->InfluenceWeights[j] / 255.f;
							SoftVert->TangentZ.Vector.X += n.X * (float)SoftVert->InfluenceWeights[j] / 255.f;
							SoftVert->TangentZ.Vector.Y += n.Y * (float)SoftVert->InfluenceWeights[j] / 255.f;
							SoftVert->TangentZ.Vector.Z += n.Z * (float)SoftVert->InfluenceWeights[j] / 255.f;
#elif	UE_VERSION_OLDER_THAN(5, 0, 0)
							SoftVert->Position += p * (float)SoftVert->InfluenceWeights[j] / 255.f;
							SoftVert->TangentZ += n * (float)SoftVert->InfluenceWeights[j] / 255.f;
#else
							SoftVert->Position += FVector3f(p * (float)SoftVert->InfluenceWeights[j] / 255.f);
							SoftVert->TangentZ += FVector4f(FVector3f(n * (float)SoftVert->InfluenceWeights[j] / 255.f), 1.f);
#endif
						}
					}
				}
			}
		}

		k->MarkPackageDirty();
		sk->MarkPackageDirty();

		k->PostEditChange();
		sk->PostEditChange();
	}
#endif
}


void UVrmBPFunctionLibrary::VRMGetMaterialPropertyOverrides(const UMaterialInterface *Material, TEnumAsByte<EBlendMode> &BlendMode, TEnumAsByte<EMaterialShadingModel> &ShadingModel, bool &IsTwoSided, bool &IsMasked){
	if (Material == nullptr) {
		return;
	}
	BlendMode		= Material->GetBlendMode();
#if	UE_VERSION_OLDER_THAN(4,23,0)
	ShadingModel = Material->GetShadingModel();
#else
	ShadingModel = Material->GetShadingModels().GetFirstShadingModel();
#endif
	IsTwoSided		= Material->IsTwoSided();
	IsMasked		= Material->IsMasked();
}


void UVrmBPFunctionLibrary::VRMGetMobileMode(bool &IsMobile, bool &IsAndroid, bool &IsIOS) {
	IsMobile = false;
	IsAndroid = false;
	IsIOS = false;

#if PLATFORM_ANDROID
	IsMobile = true;
	IsAndroid = true;
#endif

#if PLATFORM_IOS
	IsMobile = true;
	IsIOS = true;
#endif

}



void UVrmBPFunctionLibrary::VRMDrawMaterialToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UMaterialInterface* Material)
{
#if	UE_VERSION_OLDER_THAN(4,20,0)
#elif UE_VERSION_OLDER_THAN(4,25,0)
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_InvalidWorldContextObject", "DrawMaterialToRenderTarget: WorldContextObject is not valid."));
	} else if (!Material)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_InvalidMaterial", "DrawMaterialToRenderTarget: Material must be non-null."));
	} else if (!TextureRenderTarget)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_InvalidTextureRenderTarget", "DrawMaterialToRenderTarget: TextureRenderTarget must be non-null."));
	} else if (!TextureRenderTarget->Resource)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_ReleasedTextureRenderTarget", "DrawMaterialToRenderTarget: render target has been released."));
	} else
	{
		UCanvas* Canvas = World->GetCanvasForDrawMaterialToRenderTarget();

		FCanvas RenderCanvas(
			TextureRenderTarget->GameThread_GetRenderTargetResource(),
			nullptr,
			World,
			World->FeatureLevel);

		Canvas->Init(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY, nullptr, &RenderCanvas);
		Canvas->Update();



		TDrawEvent<FRHICommandList>* DrawMaterialToTargetEvent = new TDrawEvent<FRHICommandList>();

		FName RTName = TextureRenderTarget->GetFName();
		ENQUEUE_RENDER_COMMAND(BeginDrawEventCommand)(
			[RTName, DrawMaterialToTargetEvent](FRHICommandListImmediate& RHICmdList)
		{
			// Update resources that were marked for deferred update. This is important
			// in cases where the blueprint is invoked in the same frame that the render
			// target is created. Among other things, this will perform deferred render
			// target clears.
			FDeferredUpdateResource::UpdateResources(RHICmdList);

			BEGIN_DRAW_EVENTF(
				RHICmdList,
				DrawCanvasToTarget,
				(*DrawMaterialToTargetEvent),
				*RTName.ToString());
		});

		Canvas->K2_DrawMaterial(Material, FVector2D(0, 0), FVector2D(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY), FVector2D(0, 0));

		RenderCanvas.Flush_GameThread();
		Canvas->Canvas = NULL;

		FTextureRenderTargetResource* RenderTargetResource = TextureRenderTarget->GameThread_GetRenderTargetResource();
		ENQUEUE_RENDER_COMMAND(CanvasRenderTargetResolveCommand)(
			[RenderTargetResource, DrawMaterialToTargetEvent](FRHICommandList& RHICmdList)
		{
			RHICmdList.CopyToResolveTarget(RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, FResolveParams());
			STOP_DRAW_EVENT((*DrawMaterialToTargetEvent));
			delete DrawMaterialToTargetEvent;
		}
		);
	}
#elif UE_VERSION_OLDER_THAN(5,5,0)
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		//FMessageLog("Blueprint").Warning(LOCTEXT("DrawMaterialToRenderTarget_InvalidWorldContextObject", "DrawMaterialToRenderTarget: WorldContextObject is not valid."));
	} else if (!Material)
	{
		//FMessageLog("Blueprint").Warning(FText::Format(LOCTEXT("DrawMaterialToRenderTarget_InvalidMaterial", "DrawMaterialToRenderTarget[{0}]: Material must be non-null."), FText::FromString(GetPathNameSafe(WorldContextObject))));
	} else if (!TextureRenderTarget)
	{
		//FMessageLog("Blueprint").Warning(FText::Format(LOCTEXT("DrawMaterialToRenderTarget_InvalidTextureRenderTarget", "DrawMaterialToRenderTarget[{0}]: TextureRenderTarget must be non-null."), FText::FromString(GetPathNameSafe(WorldContextObject))));
#if	UE_VERSION_OLDER_THAN(5,0,0)
	} else if (!TextureRenderTarget->Resource)
	{
#else
	} else if (!TextureRenderTarget->GetResource())
	{
#endif
		//FMessageLog("Blueprint").Warning(FText::Format(LOCTEXT("DrawMaterialToRenderTarget_ReleasedTextureRenderTarget", "DrawMaterialToRenderTarget[{0}]: render target has been released."), FText::FromString(GetPathNameSafe(WorldContextObject))));
	} else
	{
		World->FlushDeferredParameterCollectionInstanceUpdates();

		FTextureRenderTargetResource* RenderTargetResource = TextureRenderTarget->GameThread_GetRenderTargetResource();

		UCanvas* Canvas = World->GetCanvasForDrawMaterialToRenderTarget();

#if	UE_VERSION_OLDER_THAN(5,3,0)
		FCanvas RenderCanvas(
			RenderTargetResource,
			nullptr,
			World,
			World->FeatureLevel);
#else
		FCanvas RenderCanvas(
			RenderTargetResource,
			nullptr,
			World,
			World->GetFeatureLevel());
#endif

		Canvas->Init(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY, nullptr, &RenderCanvas);
		Canvas->Update();

		FDrawEvent* DrawMaterialToTargetEvent = new FDrawEvent();

		FName RTName = TextureRenderTarget->GetFName();
		ENQUEUE_RENDER_COMMAND(BeginDrawEventCommand)(
			[RTName, DrawMaterialToTargetEvent, RenderTargetResource](FRHICommandListImmediate& RHICmdList)
		{
			RenderTargetResource->FlushDeferredResourceUpdate(RHICmdList);

			BEGIN_DRAW_EVENTF(
				RHICmdList,
				DrawCanvasToTarget,
				(*DrawMaterialToTargetEvent),
				*RTName.ToString());
		});

		Canvas->K2_DrawMaterial(Material, FVector2D(0, 0), FVector2D(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY), FVector2D(0, 0));

		RenderCanvas.Flush_GameThread();
		Canvas->Canvas = NULL;

		//UpdateResourceImmediate must be called here to ensure mips are generated.
		TextureRenderTarget->UpdateResourceImmediate(false);
		ENQUEUE_RENDER_COMMAND(CanvasRenderTargetResolveCommand)(
			[DrawMaterialToTargetEvent](FRHICommandList& RHICmdList)
		{
			STOP_DRAW_EVENT((*DrawMaterialToTargetEvent));
			delete DrawMaterialToTargetEvent;
		}
		);
	}
#else
	// 5.5.0
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(WorldContextObject, TextureRenderTarget, Material);
#endif
}

void UVrmBPFunctionLibrary::VRMUpdateTextureProperty(UTexture* Texture) {
	if (Texture == nullptr) return;

	Texture->UpdateResource();
#if WITH_EDITOR
	Texture->PostEditChange();
#endif
}

void UVrmBPFunctionLibrary::VRMChangeMaterialTexture(UMaterialInstanceConstant *dst, const FName texName, UTexture *texture) {
	bool bFound = false;

	for (auto &a : dst->TextureParameterValues) {
		if (a.ParameterInfo.Name != texName) {
			continue;
		}
		a.ParameterValue = texture;
		bFound = true;
		break;
	}

	if (bFound == false) {
		FTextureParameterValue *v = new (dst->TextureParameterValues) FTextureParameterValue();
		v->ParameterInfo.Index = INDEX_NONE;
		v->ParameterInfo.Name = texName;
		v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
		v->ParameterValue = texture;
		bFound = true;
	}

#if WITH_EDITOR
	if (bFound) {
		dst->MarkPackageDirty();
		dst->PreEditChange(NULL);
		dst->PostEditChange();
	}
#endif

}

void UVrmBPFunctionLibrary::VRMChangeMaterialParent(UMaterialInstanceConstant *dst, UMaterialInterface* NewParent, USkeletalMesh *UseSkeletalMesh) {
	if (dst == nullptr) {
		return;
	}

	if (dst->Parent == NewParent) {
		return;
	}
	dst->MarkPackageDirty();

	if (UseSkeletalMesh) {
		UseSkeletalMesh->MarkPackageDirty();
	}

#if WITH_EDITOR
	if (GIsEditor == false) {
		return;
	}

	dst->SetParentEditorOnly(NewParent);

	FMaterialUpdateContext UpdateContext(FMaterialUpdateContext::EOptions::Default, GMaxRHIShaderPlatform);
	UpdateContext.AddMaterialInstance(dst);

	dst->PreEditChange(NULL);
	dst->PostEditChange();

	// remove dynamic materials
	for (TObjectIterator<UMaterialInstanceDynamic> Itr; Itr; ++Itr) {
		if (Itr->Parent == dst) {
			Itr->ConditionalBeginDestroy();
		}
	}

#else
	dst->Parent = NewParent;
	dst->PostLoad();
#endif
}

// mat
#if WITH_EDITOR
namespace {
	static UMaterialInstanceConstant *LocalMaterialCache = nullptr;
}
#endif

void UVrmBPFunctionLibrary::VRMChangeMaterialSkipEditChange(UMaterialInstanceConstant *material, USkeletalMesh *UseSkeletalMesh, bool bSkip) {

#if WITH_EDITOR
	if (GIsEditor == false) {
		return;
	}
	if (material == nullptr) {
		return;
	}

	LocalMaterialCache = material;

	if (bSkip == false) {
		if (UseSkeletalMesh) {
			UseSkeletalMesh->PreEditChange(NULL);
		}
		material->PreEditChange(NULL);

	
		if (UseSkeletalMesh) {
			UseSkeletalMesh->PostEditChange();
		}
		material->PostEditChange();

		FMaterialUpdateContext UpdateContext(FMaterialUpdateContext::EOptions::Default, GMaxRHIShaderPlatform);
		UpdateContext.AddMaterialInstance(material);

		if (UseSkeletalMesh) {
			UseSkeletalMesh->MarkPackageDirty();
		}
		material->MarkPackageDirty();

		LocalMaterialCache = nullptr;
	}
#endif
}

void UVrmBPFunctionLibrary::VRMChangeMaterialShadingModel(UMaterialInstanceConstant *material, EMaterialShadingModel ShadingModel, bool bOverride, bool bForce) {
	if (material == nullptr) return;

#if WITH_EDITOR
	if (GIsEditor == false) {
		return;
	}

	bool bChange = bForce;

	if (bOverride) {
		const auto &p = material->Parent;

		if (p == nullptr) {
			bChange = true;
		} else {
#if	UE_VERSION_OLDER_THAN(4,23,0)
			auto s = p->GetShadingModel();
#else
			auto s = p->GetShadingModels().GetFirstShadingModel();
#endif

			if (s == ShadingModel) {
				bOverride = false;
			}
		}
	}
	{
#if	UE_VERSION_OLDER_THAN(4,23,0)
		auto s = material->GetShadingModel();
#else
		auto s = material->GetShadingModels().GetFirstShadingModel();
#endif

		bChange |= material->BasePropertyOverrides.bOverride_ShadingModel != bOverride;
		bChange |= s != ShadingModel;
	}


	if (bChange) {
		material->BasePropertyOverrides.bOverride_ShadingModel = bOverride;
		material->BasePropertyOverrides.ShadingModel = ShadingModel;

		if (LocalMaterialCache != material) {
			material->PreEditChange(NULL);
			material->PostEditChange();

			FMaterialUpdateContext UpdateContext(FMaterialUpdateContext::EOptions::Default, GMaxRHIShaderPlatform);
			UpdateContext.AddMaterialInstance(material);
			material->MarkPackageDirty();
		}
	}
#endif
}

void UVrmBPFunctionLibrary::VRMChangeMaterialBlendMode(UMaterialInstanceConstant *material, EBlendMode BlendMode, bool bOverride, bool bForce) {
	if (material == nullptr) return;

#if WITH_EDITOR
	if (GIsEditor == false) {
		return;
	}

	bool bChange = bForce;

	if (bOverride) {
		const auto& p = material->Parent;
		if (p == nullptr) {
			bChange = true;
		} else {
			if (p->GetBlendMode() == BlendMode) {
				bOverride = false;
			}
		}
	}


	bChange |= material->BasePropertyOverrides.bOverride_BlendMode != bOverride;
	bChange |= material->GetBlendMode() != BlendMode;


	if (bChange) {
		material->BasePropertyOverrides.bOverride_BlendMode = bOverride;
		material->BasePropertyOverrides.BlendMode = BlendMode;

		if (LocalMaterialCache != material) {

			material->PreEditChange(NULL);
			material->PostEditChange();

			FMaterialUpdateContext UpdateContext(FMaterialUpdateContext::EOptions::Default, GMaxRHIShaderPlatform);
			UpdateContext.AddMaterialInstance(material);
			material->MarkPackageDirty();
		}
	}
#endif
}

void UVrmBPFunctionLibrary::VRMChangeMaterialScalarParameter(UMaterialInstanceConstant *material, FName paramName, float param, bool bEnable) {
	if (material == nullptr) return;

#if WITH_EDITOR
	if (GIsEditor == false) {
		return;
	}

	FScalarParameterValue *v = nullptr;
	int i = -1;
	for (auto &a : material->ScalarParameterValues) {
		++i;
		if (a.ParameterInfo.Name == paramName) {
			v = &a;
			if (bEnable == false) {
				material->ScalarParameterValues.RemoveAt(i);

				{
					const auto a1 = material->ScalarParameterValues;
					const auto a2 = material->VectorParameterValues;
					const auto a3 = material->TextureParameterValues;
					const auto a4 = material->FontParameterValues;

					material->ClearParameterValuesEditorOnly();

					material->ScalarParameterValues = a1;
					material->VectorParameterValues = a2;
					material->TextureParameterValues = a3;
					material->FontParameterValues = a4;
				}

				if (LocalMaterialCache != material) {
					material->PreEditChange(NULL);
					material->PostEditChange();
				}
			}
			break;
		}
	}
	if (bEnable == false) {
		return;
	}
	if (v == nullptr) {
		v = new (material->ScalarParameterValues) FScalarParameterValue();
	}
	v->ParameterInfo.Index = INDEX_NONE;
	v->ParameterInfo.Name = paramName;
	v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
	v->ParameterValue = param;

	if (LocalMaterialCache != material) {
		material->PreEditChange(NULL);
		material->PostEditChange();
	}
#endif
}

void UVrmBPFunctionLibrary::VRMChangeMaterialVectorParameter(UMaterialInstanceConstant *material, FName paramName, FLinearColor param, bool bEnable) {
	if (material == nullptr) return;

#if WITH_EDITOR
	if (GIsEditor == false) {
		return;
	}

	FVectorParameterValue *v = nullptr;
	int i = -1;
	for (auto &a : material->VectorParameterValues) {
		++i;
		if (a.ParameterInfo.Name == paramName) {
			v = &a;
			if (bEnable == false) {
				material->VectorParameterValues.RemoveAt(i);

				{
					const auto a1 = material->ScalarParameterValues;
					const auto a2 = material->VectorParameterValues;
					const auto a3 = material->TextureParameterValues;
					const auto a4 = material->FontParameterValues;

					material->ClearParameterValuesEditorOnly();

					material->ScalarParameterValues = a1;
					material->VectorParameterValues = a2;
					material->TextureParameterValues = a3;
					material->FontParameterValues = a4;
				}

				if (LocalMaterialCache != material) {
					material->PreEditChange(NULL);
					material->PostEditChange();
				}
			}
			break;
		}
	}
	if (bEnable == false) {
		return;
	}
	if (v == nullptr) {
		v = new (material->VectorParameterValues) FVectorParameterValue();
	}
	v->ParameterInfo.Index = INDEX_NONE;
	v->ParameterInfo.Name = paramName;
	v->ParameterInfo.Association = EMaterialParameterAssociation::GlobalParameter;
	v->ParameterValue = param;

	if (LocalMaterialCache != material) {
		material->PreEditChange(NULL);
		material->PostEditChange();
	}
#endif
}

void UVrmBPFunctionLibrary::VRMChangeMaterialStaticSwitch(UMaterialInstanceConstant *material, FName paramName, bool bEnable) {
	if (material == nullptr) return;

#if WITH_EDITORONLY_DATA
	if (GIsEditor == false) {
		return;
	}

	TArray<FMaterialParameterInfo> OutParameterInfo;
	TArray<FGuid> OutParameterIds;
	material->GetAllStaticSwitchParameterInfo(OutParameterInfo, OutParameterIds);

	FStaticParameterSet paramSet;
	material->GetStaticParameterValues(paramSet);

	for (const auto &info : OutParameterInfo) {
		if (info.Name != paramName) continue;

		bool bDef = false;
		FGuid guid;
		material->GetStaticSwitchParameterDefaultValue(info, bDef, guid);


#if	UE_VERSION_OLDER_THAN(5,1,0)
		auto &params = paramSet.StaticSwitchParameters;
#elif	UE_VERSION_OLDER_THAN(5,2,0)
		auto& params = paramSet.EditorOnly.StaticSwitchParameters;
#else
		auto& params = paramSet.StaticSwitchParameters;
#endif
		int i = 0;
		for (i = 0; i < params.Num(); ++i) {
			if (params[i].ParameterInfo.Name == info.Name) {
				break;
			}
		}
		if (i < 0 || i >= params.Num()) {
			// default.
			if (bDef == bEnable) {
				continue;
			}
			i = params.AddDefaulted(1);

			params[i].ParameterInfo = info;
		}
		if (i < 0) {
			continue;
		}

		if (bEnable == bDef) {
			params.RemoveAt(i);
			i = -1;
		} else {
			params[i].bOverride = true;
			params[i].Value = bEnable;
		}
	}

	// apply always. for remove params.
	//if (paramSet.StaticSwitchParameters.Num() <= 0) {
	//	return;
	//}

	// update

	material->UpdateStaticPermutation(paramSet);
	if (LocalMaterialCache != material) {
		material->PreEditChange(NULL);
		material->PostEditChange();
	}
#endif
}

void UVrmBPFunctionLibrary::VRMGetMaterialStaticSwitch(UMaterialInstance* material, FName paramName, bool& bHasParam, bool& bEnable) {

	bHasParam = false;
	bEnable = false;
	if (material == nullptr) return;

#if WITH_EDITORONLY_DATA
	if (GIsEditor == false) {
		return;
	}

	bool Value = false;
	FGuid ExpressionGuid;
	if (material->GetStaticSwitchParameterValue(paramName, Value, ExpressionGuid))
	{
		bHasParam = true;
		bEnable = Value;
		return;
	}
	return;
#endif
}



UObject* UVrmBPFunctionLibrary::VRMDuplicateAsset(UObject *src, FString name, UObject *thisOwner) {
	if (src == nullptr) {
		return nullptr;
	}
	if (thisOwner == nullptr) {
		return nullptr;
	}

	auto *a = DuplicateObject<UObject>(src, thisOwner->GetOuter(), *name);
	return a;
}


void UVrmBPFunctionLibrary::VRMSetMaterial(USkeletalMesh *target, int no, UMaterialInterface *material) {
	if (target == nullptr) {
		return;
	}
	if (no < VRMGetMaterials(target).Num()) {
		VRMGetMaterials(target)[no].MaterialInterface = material;
	}
}

void UVrmBPFunctionLibrary::VRMSetImportedBounds(USkeletalMesh *target, FVector min, FVector max) {
	if (target == nullptr) {
		return;
	}
	FBox BoundingBox(min, max);
	target->SetImportedBounds(FBoxSphereBounds(BoundingBox));
}

bool UVrmBPFunctionLibrary::VRMGetAssetsByPackageName(FName PackageName, TArray<FAssetData>& OutAssetData, bool bIncludeOnlyOnDiskAssets){

	OutAssetData.Empty();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	auto &AssetRegistry = AssetRegistryModule.Get();

	return AssetRegistry.GetAssetsByPackageName(PackageName, OutAssetData, bIncludeOnlyOnDiskAssets);
}

void UVrmBPFunctionLibrary::VRMSetIsDirty(UObject* obj) {
	if (obj == nullptr) return;
	obj->MarkPackageDirty();
}

UTextureRenderTarget2D* UVrmBPFunctionLibrary::VRMCreateRenderTarget2D(UObject* WorldContextObject, int32 Width, int32 Height, ETextureRenderTargetFormat Format, FLinearColor ClearColor)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (Width > 0 && Height > 0 && World)
	{
		UTextureRenderTarget2D* NewRenderTarget2D = NewObject<UTextureRenderTarget2D>(WorldContextObject);
		check(NewRenderTarget2D);
		NewRenderTarget2D->RenderTargetFormat = Format;
		NewRenderTarget2D->ClearColor = ClearColor;
		NewRenderTarget2D->InitAutoFormat(Width, Height);
		NewRenderTarget2D->UpdateResourceImmediate(true);

		return NewRenderTarget2D;
	}

	return nullptr;
}

bool UVrmBPFunctionLibrary::VRMRenderingThreadEnable(bool bEnable) {
#if	UE_VERSION_OLDER_THAN(5,1,0)
	if (GIsThreadedRendering)
	{
		if (bEnable == false) {
			StopRenderingThread();
			GUseThreadedRendering = false;
		}
	} else
	{
		if (bEnable == true) {
			GUseThreadedRendering = true;
			StartRenderingThread();
		}
	}
#else
#endif
	return true;
}

int UVrmBPFunctionLibrary::VRMGetMeshSectionNum(const USkeletalMesh* mesh) {
	if (mesh == nullptr) return 0;
	return mesh->GetResourceForRendering()->LODRenderData[0].RenderSections.Num();

/*
	FSkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();
	for (int32 LODIndex = MinLODIndex; LODIndex < SkelMeshRenderData->LODRenderData.Num(); LODIndex++)
	{
		FSkeletalMeshLODRenderData& LODRenderData = SkelMeshRenderData->LODRenderData[LODIndex];
		const FSkeletalMeshLODInfo& Info = *(GetLODInfo(LODIndex));

		// Check all render sections for the used material indices
		for (int32 SectionIndex = 0; SectionIndex < LODRenderData.RenderSections.Num(); SectionIndex++)
		{
			FSkelMeshRenderSection& RenderSection = LODRenderData.RenderSections[SectionIndex];

			// By default use the material index of the render section


*/
}

bool UVrmBPFunctionLibrary::VRMRemoveMeshSection(USkeletalMesh* mesh, int LODIndex, int SectionIndex) {
#if WITH_EDITOR
	if (mesh == nullptr) return false;

	if (mesh->GetResourceForRendering()->LODRenderData.IsValidIndex(LODIndex)) {

		mesh->RemoveMeshSection(LODIndex, SectionIndex);
		mesh->MarkPackageDirty();
		return true;
	}
#endif
	return false;
}


bool UVrmBPFunctionLibrary::VRMGetShadowEnable(const USkeletalMesh *mesh, int MaterialIndex) {

	if (mesh == nullptr) {
		return false;
	}
	if (mesh->GetResourceForRendering() == nullptr) {
		return false;
	}

	const FSkeletalMeshLODRenderData &rd = mesh->GetResourceForRendering()->LODRenderData[0];

	bool bShadow = false;

	for (const auto &a : rd.RenderSections) {
		if (a.MaterialIndex != MaterialIndex) continue;
		if (a.bDisabled) continue;

		if (a.NumVertices == 0) return false;
		if (a.NumTriangles == 0) return false;

		bShadow |= a.bCastShadow;
	}

	return bShadow;
}

void UVrmBPFunctionLibrary::VRMSetLightingChannelPrim(UPrimitiveComponent* prim, bool bChannel0, bool bChannel1, bool bChannel2) {
	if (prim == nullptr) {
		return;
	}

#if	UE_VERSION_OLDER_THAN(4,25,0)
	prim->LightingChannels.bChannel0 = bChannel0;
	prim->LightingChannels.bChannel1 = bChannel1;
	prim->LightingChannels.bChannel2 = bChannel2;
#else
	prim->SetLightingChannels(bChannel0, bChannel1, bChannel2);
#endif
}

void UVrmBPFunctionLibrary::VRMSetLightingChannelLight(ULightComponent *light, bool bChannel0, bool bChannel1, bool bChannel2) {
	if (light == nullptr) {
		return;
	}
#if	UE_VERSION_OLDER_THAN(4,25,0)
	light->LightingChannels.bChannel0 = bChannel0;
	light->LightingChannels.bChannel1 = bChannel1;
	light->LightingChannels.bChannel2 = bChannel2;
#else
	light->SetLightingChannels(bChannel0, bChannel1, bChannel2);
#endif
}

void UVrmBPFunctionLibrary::VRMSetCastRaytracedShadow(ULightComponent *light, bool bEnable){
	if (light == nullptr) {
		return;
	}
#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
	light->SetCastRaytracedShadow(bEnable);
#endif
}

void UVrmBPFunctionLibrary::VRMSetShadowSlopeBias(ULightComponent *light, float bias) {
	if (light == nullptr) {
		return;
	}
#if	UE_VERSION_OLDER_THAN(4,23,0)
#else
	light->SetShadowSlopeBias(bias);
#endif
}

void UVrmBPFunctionLibrary::VRMSetSpecularScale(ULightComponent* light, float scale) {
	if (light == nullptr) {
		return;
	}
#if	UE_VERSION_OLDER_THAN(4,22,0)
#else
	light->SetSpecularScale(scale);
#endif
}


bool UVrmBPFunctionLibrary::VRMGetCameraFilmback(UCineCameraComponent *c, FCameraFilmbackSettings &s) {
	if (c) {
#if	UE_VERSION_OLDER_THAN(4,20,0)

#elif	UE_VERSION_OLDER_THAN(4,24,0)
		s = c->FilmbackSettings;
#else
		s = c->Filmback;
#endif
	
		return true;
	}
	return false;
}

bool UVrmBPFunctionLibrary::VRMSetCameraFilmback(UCineCameraComponent *c, const FCameraFilmbackSettings &s) {
	if (c) {
#if	UE_VERSION_OLDER_THAN(4,20,0)

#elif	UE_VERSION_OLDER_THAN(4,24,0)
		c->FilmbackSettings = s;
#else
		c->Filmback= s;
#endif
		return true;
	}
	return false;
}

bool UVrmBPFunctionLibrary::VRMSetPostProcessSettingFromCineCamera(FPostProcessSettings &OutSettings, const FPostProcessSettings &InSettings, const UCineCameraComponent *CineCamera){
	if (CineCamera == nullptr) {
		return false;
	}

	OutSettings = InSettings;

#if	UE_VERSION_OLDER_THAN(4,20,0)
#else

	OutSettings.bOverride_DepthOfFieldFstop = true;
	OutSettings.DepthOfFieldFstop = CineCamera->CurrentAperture;

	OutSettings.bOverride_DepthOfFieldMinFstop = true;
	OutSettings.DepthOfFieldMinFstop = CineCamera->LensSettings.MinFStop;

	OutSettings.bOverride_DepthOfFieldBladeCount = true;
	OutSettings.DepthOfFieldBladeCount = CineCamera->LensSettings.DiaphragmBladeCount;

	OutSettings.bOverride_DepthOfFieldFocalDistance = true;
	OutSettings.DepthOfFieldFocalDistance = CineCamera->CurrentFocusDistance;

#if	UE_VERSION_OLDER_THAN(4,24,0)
	OutSettings.bOverride_DepthOfFieldSensorWidth = true;
	OutSettings.DepthOfFieldSensorWidth = CineCamera->FilmbackSettings.SensorWidth;
#else
	OutSettings.bOverride_DepthOfFieldSensorWidth = true;
	OutSettings.DepthOfFieldSensorWidth = CineCamera->Filmback.SensorWidth;
#endif

#endif	// 4.20-

	return true;
}

void UVrmBPFunctionLibrary::VRMSetPostProcessToneCurveAmount(FPostProcessSettings& OutSettings, const FPostProcessSettings& InSettings, bool bOverride, float Amount) {
	OutSettings = InSettings;

#if	UE_VERSION_OLDER_THAN(4,26,0)

#else
	OutSettings.ToneCurveAmount = Amount;
	OutSettings.bOverride_ToneCurveAmount = bOverride;
#endif
}


void UVrmBPFunctionLibrary::VRMMakeCameraTrackingFocusSettings(AActor *ActorToTrack, FVector RelativeOffset, bool bDrawDebugTrackingFocusPoint, FCameraTrackingFocusSettings &Settings){
#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
	Settings.ActorToTrack = ActorToTrack;
	Settings.RelativeOffset = RelativeOffset;
	Settings.bDrawDebugTrackingFocusPoint = bDrawDebugTrackingFocusPoint;
#endif
}

void UVrmBPFunctionLibrary::VRMSetActorLabel(AActor *Actor, const FString& NewActorLabel) {
	if (Actor == nullptr) return;

#if WITH_EDITOR
	bool b1 = false;
	bool b2 = false;
	bool b3 = false;
	VRMGetPlayMode(b1, b2, b3);
	if (b1 == false) {
		Actor->SetActorLabel(NewActorLabel);
	}
#endif
}



void UVrmBPFunctionLibrary::VRMGetCameraTransform(const UObject* WorldContextObject, int32 PlayerIndex, bool bGameOnly, FTransform &transform, float &fovDegree) {

	bool bSet = false;
	transform.SetIdentity();
	fovDegree = 0;

	auto *c = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, PlayerIndex);

#if WITH_EDITOR
	if (bGameOnly == false) {
		if (GEditor) {
			if (GEditor->bIsSimulatingInEditor || c==nullptr) {
				if (GEditor->GetActiveViewport()) {
					FEditorViewportClient* ViewportClient = StaticCast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
					if (ViewportClient) {
						if (ViewportClient->AspectRatio > 0.f) {
							const auto &a = ViewportClient->ViewTransformPerspective;
							transform.SetLocation(a.GetLocation());
							transform.SetRotation(a.GetRotation().Quaternion());
							fovDegree = ViewportClient->ViewFOV;
							bSet = true;
						}
					}
				}
			}
		}
	}
#endif
	if (bSet == false) {
		if (c) {
			transform.SetLocation(c->GetCameraLocation());
			transform.SetRotation(c->GetCameraRotation().Quaternion());
			fovDegree = c->GetFOVAngle();
		}
	}
}


void UVrmBPFunctionLibrary::VRMGetPlayMode(bool &bPlay, bool &bSIE, bool &bEditor) {
	bPlay = false;
	bSIE = false;
	bEditor = false;
#if WITH_EDITOR
	if (GEditor) {
		if (GEditor->bIsSimulatingInEditor) {
			bSIE = true;
		}
	}
	if (GIsEditor) {
		bEditor = true;
	}
	if (GIsEditor == false) {
		bPlay = true;
	}
	if (GEngine) {
		if (GEngine->GameViewport) {
			bPlay = true;
		}
	}

	if (GWorld) {
		if (GWorld->HasBegunPlay() && GWorld->IsGameWorld()) {
			bPlay = true;
		}
	}

#else
	bPlay = true;
#endif
}

bool UVrmBPFunctionLibrary::VRMGetAnimationAssetData(const UAnimationAsset *animAsset, USkeleton *&skeleton) {
	if (animAsset == nullptr) {
		return false;
	}
	skeleton = animAsset->GetSkeleton();

	return true;
}

namespace {
	bool setTransParent(bool bEnable, FLinearColor crKey) {
#if PLATFORM_WINDOWS
		if (GEngine == nullptr)					return false;
		if (GEngine->GameViewport == nullptr)	return false;
		if (GEngine->GameViewport->GetWindow().IsValid() == false) return false;
		if (GEngine->GameViewport->GetWindow()->GetNativeWindow().IsValid() == false) return false;

#if WITH_EDITOR
		if (GEditor == nullptr)  return false;
		if (GEditor->GetActiveViewport() == nullptr) return false;
		if (GEditor->GetActiveViewport()->GetClient() == GEngine->GameViewport) {
			// skip editor viewport for cpu load
			// editor style cannnot be restored
			return false;
		}
#endif
		HWND h = reinterpret_cast<HWND>(GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle());

		if (h == nullptr) {
			return false;
		}

		{
			LONG lStyle = ::GetWindowLong(h, GWL_STYLE);
			LONG lexStyle = ::GetWindowLong(h, GWL_EXSTYLE);
			if (bEnable) {
				lStyle &= (~WS_BORDER);
				lStyle &= (~WS_THICKFRAME);
				lStyle &= (~WS_CAPTION);
				lStyle &= (~WS_OVERLAPPEDWINDOW);
				

				lexStyle |= WS_EX_LAYERED;
				//lexStyle &= (~WS_EX_WINDOWEDGE);
				//lexStyle &= (~WS_EX_CLIENTEDGE);
			} else {
				lStyle |= WS_BORDER;
				lStyle |= (WS_THICKFRAME);
				lStyle |= (WS_CAPTION);
				lStyle |= (WS_OVERLAPPEDWINDOW);

				lexStyle &= (~WS_EX_LAYERED);
				//lexStyle |= (WS_EX_WINDOWEDGE);
				//lexStyle |= (WS_EX_CLIENTEDGE);
			}
			//::SetWindowLong(h, GWL_STYLE, lStyle);
			::SetWindowLong(h, GWL_EXSTYLE, lexStyle);

			//SetWindowPos(h, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE |SWP_NOZORDER | SWP_FRAMECHANGED);
		}

		uint32_t cr = 0;
		cr |= ((int)(crKey.R*255.f) & 0xFF);
		cr |= ((int)(crKey.G*255.f) & 0xFF) << 8;
		cr |= ((int)(crKey.B*255.f) & 0xFF) << 16;

		uint8_t a = 0;
		a |= ((int)(crKey.A*255.f) & 0xFF);

		DWORD dwFlags = LWA_ALPHA;
		if (bEnable) {
			dwFlags = LWA_COLORKEY;
		} else {
			cr = 0;
			a = 255;
		}
		BOOL b = SetLayeredWindowAttributes(h, cr, a, dwFlags);
		return b != 0;
#endif
		return false;
	}
	void setDefaultWindow(const bool) {
		setTransParent(false, FLinearColor(1, 1, 1, 1));
	}
}

bool UVrmBPFunctionLibrary::VRMSetTransparentWindow(bool bEnable, FLinearColor crKey) {
	bool ret = false;
#if PLATFORM_WINDOWS
	ret = setTransParent(bEnable, crKey);

#if WITH_EDITOR
	if (bEnable) {
		static bool bFirst = true;
		if (bFirst) {
			bFirst = false;
			FEditorDelegates::PrePIEEnded.AddStatic(&setDefaultWindow);
		}
	}
#endif

#endif
	return ret;
}

void UVrmBPFunctionLibrary::VRMExecuteConsoleCommand(UObject* WorldContextObject, const FString &cmd){
	if (cmd.IsEmpty()) {
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr) {
		return;
	}

#if	UE_VERSION_OLDER_THAN(5,5,0)

	FOutputDevice& EffectiveOutputDevice = (FOutputDevice&)(*GLog);
	FConsoleManager& ConsoleManager = (FConsoleManager&)IConsoleManager::Get();
#else

	FOutputDevice& EffectiveOutputDevice = (FOutputDevice&)(*GLog);
	auto& ConsoleManager = IConsoleManager::Get();
#endif
	ConsoleManager.ProcessUserConsoleInput(&(cmd.GetCharArray()[0]), EffectiveOutputDevice, World);

}


bool UVrmBPFunctionLibrary::VRMAddTickPrerequisite(UActorComponent *dst, UActorComponent *src, bool bRemove) {
	if (dst == nullptr || src == nullptr) {
		return false;
	}

	if (bRemove == false) {
		dst->PrimaryComponentTick.AddPrerequisite(src, src->PrimaryComponentTick);
	} else {
		dst->PrimaryComponentTick.RemovePrerequisite(src, src->PrimaryComponentTick);
	}
	return true;
}

void UVrmBPFunctionLibrary::VRMAllowTranslucentSelection(bool bEnable) {
#if WITH_EDITOR
	{
		auto* Settings = GetMutableDefault<UEditorPerProjectUserSettings>();

		if (Settings == nullptr) {
			return;
		}
		if (Settings->bAllowSelectTranslucent == bEnable) {
			return;
		}

		FLevelEditorActionCallbacks::OnAllowTranslucentSelection();
	}
#endif

}


void UVrmBPFunctionLibrary::VRMSetWidgetMode(const EVRMWidgetMode mode) {
#if WITH_EDITOR
/*
	if (GEditor) {
		if (GEditor->GetActiveViewport()) {
			FEditorViewportClient* ViewportClient = StaticCast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
			if (ViewportClient) {
				FWidget::EWidgetMode tmp[] = {
					FWidget::WM_Translate,
					FWidget::WM_Rotate,
					FWidget::WM_Scale,
				};
				ViewportClient->SetWidgetMode(tmp[(int)mode]);
			}
		}
	}
	*/
#endif
}

/*
bool UVrmBPFunctionLibrary::VRMLiveLinkEvaluate(const FName Name, TArray<FName> & PropertyNames, TArray<float> & PropertyValues, int &FrameNo){
	const FLiveLinkSubjectName Subject(Name);
	PropertyNames.Empty();
	PropertyValues.Empty();
	FrameNo = 0;

	IModularFeatures& ModularFeatures = IModularFeatures::Get();
	if (ModularFeatures.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		ILiveLinkClient& LiveLinkClient = ModularFeatures.GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
		FLiveLinkSubjectFrameData FrameData;

		//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Live Link")
		TSubclassOf<ULiveLinkRole> Role = ULiveLinkBasicRole::StaticClass();

		if (LiveLinkClient.EvaluateFrame_AnyThread(Subject, Role, FrameData) == false) {
			return false;
		}

		{
			auto* p = FrameData.StaticData.GetBaseData();
			if (p) {
				PropertyNames = p->PropertyNames;
			}
		}
		{
			auto* p = FrameData.FrameData.GetBaseData();
			if (p) {
				PropertyValues = p->PropertyValues;
				FrameNo = p->MetaData.SceneTime.Time.GetFrame().Value;

			}
		}
		return true;
	}

	return false;
}
*/


void UVrmBPFunctionLibrary::VRMSetPostProcessSettingAO(FPostProcessSettings& OutSettings, const FPostProcessSettings& InSettings, bool bOverride, float AOIntensity, float Radius, bool bRayTracing){

	OutSettings = InSettings;

	FPostProcessSettings &o = OutSettings;

	o.AmbientOcclusionRadius = Radius;
	o.AmbientOcclusionIntensity = AOIntensity;
	o.bOverride_AmbientOcclusionRadius = bOverride;
	o.bOverride_AmbientOcclusionIntensity = bOverride;

#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
	o.RayTracingAO = bRayTracing;
	o.bOverride_RayTracingAO = bOverride;
#endif

#if	UE_VERSION_OLDER_THAN(4,26,0)
#else
	o.RayTracingAORadius = Radius;
	o.RayTracingAOIntensity = AOIntensity;
	o.bOverride_RayTracingAORadius = bOverride;
	o.bOverride_RayTracingAOIntensity = bOverride;
#endif
}


////

bool UVrmBPFunctionLibrary::VRMBakeAnim(const USkeletalMeshComponent* skc, const FString& FilePath2, const FString& AssetFileName2){
#if WITH_EDITOR
	if (skc == nullptr) {
		return false;
	}

	FString FilePath = FilePath2;
	FString AssetFileName = AssetFileName2;

	FString dummy[2] = {
		"/Game/",
		"tmpAnimSequence",
	};
	if (FilePath == "") {
		FilePath = dummy[0];
	}
	if (AssetFileName == "") {
		AssetFileName = dummy[1];
	}
	FilePath += TEXT("/");

	while (FilePath.Find(TEXT("//")) >= 0) {
		FilePath = FilePath.Replace(TEXT("//"), TEXT("/"));
	}

	while (AssetFileName.Find(TEXT("/")) >= 0) {
		AssetFileName = AssetFileName.Replace(TEXT("/"), TEXT(""));
	}

	USkeletalMesh* sk = VRMGetSkinnedAsset(skc);
	USkeleton* k = VRMGetSkeleton( VRMGetSkinnedAsset(skc) );

	//FString NewPackageName = "/Game/aaaa";
	FString NewPackageName = FilePath + AssetFileName;
#if	UE_VERSION_OLDER_THAN(4,26,0)
	UPackage* Package = CreatePackage(nullptr, *NewPackageName);
#else
	UPackage* Package = CreatePackage(*NewPackageName);
#endif

	UAnimSequence* ase;
	ase = NewObject<UAnimSequence>(Package, *(TEXT("A_") + AssetFileName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

#if	UE_VERSION_OLDER_THAN(5,0,0)
	ase->CleanAnimSequenceForImport();
#else
	ase->GetController().ResetModel();
#endif

	ase->SetSkeleton(k);

	float totalTime = 0.f;
	int totalFrameNum = 0;

	const auto componentTransInv = skc->GetComponentTransform().Inverse();

	for (int i = 0; i < k->GetRefLocalPoses().Num(); ++i) {
		FRawAnimSequenceTrack RawTrack;

		const auto BoneName = skc->GetBoneName(i);

		FTransform parentBoneInv = FTransform::Identity;
		if (i > 0) {
			const auto ParentBoneName = skc->GetParentBone(BoneName);
			const auto parentTrans = skc->GetBoneTransform(skc->GetBoneIndex(ParentBoneName));
			auto parentCompTrans = parentTrans * componentTransInv;

			//auto r = parentCompTrans.GetRotation();
			//r = FRotator(0, -90, 0).Quaternion() * r * FRotator(0, 90, 0).Quaternion();
			//parentCompTrans.SetRotation(r);

			parentBoneInv = parentCompTrans.Inverse();
			{

			}
		}

		{
			const auto refPose = VRMGetRefSkeleton(sk).GetRawRefBonePose()[i];
#if	UE_VERSION_OLDER_THAN(5,0,0)
			RawTrack.PosKeys.Add(refPose.GetLocation());
#else
			RawTrack.PosKeys.Add(FVector3f(refPose.GetLocation()));
#endif

		}

		const auto srcTrans = skc->GetBoneTransform(i);
		auto compTrans = srcTrans * componentTransInv;
		{
			//auto r = compTrans.GetRotation();
			//r = FRotator(0, -90, 0).Quaternion() * r * FRotator(0, 90, 0).Quaternion();
			//compTrans.SetRotation(r);
		}
		const auto boneTrans = compTrans * parentBoneInv;

#if	UE_VERSION_OLDER_THAN(5,0,0)
		FQuat q = boneTrans.GetRotation();
#else
		FQuat4f q(boneTrans.GetRotation());
#endif

		RawTrack.RotKeys.Add(q);

		//FVector s = srcTrans.GetScale3D();
		FVector s(1, 1, 1);
#if	UE_VERSION_OLDER_THAN(5,0,0)
		RawTrack.ScaleKeys.Add(s);
		if (RawTrack.PosKeys.Num() == 0) {
			RawTrack.PosKeys.Add(FVector::ZeroVector);
		}
		if (RawTrack.RotKeys.Num() == 0) {
			RawTrack.RotKeys.Add(FQuat::Identity);
		}
		if (RawTrack.ScaleKeys.Num() == 0) {
			RawTrack.ScaleKeys.Add(FVector::OneVector);
		}
#else
		RawTrack.ScaleKeys.Add(FVector3f(s));
		if (RawTrack.PosKeys.Num() == 0) {
			RawTrack.PosKeys.Add(FVector3f::ZeroVector);
		}
		if (RawTrack.RotKeys.Num() == 0) {
			RawTrack.RotKeys.Add(FQuat4f::Identity);
		}
		if (RawTrack.ScaleKeys.Num() == 0) {
			RawTrack.ScaleKeys.Add(FVector3f::OneVector);
		}
#endif

#if	UE_VERSION_OLDER_THAN(5,0,0)
		ase->AddNewRawTrack(BoneName, &RawTrack);
#elif UE_VERSION_OLDER_THAN(5,2,0)
		ase->GetController().AddBoneTrack(BoneName);
		ase->GetController().SetBoneTrackKeys(BoneName, RawTrack.PosKeys, RawTrack.RotKeys, RawTrack.ScaleKeys);
#else
		ase->GetController().AddBoneCurve(BoneName);
		ase->GetController().SetBoneTrackKeys(BoneName, RawTrack.PosKeys, RawTrack.RotKeys, RawTrack.ScaleKeys);
#endif

		totalFrameNum = 1;
		totalTime = 1;

#if	UE_VERSION_OLDER_THAN(4,22,0)
		ase->NumFrames = totalFrameNum;
#elif	UE_VERSION_OLDER_THAN(5,0,0)
		ase->SetRawNumberOfFrame(totalFrameNum);
#else
		ase->GetController().SetFrameRate(FFrameRate(totalFrameNum, 1));
#endif

#if	UE_VERSION_OLDER_THAN(5,0,0)
		ase->SequenceLength = totalTime;
		ase->MarkRawDataAsModified();
#elif UE_VERSION_OLDER_THAN(5,2,0)
		ase->GetController().SetPlayLength(totalTime);
		ase->SetUseRawDataOnly(true);
		ase->FlagDependentAnimationsAsRawDataOnly();
		ase->UpdateDependentStreamingAnimations();
#elif UE_VERSION_OLDER_THAN(5,6,0)
		ase->GetController().SetNumberOfFrames(ase->GetController().ConvertSecondsToFrameNumber(totalTime));
		ase->SetUseRawDataOnly(true);
		ase->FlagDependentAnimationsAsRawDataOnly();
		ase->UpdateDependentStreamingAnimations();
#else
		ase->GetController().SetNumberOfFrames(ase->GetController().ConvertSecondsToFrameNumber(totalTime));
		//ase->SetUseRawDataOnly(true);
		ase->FlagDependentAnimationsAsRawDataOnly();
		ase->UpdateDependentStreamingAnimations();
#endif


	}

#if	UE_VERSION_OLDER_THAN(5,0,0)
	const bool bSourceDataExists = ase->HasSourceRawData();
	if (bSourceDataExists)
	{
		ase->BakeTrackCurvesToRawAnimation();
	} else {
		ase->PostProcessSequence();
	}
#elif	UE_VERSION_OLDER_THAN(5,1,0)
	ase->PostProcessSequence();
	ase->MarkRawDataAsModified(true);
	ase->OnRawDataChanged();
	ase->MarkPackageDirty();
#else
		// todo
#endif

#endif // editor

	return true;
}


void UVrmBPFunctionLibrary::VRMGetAllActorsHasSceneComponent(const UObject* WorldContextObject, TSubclassOf<UObject> UObjectClass, TArray<AActor*>& OutActors){

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (UObjectClass && World)
	{
		for (TObjectIterator<USceneComponent> it; it; ++it) {
			USceneComponent *c = *it;
			if (c->GetWorld() != World) continue;
			if (c->IsA(UObjectClass) == false) continue;
			

			auto a = c->GetOwner();
			if (a == nullptr) continue;
			if (IsValid(a) == false) continue;

			OutActors.AddUnique(a);
		}
	}
}

void UVrmBPFunctionLibrary::VRMGetRigNodeNameFromBoneName(const USkeleton* skeleton, const FName& boneName, FName& RigNodeName){
#if WITH_EDITOR
#if UE_VERSION_OLDER_THAN(5,4,0)
	RigNodeName = skeleton->GetRigNodeNameFromBoneName(boneName);
#else
	RigNodeName = NAME_None;
#endif
#endif
}

void UVrmBPFunctionLibrary::VRMSetPerBoneMotionBlur(USkinnedMeshComponent* SkinnedMesh, bool bUsePerBoneMotionBlur) {
	if (SkinnedMesh == nullptr) return;

	SkinnedMesh->bPerBoneMotionBlur = bUsePerBoneMotionBlur;
}


//void UVrmBPFunctionLibrary::GetIKRigDefinition(UIKRetargeter, UIKRigDefinition*& src, UIKRigDefinition*& target) {
void UVrmBPFunctionLibrary::VRMGetIKRigDefinition(UObject *retargeter, UObject * &src, UObject * &target) {
#if	UE_VERSION_OLDER_THAN(5,0,0)
#elif UE_VERSION_OLDER_THAN(5,4,0)
	UIKRetargeter* r = Cast<UIKRetargeter>(retargeter);
	if (r) {
		src = Cast<UIKRigDefinition>(r->GetSourceIKRigWriteable());
		target = Cast <UIKRigDefinition>(r->GetTargetIKRigWriteable());
	}
#else
	UIKRetargeter* r = Cast<UIKRetargeter>(retargeter);
	if (r) {
		src = Cast<UIKRigDefinition>(r->GetIKRigWriteable(ERetargetSourceOrTarget::Source));
		target = Cast <UIKRigDefinition>(r->GetIKRigWriteable(ERetargetSourceOrTarget::Target));
	}
#endif
}

void UVrmBPFunctionLibrary::VRMGetPreviewMesh(UObject* target, USkeletalMesh*& mesh){
	if (target == nullptr) return;

	IInterface_PreviewMeshProvider *p = Cast<IInterface_PreviewMeshProvider>(target);
	if (p == nullptr) return;

	mesh = p->GetPreviewMesh();
}

void UVrmBPFunctionLibrary::VRMGetSkeletalMeshFromSkinnedMeshComponent(const USkinnedMeshComponent* target, USkeletalMesh*& skeletalmesh) {
	if (target == nullptr) {
		skeletalmesh = nullptr;
		return;
	}
#if	UE_VERSION_OLDER_THAN(5,1,0)
	skeletalmesh = target->SkeletalMesh;
#else
	skeletalmesh = Cast<USkeletalMesh>(target->GetSkinnedAsset());
#endif
}

void UVrmBPFunctionLibrary::VRMGetTopLevelAssetName(const FAssetData& target, FName& AssetName) {
	AssetName = NAME_None;


#if	UE_VERSION_OLDER_THAN(5,1,0)
	AssetName = target.AssetClass;
#else
	AssetName = target.AssetClassPath.GetAssetName();
#endif

}


UVrmAssetListObject* UVrmBPFunctionLibrary::VRMGetVrmAssetListObjectFromAsset(const UObject* Asset) {
	return VRMUtil::GetAssetListObjectAny(Asset);
}


bool UVrmBPFunctionLibrary::VRMIsMovieRendering() {
#if VRM4U_USE_MRQ
	if (GEditor) {
		UMoviePipelineQueueSubsystem* s = GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
		if (s == nullptr) return false;

		return s->IsRendering();
	}
#endif
	return false;
}

bool UVrmBPFunctionLibrary::VRMIsTemporaryObject(const UObject *obj) {
	if (obj == nullptr) return true;

	if (obj->HasAnyFlags(EObjectFlags::RF_Transient)) {
		return true;
	}
	return false;
}

bool UVrmBPFunctionLibrary::VRMIsEditorPreviewObject(const UObject* obj) {
	if (obj == nullptr) return true;

	const UWorld* World = obj->GetWorld();

	if (World == nullptr) return true;

	if (World->WorldType == EWorldType::EditorPreview)
	{
		return true;
	}

	return false;
}


