// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertHumanoid.h"
#include "VrmConvert.h"
#include "VrmUtil.h"

#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"

#include "Engine/SkeletalMesh.h"
#include "RenderingThread.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"
#include "Animation/Skeleton.h"
#include "Animation/NodeMappingContainer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>


static void renameToHumanoidBone(USkeletalMesh *targetSK, const UVrmMetaObject *meta, const USkeletalMesh *srcSK) {

	USkeleton *targetSkeleton = VRMGetSkeleton(targetSK);
	UNodeMappingContainer* rig = nullptr;
	if (VRMGetNodeMappingData(targetSK).Num()) {
		rig = VRMGetNodeMappingData(targetSK)[0];
	}
	UNodeMappingContainer* src_rig = nullptr;
	if (VRMGetNodeMappingData(srcSK).Num()) {
		src_rig = VRMGetNodeMappingData(srcSK)[0];
	}

	if (meta == nullptr) {
		return;
	}
	//k->RemoveBonesFromSkeleton()
	auto &allbone = const_cast<TArray<FMeshBoneInfo> &>(targetSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo());

	TMap<FName, FName> changeTable;

	{
		bool bHasRoot = false;
		for (auto& a : allbone) {
			if (a.Name == TEXT("Root")) {
				a.Name = TEXT("root");
#if WITH_EDITORONLY_DATA
				a.ExportName = TEXT("root");
#endif
				bHasRoot = true;
				break;
			}
		}
		if (bHasRoot == false) {
			if (allbone[0].Name != TEXT("hip")) {
				allbone[0].Name = TEXT("root");
#if WITH_EDITORONLY_DATA
				allbone[0].ExportName = TEXT("root");
#endif
			}
		}
	}

	for (auto &a : allbone) {
		auto p = meta->humanoidBoneTable.FindKey(a.Name.ToString());
		if (p == nullptr) {
			continue;
		}
		if (p->IsEmpty()) {
			continue;
		}
		for (auto &b : allbone) {
			if (a == b) continue;
			if (b.Name == **p) {
				b.Name = *(b.Name.ToString() + TEXT("_renamed_vrm4u"));
			}
		}
		changeTable.Add(a.Name, **p);

		a.Name = **p;
#if WITH_EDITORONLY_DATA
		a.ExportName = **p;
#endif
	}

	const_cast<FReferenceSkeleton&>(targetSkeleton->GetReferenceSkeleton()).RebuildRefSkeleton(targetSkeleton, true);

	VRMSetRefSkeleton(targetSK, targetSkeleton->GetReferenceSkeleton());

#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(4,20,0)
#elif UE_VERSION_OLDER_THAN(5,4,0)
	if (src_rig) {
		rig->SetSourceAsset(src_rig->GetSourceAsset());
		rig->SetTargetAsset(targetSK);

		targetSkeleton->RefreshRigConfig();

		{
			auto &table = src_rig->GetNodeMappingTable();

			for (auto &t : table) {
				FName n = t.Value;
				{
					auto *ret = changeTable.Find(t.Value);
					if (ret) {
						n = *ret;
					}
				}

				rig->AddMapping(t.Key, n);
				targetSkeleton->SetRigBoneMapping(t.Key, n);
			}
		}
		rig->PostEditChange();
	}
#else
	//  ue5.4 no rig data
#endif
#endif
}

static void renameToUE4Bone(USkeletalMesh *targetSK, UVrmMetaObject *meta, const USkeletalMesh *srcSK) {

	USkeleton *targetSkeleton = VRMGetSkeleton(targetSK);
	UNodeMappingContainer* rig = nullptr;
	if (VRMGetNodeMappingData(targetSK).Num()) {
		rig = VRMGetNodeMappingData(targetSK)[0];
	}
	UNodeMappingContainer* src_rig = nullptr;
	if (VRMGetNodeMappingData(srcSK).Num()) {
		src_rig = VRMGetNodeMappingData(srcSK)[0];
	}

	//k->RemoveBonesFromSkeleton()
	auto &allbone = const_cast<TArray<FMeshBoneInfo> &>(targetSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo());

	TMap<FName, FName> changeTable;

	allbone[0].Name = TEXT("root");
#if WITH_EDITORONLY_DATA
	allbone[0].ExportName = TEXT("root");
#endif

	for (auto &a : allbone) {

		FString toVRMBone;
		for (auto& tmp : meta->humanoidBoneTable) {
			if (a.Name.ToString().ToLower() != tmp.Value.ToLower()) continue;

			toVRMBone = tmp.Key;
			break;
		}
		if (toVRMBone == "") continue;

		FString toUE4Bone;
		for (auto& tmp : VRMUtil::table_ue4_vrm) {
			if (toVRMBone.ToLower() != tmp.BoneVRM.ToLower()) continue;

			toUE4Bone = tmp.BoneUE4;
			break;
		}
		if (toUE4Bone == "") continue;

		FString newName = toUE4Bone;

		for (auto &b : allbone) {
			if (a == b) continue;
			if (b.Name == *(newName.ToLower())) {
				b.Name = *(b.Name.ToString() + TEXT("_renamed_vrm4u"));
			}
		}
		changeTable.Add(a.Name, *newName);

		a.Name = *newName;
#if WITH_EDITORONLY_DATA
		a.ExportName = *newName;
#endif
	}

	const_cast<FReferenceSkeleton&>(targetSkeleton->GetReferenceSkeleton()).RebuildRefSkeleton(targetSkeleton, true);

	VRMSetRefSkeleton(targetSK, targetSkeleton->GetReferenceSkeleton());

#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(4,20,0)
#elif UE_VERSION_OLDER_THAN(5,4,0)

	if (src_rig) {
		rig->SetSourceAsset(src_rig->GetSourceAsset());
		rig->SetTargetAsset(targetSK);

		targetSkeleton->RefreshRigConfig();

		{
			auto &table = src_rig->GetNodeMappingTable();

			for (auto &t : table) {
				FName n = t.Value;
				{
					auto *ret = changeTable.Find(t.Value);
					if (ret) {
						n = *ret;
					}
				}

				rig->AddMapping(t.Key, n);
				targetSkeleton->SetRigBoneMapping(t.Key, n);
			}
		}
		rig->PostEditChange();
	}
#else
	// ue5.4 no rig data
#endif
#endif
}



bool VRMConverter::ConvertHumanoid(UVrmAssetListObject *vrmAssetList) {
	if (Options::Get().IsGenerateHumanoidRenamedMesh() == false) {
		return true;
	}
	const USkeletalMesh *src_sk = vrmAssetList->SkeletalMesh;
	const USkeleton *src_k = VRMGetSkeleton(src_sk);
	const UNodeMappingContainer *src_rig = nullptr;
	if (VRMGetNodeMappingData(src_sk).Num()) {
		src_rig = VRMGetNodeMappingData(src_sk)[0];
	}

	USkeletalMesh *new_sk[2] = {};
	USkeleton *new_k[2] = {};
	UNodeMappingContainer *new_rig[2] = {};

	// 0: humanoid
	// 1: ue4mannequin
	for (int i = 0; i < 2; ++i) {
		FString name_skeleton;
		FString name_mesh;
		FString name_rig;

		if (i == 0) {
			name_skeleton = (TEXT("SKEL_") + vrmAssetList->BaseFileName + TEXT("_humanoid"));
			name_mesh = (FString(TEXT("SK_")) + vrmAssetList->BaseFileName + TEXT("_humanoid"));
			name_rig = (FString(TEXT("RIG_")) + vrmAssetList->BaseFileName + TEXT("_humanoid"));
		} else {
			name_skeleton = (TEXT("SKEL_") + vrmAssetList->BaseFileName + TEXT("_ue4mannequin"));
			name_mesh = (FString(TEXT("SK_")) + vrmAssetList->BaseFileName + TEXT("_ue4mannequin"));
			name_rig = (FString(TEXT("RIG_")) + vrmAssetList->BaseFileName + TEXT("_ue4mannequin"));
		}

		USkeleton *base = nullptr;
		USkeletalMesh *ss = nullptr;
		UNodeMappingContainer *rr = nullptr;

#if WITH_EDITOR
		{
			auto p = vrmAssetList->Package;

			if (i == 0) {
				ss = VRM4U_DuplicateObject<USkeletalMesh>(src_sk, p, *name_mesh);
				base = VRM4U_DuplicateObject<USkeleton>(src_k, p, *name_skeleton);
				if (src_rig) {
					rr = VRM4U_NewObject<UNodeMappingContainer>(p, *name_rig, RF_Public | RF_Standalone);
				}
			}else {
				ss = VRM4U_DuplicateObject<USkeletalMesh>(src_sk, p, *name_mesh);
				base = VRM4U_DuplicateObject<USkeleton>(src_k, p, *name_skeleton);
				if (src_rig) {
					rr = VRM4U_NewObject<UNodeMappingContainer>(p, *name_rig, RF_Public | RF_Standalone);
				}
			}
		}
#else
		ss = const_cast<USkeletalMesh*>(src_sk);
		base = const_cast<USkeleton*>(src_k);
		if (i == 1) continue;
#endif

		new_sk[i] = ss;
		new_k[i] = base;
		new_rig[i] = rr;

		VRMSetSkeleton(ss, base);
		VRMGetNodeMappingData(ss).Empty();
		VRMGetNodeMappingData(ss).Add(rr);


		if (i == 0) {
			// org -> humanoid
			renameToHumanoidBone(ss, vrmAssetList->VrmMetaObject, src_sk);
		} else {
			// humanoid -> ue4mannequin
			renameToUE4Bone(ss, vrmAssetList->VrmMetaObject, src_sk);
		}

		ss->CalculateInvRefMatrices();
		ss->CalculateExtendedBounds();
#if WITH_EDITORONLY_DATA
		ss->ConvertLegacyLODScreenSize();
#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
		ss->UpdateGenerateUpToData();
#endif
#endif

#if WITH_EDITORONLY_DATA
		base->SetPreviewMesh(ss);
#endif
		base->RecreateBoneTree(ss);

		// retarget mode
		for (int bon = 0; bon < base->GetReferenceSkeleton().GetRawBoneNum(); ++bon) {
			auto type = src_k->GetBoneTranslationRetargetingMode(bon);
			base->SetBoneTranslationRetargetingMode(bon, type);
		}

		if (i == 0) {
			vrmAssetList->HumanoidSkeletalMesh = ss;
			vrmAssetList->VrmHumanoidMetaObject->SkeletalMesh = ss;
		} else {
			vrmAssetList->UE4SkeletalMesh = ss;
			vrmAssetList->VrmMannequinMetaObject->SkeletalMesh = ss;
		}
	}

	// meta rename check
	// 0: humanoid
	// 1: ue4mannequin
	for (int i = 0; i < 2; ++i) {

		USkeleton* base = nullptr;
		USkeletalMesh* ss = nullptr;
		UNodeMappingContainer* rr = nullptr;
		UVrmMetaObject* meta = nullptr;

		if (i == 0) {
			ss = vrmAssetList->HumanoidSkeletalMesh;
			base = VRMGetSkeleton(ss);
			meta = vrmAssetList->VrmHumanoidMetaObject;
		} else {
			ss = vrmAssetList->UE4SkeletalMesh;
			base = VRMGetSkeleton(ss);
			meta = vrmAssetList->VrmMannequinMetaObject;
		}

		int boneNo = 0;
		TPair<FString, FString> prev;
		for (auto& a : meta->humanoidBoneTable) {
			if (ss==nullptr) break;
			auto ind = VRMGetRefSkeleton(ss).FindBoneIndex(*(a.Value));

			if (boneNo == 0) {
				prev = a;
			}

			if (ind == INDEX_NONE && a.Value!="") {
				// set prev bone name
				a.Value = prev.Value;
			}
			prev = a;
			++boneNo;
		}
	}


	return true;
}

VrmConvertHumanoid::VrmConvertHumanoid()
{
}

VrmConvertHumanoid::~VrmConvertHumanoid()
{
}
