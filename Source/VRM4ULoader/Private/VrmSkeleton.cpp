// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmSkeleton.h"
#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "Engine/SkeletalMesh.h"
//#include "UnStringConv.h

#include "VrmConvert.h"
#include "VrmUtil.h"
#include "VRM4ULoaderLog.h"

#include <assimp/scene.h>       // Output data structure
#include <assimp/mesh.h>       // Output data structure

static int countParent(const aiNode *node, TArray<const aiNode*> &t, int c) {
	for (auto &a : t) {
		for (uint32_t i = 0; i < a->mNumChildren; ++i) {
			if (node == a->mChildren[i]) {
				return countParent(a, t, c + 1);
			}
		}
	}
	return c;
}

static int countChild(aiNode *node, int c) {
	c += node->mNumChildren;
	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		c = countChild(node->mChildren[i], c);
	}
	return c;
}

bool findActiveBone(const aiNode *node, TArray<FString> &table) {

	if (table.Find(node->mName.C_Str()) != INDEX_NONE) {
		return true;
	}
	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		if (findActiveBone(node->mChildren[i], table)) {
			return true;
		}
	}
	return false;
}

TArray<FString> makeActiveBone(const aiScene *scene) {
	TArray <FString> boneNameTable;
	for (uint32 m = 0; m < scene->mNumMeshes; ++m) {
		const auto &aiM = *scene->mMeshes[m];

		for (uint32 b = 0; b < aiM.mNumBones; ++b) {
			const auto &aiB = *aiM.mBones[b];
			boneNameTable.AddUnique(aiB.mName.C_Str());
		}
	}
	return boneNameTable;
}

TMap<const aiNode*, const aiBone* > makeAiBoneTable(const aiScene* scene, TArray<const aiNode*> &nodeArray) {
	TMap<const aiNode*, const aiBone*> table;

	for (uint32 m = 0; m < scene->mNumMeshes; ++m) {
		const auto& aiM = *scene->mMeshes[m];

		for (uint32 b = 0; b < aiM.mNumBones; ++b) {
			const auto& aiB = aiM.mBones[b];
			for (auto a : nodeArray) {
				if (a->mName == aiB->mName) {
					table.Add(a, aiB);
					break;
				}
			}
		}
	}

	return table;
}

FMatrix convertAiMatToFMatrix(aiMatrix4x4 t) {
	FMatrix m;

	m.M[0][0] = t.a1; m.M[1][0] = t.a2; m.M[2][0] = t.a3; m.M[3][0] = -t.a4 * 100.f;
	m.M[0][1] = t.b1; m.M[1][1] = t.b2; m.M[2][1] = t.b3; m.M[3][1] = t.c4 * 100.f;//t.b4*100.f;
	m.M[0][2] = t.c1; m.M[1][2] = t.c2; m.M[2][2] = t.c3; m.M[3][2] = t.b4 * 100.f;//t.c4*100.f;
	m.M[0][3] = t.d1; m.M[1][3] = t.d2; m.M[2][3] = t.d3; m.M[3][3] = t.d4;

	if (VRMConverter::Options::Get().IsVRM10Model()) {
		m.M[0][0] = t.a1; m.M[1][0] = t.a2; m.M[2][0] = t.a3; m.M[3][0] = t.a4 * 100.f;
		m.M[0][1] = t.b1; m.M[1][1] = t.b2; m.M[2][1] = t.b3; m.M[3][1] = t.b4 * 100.f;//t.b4*100.f;
		m.M[0][2] = t.c1; m.M[1][2] = t.c2; m.M[2][2] = t.c3; m.M[3][2] = t.c4 * 100.f;//t.c4*100.f;
		m.M[0][3] = t.d1; m.M[1][3] = t.d2; m.M[2][3] = t.d3; m.M[3][3] = t.d4;

		// rot
		{
			FTransform f;
			f.SetRotation(FRotator(0, 0, 90).Quaternion());

			m = f.ToMatrixNoScale() * m * f.ToMatrixNoScale().Inverse();
		}
	}

	if (VRMConverter::Options::Get().IsPMXModel() || VRMConverter::Options::Get().IsBVHModel()) {
		m.M[3][0] *= -1.f;
		m.M[3][1] *= -1.f;
	}
	{
		m.M[3][0] *= VRMConverter::Options::Get().GetModelScale();
		m.M[3][1] *= VRMConverter::Options::Get().GetModelScale();
		m.M[3][2] *= VRMConverter::Options::Get().GetModelScale();
		//m.M[3][3] *= VRMConverter::Options::Get().GetModelScale();
	}
	return m;
}


static bool hasMeshInChild(aiNode *node) {
	if (node == nullptr) {
		return false;
	}
	if (node->mNumMeshes > 0) {
		return true;
	}
	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		bool b = hasMeshInChild(node->mChildren[i]);
		if (b) {
			return true;
		}
	}
	return false;
}


static void rr3(const aiNode *node, TArray<const aiNode*> &t, bool &bHasMesh, const bool bOnlyRootBone) {
	bHasMesh = false;
	if (node == nullptr) {
		return;
	}
	if (node->mNumMeshes > 0) {
		bHasMesh = true;
	}

	t.Push(node);
	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		bool b = false;
		rr3(node->mChildren[i], t, b, bOnlyRootBone);

		if (b) {
			bHasMesh = true;
		}
	}
	if (bHasMesh==false && bOnlyRootBone) {
		//t.Remove(node);
	}
}

static void rr(const aiNode *node, TArray<const aiNode*> &t, bool &bHasMesh, const bool bOnlyRootBone, aiScene *scene) {

	auto target = node;

	if (bOnlyRootBone) {

		int maxIndex = -1;
		int maxChild = 0;

		// find deep tree
		for (uint32 i = 0; i < node->mNumChildren; ++i) {
			int cc = countChild(node->mChildren[i], 0);

			if (cc > maxChild) {
				maxChild = cc;
				maxIndex = i;
			}

		}

		auto tableBone = makeActiveBone(scene);

		for (uint32 i = 0; i < node->mNumChildren; ++i) {
			if (i == maxIndex) {
				continue;
			}
			if (findActiveBone(node->mChildren[i], tableBone)){
				maxIndex = -1;
				break;
			}
		}

		if (maxIndex >= 0) {

			const auto trans = node->mChildren[maxIndex]->mTransformation;
			float f = 0.f;
			f += FMath::Abs(trans.a4) + FMath::Abs(trans.b4) + FMath::Abs(trans.c4);
			if (f < 1.e-8f) {
				target = node->mChildren[maxIndex];
			} else {
				// root transform must zero.
			}
		}
	}

	rr3(target, t, bHasMesh, bOnlyRootBone);
}

/////////////////////////

void VRMSkeleton::readVrmBone(aiScene* scene, int& boneOffset, FReferenceSkeleton &RefSkeleton, UVrmAssetListObject* vrmAssetList) {
	boneOffset = 0;


	FReferenceSkeletonModifier RefSkelModifier(RefSkeleton, nullptr);

	{
		FTransform t;
		t.SetIdentity();

		for (int i = 0; i < RefSkeleton.GetRawBoneNum(); ++i) {
			RefSkelModifier.UpdateRefPoseTransform(i, t);
		}
	}


	TArray<const aiNode*> nodeArray;
	{
		bool dummy = false;
		bool bSimpleRootBone = false;
		bSimpleRootBone = VRMConverter::Options::Get().IsSimpleRootBone();
		rr(scene->mRootNode, nodeArray, dummy, bSimpleRootBone, scene);
	}

	TMap<const aiNode*, const aiBone*> aiBoneTable = makeAiBoneTable(scene, nodeArray);


	{
		{

			TArray<FString> rec_low;
			TArray<FString> rec_orig;
			for (int targetBondeID = 0; targetBondeID < nodeArray.Num(); ++targetBondeID) {
				auto& a = nodeArray[targetBondeID];

				FString str = UTF8_TO_TCHAR(a->mName.C_Str());
				auto f = rec_low.Find(str.ToLower());
				if (f >= 0) {
					// same name check
					//aiString origName = a->mName;
					//str += TEXT("_renamed_vrm4u_") + FString::Printf(TEXT("%02d"), targetBondeID);
					//a->mName.Set(TCHAR_TO_ANSI(*str));

					if (rec_orig[f] == UTF8_TO_TCHAR(a->mName.C_Str())) {
						// same name node!
						aiNode* n[] = {
							scene->mRootNode->FindNode(a->mName),
							scene->mRootNode->FindNode(TCHAR_TO_ANSI(*rec_orig[f])),
						};
						int t[] = {
							countParent(n[0], nodeArray, 0),
							countParent(n[1], nodeArray, 0),
						};

						char tmp[512];
						snprintf(tmp, 512, "%s_DUP", (n[0]->mName.C_Str()));
						if ((t[0] < t[1]) || n[1]==nullptr) {
							n[0]->mName = tmp;
						} else {
							n[1]->mName = tmp;
						}

						//countParent(

						//continue;
					}

					continue;
					/*
					TMap<FString, FString> renameTable;
					{
						FString s;
						s = rec_orig[f];
						s += TEXT("_renamed_vrm4u_") + FString::Printf(TEXT("%02d"), f);
						renameTable.FindOrAdd(rec_orig[f]) = s;

						s = UTF8_TO_TCHAR(a->mName.C_Str());
						s += TEXT("_renamed_vrm4u_") + FString::Printf(TEXT("%02d"), targetBondeID);
						renameTable.FindOrAdd(UTF8_TO_TCHAR(a->mName.C_Str())) = s;
						str = s;
					}


					//add
					for (uint32_t meshID = 0; meshID < scene->mNumMeshes; ++meshID) {
						auto& aiM = *(scene->mMeshes[meshID]);

						for (uint32_t allBoneID = 0; allBoneID < aiM.mNumBones; ++allBoneID) {
							auto& aiB = *(aiM.mBones[allBoneID]);
							auto res = renameTable.Find(UTF8_TO_TCHAR(aiB.mName.C_Str()));
							if (res) {
								//if (strcmp(aiB.mName.C_Str(), origName.C_Str()) == 0) {

								char tmp[512];
								//if (bone.Num() == aiM.mNumBones) {
								//	snprintf(tmp, 512, "%s_renamed_vrm4u_%02d", origName.C_Str(), allBoneID);
								//} else {
								//snprintf(tmp, 512, "%s", a->mName.C_Str());
								snprintf(tmp, 512, "%s", TCHAR_TO_ANSI(**res));
								//}
								//FString tmp = origName.C_Str();
								//tmp += TEXT("_renamed_vrm4u") + FString::Printf(TEXT("%02d"), allBoneID);

								aiB.mName = tmp;
							}
						}
					}
					*/
				}
				/*
				if (0) {
					// ascii code
					aiString origName = a->mName;
					str = TEXT("_renamed_vrm4u_") + FString::Printf(TEXT("%02d"), targetBondeID);
					a->mName.Set(TCHAR_TO_ANSI(*str));

					//add
					for (uint32_t meshID = 0; meshID < scene->mNumMeshes; ++meshID) {
						auto &aiM = *(scene->mMeshes[meshID]);

						for (uint32_t allBoneID = 0; allBoneID < aiM.mNumBones; ++allBoneID) {
							auto &aiB = *(aiM.mBones[allBoneID]);
							if (strcmp(aiB.mName.C_Str(), origName.C_Str()) == 0) {

								char tmp[512];
								if (bone.Num() == aiM.mNumBones) {
									snprintf(tmp, 512, "_renamed_vrm4u_%02d", allBoneID);
								}else {
									snprintf(tmp, 512, "_renamed_vrm4u_%02d", allBoneID + bone.Num()*targetBondeID);
								}
								//FString tmp = origName.C_Str();
								//tmp += TEXT("_renamed_vrm4u") + FString::Printf(TEXT("%02d"), allBoneID);

								aiB.mName = tmp;
							}
						}
					}

				}
				*/

				rec_low.Add(str.ToLower());
				rec_orig.Add(str);
			}
		}


		int totalBoneCount = 0;

		TArray<FTransform> poseGlobal_bindpose;	// bone
		TArray<FTransform> poseGlobal_tpose;	// node

		TArray<FTransform> poseLocal_bindpose;	// bone
		TArray<FTransform> poseLocal_tpose;		// node

		poseGlobal_bindpose.SetNum(nodeArray.Num());
		poseGlobal_tpose.SetNum(nodeArray.Num());

		poseLocal_bindpose.SetNum(nodeArray.Num());
		poseLocal_tpose.SetNum(nodeArray.Num());

		// generate transform
		for (int nodeNo = 0; nodeNo < nodeArray.Num(); ++nodeNo) {
			auto node = nodeArray[nodeNo];
			FString nodeName = UTF8_TO_TCHAR(node->mName.C_Str());

			int32 ParentIndex = INDEX_NONE;
			if (nodeArray.Find(node->mParent, ParentIndex) == false) {
				ParentIndex = INDEX_NONE;
			}

			// t-pose (node pose)
			{
				FMatrix m = convertAiMatToFMatrix(node->mTransformation);

				FTransform localpose;
				localpose.SetFromMatrix(m);

				if (ParentIndex >= 0) {
					poseGlobal_tpose[nodeNo] = localpose * poseGlobal_tpose[ParentIndex];
				}
				else {
					poseGlobal_tpose[nodeNo] = localpose;
				}
				poseLocal_tpose[nodeNo] = localpose;
			}
		}// tpose

		for (int nodeNo = 0; nodeNo < nodeArray.Num(); ++nodeNo) {
			auto node = nodeArray[nodeNo];
			FString nodeName = UTF8_TO_TCHAR(node->mName.C_Str());

			int32 ParentIndex = INDEX_NONE;
			if (nodeArray.Find(node->mParent, ParentIndex) == false) {
				ParentIndex = INDEX_NONE;
			}

			// bind-pose
			{
				auto pBone = aiBoneTable.Find(node);

				if (pBone == nullptr) {
					// generate from tpose matrix
					if (ParentIndex == INDEX_NONE) {
						poseLocal_bindpose[nodeNo] = poseLocal_tpose[nodeNo];
						poseGlobal_bindpose[nodeNo] = poseLocal_bindpose[nodeNo];
					} else {
						poseLocal_bindpose[nodeNo] = poseLocal_tpose[nodeNo];
						poseGlobal_bindpose[nodeNo] = poseLocal_bindpose[nodeNo] * poseGlobal_bindpose[ParentIndex];
					}
				}else{
					auto bone = *pBone;

					FMatrix m = convertAiMatToFMatrix(bone->mOffsetMatrix);
					if (ParentIndex == INDEX_NONE) {
					} else {
					}


					FTransform globalpose;

					globalpose.SetFromMatrix(m.Inverse());
					poseGlobal_bindpose[nodeNo] = globalpose;

					if (ParentIndex == INDEX_NONE) {
						poseLocal_bindpose[nodeNo] = globalpose;
					}else{
						poseLocal_bindpose[nodeNo] = globalpose * poseGlobal_bindpose[ParentIndex].Inverse();
					}
				}
			}
		} // bind pose


		for (int nodeNo = 0; nodeNo < nodeArray.Num(); ++nodeNo) {
			auto node = nodeArray[nodeNo];
			FMeshBoneInfo info;
			info.Name = UTF8_TO_TCHAR(node->mName.C_Str());
#if WITH_EDITORONLY_DATA
			info.ExportName = UTF8_TO_TCHAR(node->mName.C_Str());
#endif

			int32 ParentIndex = INDEX_NONE;
			if (nodeArray.Find(node->mParent, ParentIndex) == false) {
				ParentIndex = INDEX_NONE;
			}
			info.ParentIndex = ParentIndex;

			FMatrix m = convertAiMatToFMatrix(node->mTransformation);

			FTransform pose;
			
			if (VRMConverter::Options::Get().IsVRM10Bindpose()) {
				pose = poseLocal_bindpose[nodeNo];
			}else{
				pose = poseLocal_tpose[nodeNo];
			}
			// remove local axis
			if (VRMConverter::Options::Get().IsVRM10Model()) {
				if (VRMConverter::Options::Get().IsVRM10RemoveLocalRotation()) {

					if (vrmAssetList){
						if (vrmAssetList->VrmMetaObject) {
							TArray<FString> v;
							vrmAssetList->VrmMetaObject->humanoidBoneTable.GenerateValueArray(v);
							//if (v.Find(info.Name.ToString()) != INDEX_NONE) {
								pose.SetRotation(FQuat::Identity);
								if (ParentIndex >= 0) {
									pose.SetTranslation(poseGlobal_tpose[nodeNo].GetLocation() - poseGlobal_tpose[ParentIndex].GetLocation());
								}
							//}
						}
					}

				}
			}

			// bone name check
			{
				auto baseName = info.Name.ToString();
				for (int c = 0; c < 100; ++c) {
					if (RefSkeleton.FindRawBoneIndex(info.Name) == INDEX_NONE) {
						break;
					}
					info.Name = *FString::Printf(TEXT("%s_vrm4u%02d"), *baseName, c);
				}
			}
			if (totalBoneCount > 0 && ParentIndex == INDEX_NONE) {
				// bad bone. root?
				continue;
			}
			if (VRMConverter::Options::Get().IsVRMAModel()) {
				if (vrmAssetList->VrmMetaObject) {
					for (auto& t : vrmAssetList->VrmMetaObject->humanoidBoneTable) {

						if (t.Value == info.Name.ToString()) {
							info.Name = *t.Key;
							break;
						}
					}
				}
			}

			RefSkelModifier.Add(info, pose);
			totalBoneCount++;

			if (vrmAssetList) {
				vrmAssetList->Pose_bind.Add(UTF8_TO_TCHAR(node->mName.C_Str()), poseGlobal_bindpose[nodeNo]);
				vrmAssetList->Pose_tpose.Add(UTF8_TO_TCHAR(node->mName.C_Str()), poseGlobal_tpose[nodeNo]);
			}

			if (totalBoneCount == 1) {
				if (VRMConverter::Options::Get().IsDebugOneBone()) {
					// root only
					break;
				}
			}
		}
	}
}


void VRMSkeleton::addIKBone(UVrmAssetListObject* vrmAssetList, USkeletalMesh *sk) {

	if (VRMConverter::Options::Get().IsGenerateIKBone() == false) {
		return;
	}

	// refskeleton
	auto& BonePose = VRMGetRefSkeleton(sk).GetRefBonePose();
	FReferenceSkeletonModifier RefSkelModifier(VRMGetRefSkeleton(sk), VRMGetSkeleton(sk));

	{
		FString BoneCheckList[] = {
			TEXT("ik_foot_l"),
			TEXT("ik_foot_r"),
			TEXT("ik_hand_l"),
			TEXT("ik_hand_r"),
			TEXT("ik_hand_root"),
			TEXT("ik_foot_root"),
			TEXT("ik_hand_gun"),
		};
		for (auto& s : BoneCheckList) {
			if (RefSkelModifier.FindBoneIndex(*s) != INDEX_NONE) {
				return;
			}
		}
	}

	FTransform trans;
	FMeshBoneInfo info;
	info.ParentIndex = 0;

	info.Name = TEXT("ik_foot_root");
	RefSkelModifier.Add(info, trans);

	info.Name = TEXT("ik_hand_root");
	RefSkelModifier.Add(info, trans);

	// rebuild tmp
	VRMGetRefSkeleton(sk).RebuildRefSkeleton(VRMGetSkeleton(sk), true);
	{
		info.ParentIndex = RefSkelModifier.FindBoneIndex(TEXT("ik_foot_root"));
		info.Name = ("ik_foot_l");
		RefSkelModifier.Add(info, trans);
		info.Name = ("ik_foot_r");
		RefSkelModifier.Add(info, trans);

		info.ParentIndex = RefSkelModifier.FindBoneIndex(TEXT("ik_hand_root"));
		info.Name = ("ik_hand_gun");
		RefSkelModifier.Add(info, trans);
	}
	// rebuild tmp
	VRMGetRefSkeleton(sk).RebuildRefSkeleton(VRMGetSkeleton(sk), true);
	{

		info.ParentIndex = RefSkelModifier.FindBoneIndex(TEXT("ik_hand_gun"));
		info.Name = ("ik_hand_l");
		RefSkelModifier.Add(info, trans);
		info.Name = ("ik_hand_r");
		RefSkelModifier.Add(info, trans);

	}
	// rebuild tmp
	VRMGetRefSkeleton(sk).RebuildRefSkeleton(VRMGetSkeleton(sk), true);

	{
		FString orgBone[4];
		FString newBone[4] = {
			TEXT("ik_foot_l"),
			TEXT("ik_foot_r"),
			TEXT("ik_hand_l"),
			TEXT("ik_hand_r"),
		};
		FString parentBone[4] = {
			TEXT("ik_foot_root"),
			TEXT("ik_foot_root"),
			TEXT("ik_hand_gun"),
			TEXT("ik_hand_gun"),
		};
		{
			FString ue4bone[4] = {
				TEXT("foot_l"),
				TEXT("foot_r"),
				TEXT("hand_l"),
				TEXT("hand_r"),
			};
			for (int i = 0; i < 4; ++i) {
				FString humanoid;
				for (auto& t : VRMUtil::table_ue4_vrm) {
					if (t.BoneUE4 == ue4bone[i]) {
						humanoid = t.BoneVRM;
						break;
					}
				}
				for (auto& t : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
					if (t.Key == humanoid) {
						orgBone[i] = t.Value;
						break;
					}
				}
			}
		}

		// gun only
		FTransform gun_trans;
		{
			{
				const int i = 3;

				int32 ind = 0;
				ind = VRMGetRefSkeleton(sk).FindBoneIndex(*orgBone[i]);
				if (ind >= 0) {
					FTransform a;
					while (ind >= 0) {
						a *= BonePose[ind];
						ind = VRMGetRefSkeleton(sk).GetParentIndex(ind);
					}

					gun_trans = a;
					RefSkelModifier.UpdateRefPoseTransform(RefSkelModifier.FindBoneIndex(TEXT("ik_hand_gun")), a);
				}
			}
			// rebuild tmp
			VRMGetRefSkeleton(sk).RebuildRefSkeleton(VRMGetSkeleton(sk), true);
		}
		for (int i = 0; i < 4; ++i) {
			int32 ind = 0;
			ind = RefSkelModifier.FindBoneIndex(*orgBone[i]);
			if (ind >= 0) {
				bool first = true;
				FTransform a;
				while (ind >= 0) {
					a *= BonePose[ind];
					ind = VRMGetRefSkeleton(sk).GetParentIndex(ind);
				}
				if (i >= 2) {
					a = gun_trans.Inverse() * a;
				}
				RefSkelModifier.UpdateRefPoseTransform(RefSkelModifier.FindBoneIndex(*newBone[i]), a);
			}
		}
	}
}

