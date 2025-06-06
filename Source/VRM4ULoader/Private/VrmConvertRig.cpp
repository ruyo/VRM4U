// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertRig.h"
#include "VrmConvert.h"
#include "VrmUtil.h"

#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "LoaderBPFunctionLibrary.h"
#include "VrmBPFunctionLibrary.h"

#include "Engine/SkeletalMesh.h"
#include "RenderingThread.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"
#include "Animation/NodeMappingContainer.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UnrealType.h"
#if UE_VERSION_OLDER_THAN(5,4,0)
#include "Animation/Rig.h"
#endif

#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#if	UE_VERSION_OLDER_THAN(5,5,0)
#else
#include "PhysicsEngine/SkeletalBodySetup.h"
#endif


#include "VrmRigHeader.h"


#if WITH_EDITOR
#include "IPersonaToolkit.h"
#include "PersonaModule.h"
#include "Modules/ModuleManager.h"
#include "Animation/DebugSkelMeshComponent.h"
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>

//#include "Engine/.h"

namespace {

	static int GetChildBoneLocal(const FReferenceSkeleton &skeleton, const int32 ParentBoneIndex, TArray<int32> & Children) {
		Children.Reset();
		//auto &r = skeleton->GetReferenceSkeleton();
		auto &r = skeleton;

		const int32 NumBones = r.GetRawBoneNum();
		for (int32 ChildIndex = ParentBoneIndex + 1; ChildIndex < NumBones; ChildIndex++)
		{
			if (ParentBoneIndex == r.GetParentIndex(ChildIndex))
			{
				Children.Add(ChildIndex);
			}
		}
		return Children.Num();
	}

	static bool isSameOrChild(const FReferenceSkeleton &skeleton, const int32 TargetBoneIndex, const int32 SameOrChildBoneIndex) {
		auto &r = skeleton;

		if (TargetBoneIndex < 0 || SameOrChildBoneIndex < 0) {
			return false;
		}

		int32 c = SameOrChildBoneIndex;
		for (int i = 0; i < skeleton.GetRawBoneNum(); ++i) {

			if (c < 0) break;

			if (c == TargetBoneIndex) {
				return true;
			}

			c = skeleton.GetParentIndex(c);
		}
		return false;
	}
}

bool VRMConverter::ConvertRig(UVrmAssetListObject *vrmAssetList) {

	if (VRMConverter::Options::Get().IsDebugOneBone()) {
		return true;
	}

	if (vrmAssetList->SkeletalMesh == nullptr) {
		return false;
	}

	bool bPlay = false;
	{
		bool b1, b2, b3;
		b1 = b2 = b3 = false;
		UVrmBPFunctionLibrary::VRMGetPlayMode(b1, b2, b3);
		bPlay = b1;
	}
	if (bPlay) {
	}

	// override bone table
	if (VRMConverter::Options::Get().IsVRMAModel()) {
		if (vrmAssetList->VrmMetaObject) {
			for (auto& t : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
				t.Value = t.Key;
			}
		}
	}

	auto* k = VRMGetSkeleton(vrmAssetList->SkeletalMesh);

#if	UE_VERSION_OLDER_THAN(4,20,0)
#elif UE_VERSION_OLDER_THAN(5,4,0)

#if WITH_EDITOR
	UNodeMappingContainer* mc = nullptr;

#if	UE_VERSION_OLDER_THAN(5,0,0)
	{
		FString name = FString(TEXT("RIG_")) + vrmAssetList->BaseFileName;
		mc = VRM4U_NewObject<UNodeMappingContainer>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);
	}
	VRMGetNodeMappingData(vrmAssetList->SkeletalMesh).Add(mc);
	URig *EngineHumanoidRig = LoadObject<URig>(nullptr, TEXT("/Engine/EngineMeshes/Humanoid.Humanoid"), nullptr, LOAD_None, nullptr);
	mc->SetSourceAsset(EngineHumanoidRig);

	VRMGetSkeleton(vrmAssetList->SkeletalMesh)->SetRigConfig(EngineHumanoidRig);

	mc->SetTargetAsset(vrmAssetList->SkeletalMesh);
	mc->AddDefaultMapping();
#endif

	{
		const VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(aiData->mVRMMeta);

		auto func = [&](const FString &a, const FString b) {
			if (mc == nullptr) return;
			mc->AddMapping(*a, *b);
			VRMGetSkeleton(vrmAssetList->SkeletalMesh)->SetRigBoneMapping(*a, *b);
		};
		auto func2 = [&](const FString &a, FName b) {
			func(a, b.ToString());
		};

		if (meta) {
			for (auto &t : VRMUtil::table_ue4_vrm) {
				FString target = t.BoneVRM;
				const FString &ue4 = t.BoneUE4;

				if (ue4.Compare(TEXT("Root"), ESearchCase::IgnoreCase) == 0) {
					auto &a = VRMGetSkeleton(vrmAssetList->SkeletalMesh)->GetReferenceSkeleton().GetRefBoneInfo();
					target = a[0].Name.ToString();
				}

				if (target.Len() == 0) {
					continue;
				}


				for (auto b : meta->humanoidBone) {

					if (target.Compare(b.humanBoneName.C_Str()) != 0) {
						continue;
					}
					target = b.nodeName.C_Str();
					break;
				}

				func(ue4, target);
			}
			{
				const TArray<FString> cc = {
					TEXT("Root"),
					TEXT("Pelvis"),
					TEXT("spine_01"),
					TEXT("spine_02"),
					TEXT("spine_03"),
					TEXT("neck_01"),
				};

				// find bone from child bone
				for (int i = cc.Num() - 2; i > 0; --i) {
					if (mc == nullptr) continue;

					const auto &m = mc->GetNodeMappingTable();

					{
						const auto p0 = m.Find(*cc[i]);
						if (p0) {
							// map exist
							continue;
						}
					}
					const auto p = m.Find(*cc[i + 1]);
					if (p == nullptr) {
						// child none
						continue;
					}

					const FName *parentRoot = nullptr;
					for (int toParent = i - 1; toParent > 0; --toParent) {
						parentRoot = m.Find(*cc[toParent]);
						if (parentRoot) {
							break;
						}
					}
					if (parentRoot == nullptr) {
						continue;
					}

					// find (child) p -> (parent)parentRoot
					FString newTarget = parentRoot->ToString();
					{

						const int32 index = k->GetReferenceSkeleton().FindBoneIndex(*p);
						const int32 indexParent = k->GetReferenceSkeleton().GetParentIndex(index);
						const int32 indexRoot = k->GetReferenceSkeleton().FindBoneIndex(*parentRoot);

						if (isSameOrChild(k->GetReferenceSkeleton(), indexRoot, indexParent)) {
							newTarget = k->GetReferenceSkeleton().GetBoneName(indexParent).ToString();
						}
					}
					func(cc[i], newTarget);
				}

				// set null -> parent bone
				for (int i = 1; i < cc.Num(); ++i) {
					if (mc == nullptr) continue;
					const auto &m = mc->GetNodeMappingTable();

					{
						const auto p0 = m.Find(*cc[i]);
						if (p0) {
							// map exist
							continue;
						}
					}
					const auto pp = m.Find(*cc[i-1]);
					if (pp == nullptr) {
						// parent none
						continue;
					}

					// map=nullptr, parent=exist
					FString newTarget = pp->ToString();

					{
						int32 index = k->GetReferenceSkeleton().FindBoneIndex(*pp);
						TArray<int32> child;
						GetChildBoneLocal(k->GetReferenceSkeleton(), index, child);
						if (child.Num() == 1) {
							// use one child
							// need neck check...
							//newTarget = k->GetReferenceSkeleton().GetBoneName(child[0]).ToString();
						}
					}

					func(cc[i], newTarget);
				}
			}
		} else {
			// BVH auto mapping
			
			const auto &rSk = k->GetReferenceSkeleton(); //EngineHumanoidRig->GetSourceReferenceSkeleton();
			
			TArray<FString> rBoneList;
			{
				for (auto &a : rSk.GetRawRefBoneInfo()) {
					rBoneList.Add(a.Name.ToString());
				}
			}
			TArray<FString> existTable;
			int boneIndex = -1;
			for (auto &b : rBoneList) {
				++boneIndex;

				if (b.Find(TEXT("neck")) >= 0) {
					const FString t = TEXT("neck_01");
					if (existTable.Find(t) >= 0) {
						continue;
					}
					existTable.Add(t);
					func(t, b);


					const TArray<FString> cc = {
						TEXT("spine_03"),
						TEXT("spine_02"),
						TEXT("spine_01"),
						TEXT("Pelvis"),
						TEXT("Root"),
					};

					TArray<FName> targetBone;
					targetBone.SetNum(5);

					// map
					const int neckBoneIndex = boneIndex;
					{
						int p = neckBoneIndex;

						for (int i = 0; i < 5; ++i) {
							int p2 = rSk.GetParentIndex(p);
							if (p2 >= 0) {
								p = p2;
							}
							targetBone[i] = rSk.GetBoneName(p);
						}
					}

					// upper chest skip
					for (int i = 0; i < 5; ++i) {
						if (targetBone[2] == targetBone[3]) {
							targetBone[2] = targetBone[1];
							targetBone[1] = targetBone[0];
						}
					}

					// root
					targetBone[cc.Num() - 1] = rSk.GetBoneName(0);

					// pelvis, spine
					{
						int p = neckBoneIndex;

						for (int i = 0; i < 100; ++i) {
							p = rSk.GetParentIndex(p);
							if (p <= 0) break;

							int p2 = rSk.GetParentIndex(p);
							if (p2 <= 0) break;

							// pelvis
							targetBone[cc.Num() - 2] = rSk.GetBoneName(p2);

							// spine
							targetBone[cc.Num() - 3] = rSk.GetBoneName(p);
						}
					}

					//regist
					for (int i = 0; i < 5; ++i) {
						func2(cc[i], targetBone[i]);
					}
				}
				if (b.Find(TEXT("head")) >= 0) {
					const FString t = TEXT("head");
					if (existTable.Find(t) >= 0) {
						continue;
					}
					existTable.Add(t);
					func(t, b);
				}

				if (b.Find(TEXT("hand")) >= 0) {
					if (b.Find("r") >= 0) {
						const FString t = TEXT("Hand_R");
						if (existTable.Find(t) >= 0){
							continue;
						}
						existTable.Add(t);

						func(t, b);

						int p = rSk.GetParentIndex(boneIndex);
						if (p >= 0) {
							func2(TEXT("lowerarm_r"), rSk.GetBoneName(p));
						}

						p = rSk.GetParentIndex(p);
						if (p >= 0) {
							func2(TEXT("UpperArm_R"), rSk.GetBoneName(p));

							p = rSk.GetParentIndex(p);
							if (p >= 0) {
								func2(TEXT("clavicle_r"), rSk.GetBoneName(p));
							}
						}

						{
							// finger right
							TArray<int32> childBone;
							GetChildBoneLocal(rSk, boneIndex, childBone);
							FString tt[6][2] = {
								{"thumb",	"thumb_01_r"},
								{"index",	"index_01_r"},
								{"middle",	"middle_01_r"},
								{"ring",		"ring_01_r"},
								{"little",	"pinky_01_r"},	// little
								{"pinky",	"pinky_01_r"},	// little
							};
							for (auto& c : childBone) {
								auto name = rSk.GetBoneName(c);
								for (int i = 0; i < 6; ++i) {
									{
										int tmp = name.ToString().Find(tt[i][0]);
										if (tmp < 0) continue;
									}
									int ind = VRMUtil::ue4_humanoid_bone_list.Find(tt[i][1]);
									if (ind < 0) continue;

									func2(VRMUtil::ue4_humanoid_bone_list[ind], rSk.GetBoneName(c));

									TArray<int32> tmp = { c };

									GetChildBoneLocal(rSk, tmp[0], tmp);
									if (tmp.Num() <= 0) continue;
									func2(VRMUtil::ue4_humanoid_bone_list[ind + 1], rSk.GetBoneName(tmp[0]));

									GetChildBoneLocal(rSk, tmp[0], tmp);
									if (tmp.Num() <= 0) continue;
									func2(VRMUtil::ue4_humanoid_bone_list[ind + 2], rSk.GetBoneName(tmp[0]));
								}
							}
						}
					}
					if (b.Find("l") >= 0) {
						const FString t = TEXT("Hand_L");
						if (existTable.Find(t) >= 0) {
							continue;
						}
						existTable.Add(t);

						func(t, b);

						int p = rSk.GetParentIndex(boneIndex);
						if (p >= 0) {
							func2(TEXT("lowerarm_l"), rSk.GetBoneName(p));

							p = rSk.GetParentIndex(p);
							if (p >= 0) {
								func2(TEXT("UpperArm_L"), rSk.GetBoneName(p));

								p = rSk.GetParentIndex(p);
								if (p >= 0) {
									func2(TEXT("clavicle_l"), rSk.GetBoneName(p));
								}
							}
						}

						{
							// finger left
							TArray<int32> childBone;
							GetChildBoneLocal(rSk, boneIndex, childBone);
							FString tt[6][2] = {
								{"thumb",	"thumb_01_l"},
								{"index",	"index_01_l"},
								{"middle",	"middle_01_l"},
								{"ring",		"ring_01_l"},
								{"little",	"pinky_01_l"},	// little
								{"pinky",	"pinky_01_l"},	// little
							};
							for (auto& c : childBone) {
								auto name = rSk.GetBoneName(c);
								for (int i = 0; i < 6; ++i) {
									{
										int tmp = name.ToString().Find(tt[i][0]);
										if (tmp < 0) continue;
									}
									int ind = VRMUtil::ue4_humanoid_bone_list.Find(tt[i][1]);
									if (ind < 0) continue;

									func2(VRMUtil::ue4_humanoid_bone_list[ind], rSk.GetBoneName(c));

									TArray<int32> tmp = { c };

									GetChildBoneLocal(rSk, tmp[0], tmp);
									if (tmp.Num() <= 0) continue;
									func2(VRMUtil::ue4_humanoid_bone_list[ind + 1], rSk.GetBoneName(tmp[0]));

									GetChildBoneLocal(rSk, tmp[0], tmp);
									if (tmp.Num() <= 0) continue;
									func2(VRMUtil::ue4_humanoid_bone_list[ind + 2], rSk.GetBoneName(tmp[0]));
								}
							}
						}
					}
				}
				if (b.Find(TEXT("foot")) >= 0) {
					if (b.Find("r") >= 0) {
						const FString t = TEXT("Foot_R");
						if (existTable.Find(t) >= 0) {
							continue;
						}
						existTable.Add(t);

						func(t, b);

						{
							TArray<int32> c;
							GetChildBoneLocal(rSk, boneIndex, c);
							if (c.Num()) {
								func2(TEXT("ball_r"), rSk.GetBoneName(c[0]));
							}
						}

						int p = rSk.GetParentIndex(boneIndex);
						if (p >= 0) {
							func2(TEXT("calf_r"), rSk.GetBoneName(p));

							p = rSk.GetParentIndex(p);
							if (p >= 0) {
								func2(TEXT("Thigh_R"), rSk.GetBoneName(p));
							}
						}
					}
					if (b.Find("l") >= 0) {
						const FString t = TEXT("Foot_L");
						if (existTable.Find(t) >= 0) {
							continue;
						}
						existTable.Add(t);

						func(t, b);

						{
							TArray<int32> c;
							GetChildBoneLocal(rSk, boneIndex, c);
							if (c.Num()) {
								func2(TEXT("ball_l"), rSk.GetBoneName(c[0]));
							}
						}

						int p = rSk.GetParentIndex(boneIndex);
						if (p >= 0) {
							func2(TEXT("calf_l"), rSk.GetBoneName(p));

							p = rSk.GetParentIndex(p);
							if (p >= 0) {
								func2(TEXT("Thigh_L"), rSk.GetBoneName(p));
							}
						}
					}
				}

				{
					// pmx bone map
					for (const auto &t : VRMUtil::table_ue4_pmx) {

						FString pmxBone;
						VRMUtil::GetReplacedPMXBone(pmxBone, t.BoneVRM);

						FString targetList[2] = {
							pmxBone,
							t.BoneVRM,
						};

						bool finish = false;
						for (int i = 0; i < 2; ++i) {
							FString target = targetList[i];// t.BoneVRM;
							const FString& ue4 = t.BoneUE4;
							auto ind = k->GetReferenceSkeleton().FindBoneIndex(*target);
							if (ind != INDEX_NONE) {
								func(ue4, target);

								for (const auto& v : VRMUtil::table_ue4_vrm) {
									if (v.BoneUE4 == t.BoneUE4) {
										if (v.BoneVRM.IsEmpty()) {
											continue;
										}
										finish = true;
										break;
									}
								}
							}
							if (finish) break;
						}// 2 loop
					}
				}// pmx map
			}
		}// map end
	}

	//mc->AddMapping
	if (mc) {
		mc->PostEditChange();
		vrmAssetList->HumanoidRig = mc;
	}
#endif // editor
#endif //420

	if (VRMConverter::Options::Get().IsVRMAModel()) {
		// for expression curve
		for (int i = 0; i < k->GetReferenceSkeleton().GetRawBoneNum(); ++i) {
			k->SetBoneTranslationRetargetingMode(i, EBoneTranslationRetargetingMode::Animation);
		}

		if (vrmAssetList && vrmAssetList->VrmMetaObject) {
			for (auto& t : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
				auto i = k->GetReferenceSkeleton().FindBoneIndex(*t.Value);
				if (i != INDEX_NONE) {
					if (t.Key.Compare(TEXT("hips"), ESearchCase::IgnoreCase) == 0) {
					} else {
						k->SetBoneTranslationRetargetingMode(i, EBoneTranslationRetargetingMode::Skeleton);
					}
				}
			}
		}
	}else{

		FString PelvisBoneName;
		if (vrmAssetList && vrmAssetList->VrmMetaObject) {
			for (auto& t : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
				if (t.Key.Compare(TEXT("hips"), ESearchCase::IgnoreCase) == 0) {
					PelvisBoneName = t.Value;
					break;
				}
			}
		}

		int bone = -1;
		for (int i = 0; i < k->GetReferenceSkeleton().GetRawBoneNum(); ++i) {
			//const int32 BoneIndex = k->GetReferenceSkeleton().FindBoneIndex(InBoneName);
			k->SetBoneTranslationRetargetingMode(i, EBoneTranslationRetargetingMode::Skeleton);
			//FAssetNotifications::SkeletonNeedsToBeSaved(k);
			if (k->GetReferenceSkeleton().GetBoneName(i).Compare(*PelvisBoneName) == 0) {
				bone = i;
			}
		}

		bool first = true;
		while (bone >= 0) {
			if (first) {
				k->SetBoneTranslationRetargetingMode(bone, EBoneTranslationRetargetingMode::AnimationScaled);
			}
			else {
				k->SetBoneTranslationRetargetingMode(bone, EBoneTranslationRetargetingMode::Animation);
			}
			first = false;

			bone = k->GetReferenceSkeleton().GetParentIndex(bone);
		}
		{
			FName n[] = {
				TEXT("ik_foot_root"),
				TEXT("ik_foot_l"),
				TEXT("ik_foot_r"),
				TEXT("ik_hand_root"),
				TEXT("ik_hand_gun"),
				TEXT("ik_hand_l"),
				TEXT("ik_hand_r"),
			};

			for (auto& s : n) {
				int32 ind = k->GetReferenceSkeleton().FindBoneIndex(s);
				if (ind < 0) continue;

				k->SetBoneTranslationRetargetingMode(ind, EBoneTranslationRetargetingMode::Animation, false);
			}
		}
		if (VRMConverter::Options::Get().IsPMXModel()) {
			// center to animscale
			int32 ind = k->GetReferenceSkeleton().FindBoneIndex(*(VRMUtil::table_ue4_pmx[1].BoneVRM));
			if (ind >= 0) {
				k->SetBoneTranslationRetargetingMode(ind, EBoneTranslationRetargetingMode::AnimationScaled, false);
			}
		}
		if (VRMConverter::Options::Get().IsBVHModel()) {
			// 0 == root by vrm4u dummy
			// 1 == root by model
			k->SetBoneTranslationRetargetingMode(0, EBoneTranslationRetargetingMode::Animation, false);
			k->SetBoneTranslationRetargetingMode(1, EBoneTranslationRetargetingMode::Animation, false);
		}
		if (VRMConverter::Options::Get().IsVRMModel() == false) {
			k->SetBoneTranslationRetargetingMode(0, EBoneTranslationRetargetingMode::Animation, false);
		}

	}

	// dummy Collision
	if (vrmAssetList) {
		const VRM::VRMMetadata* meta = reinterpret_cast<VRM::VRMMetadata*>(aiData->mVRMMeta);
		USkeletalMesh* sk = vrmAssetList->SkeletalMesh;
		UPhysicsAsset* pa = VRMGetPhysicsAsset(sk);
		const FString dummy_target[] = {
			TEXT("hips"),
			TEXT("head"),

			TEXT("rightHand"),
			TEXT("leftHand"),
			TEXT("leftMiddleDistal"),
			TEXT("rightMiddleDistal"),


			TEXT("rightFoot"),
			TEXT("leftFoot"),

			TEXT("leftToes"),
			TEXT("rightToes"),

			TEXT("rightLowerArm"),
			TEXT("leftLowerArm"),

			TEXT("rightLowerLeg"),
			TEXT("leftLowerLeg"),
		};
		if (meta && pa) {
			for (const auto& a : meta->humanoidBone) {

				{
					bool bFound = false;
					for (auto& d : dummy_target) {
						if (d.Compare(a.humanBoneName.C_Str()) == 0) {
							bFound = true;
						}
					}
					if (bFound == false) {
						continue;
					}
				}

				{
					bool b = false;
					for (const auto& bs : pa->SkeletalBodySetups) {
						FString s = bs->BoneName.ToString();
						if (s.Compare(a.nodeName.C_Str(), ESearchCase::IgnoreCase) == 0) {
							b = true;
							break;
						}
					}
					if (b) {
						continue;
					}
				}

				const int targetBone = VRMGetRefSkeleton(sk).FindRawBoneIndex(a.nodeName.C_Str());
				if (targetBone == INDEX_NONE) {
					break;
				}

				USkeletalBodySetup* bs = nullptr;
				int BodyIndex1 = -1;

				bs = NewObject<USkeletalBodySetup>(pa, *(FString(TEXT("dummy_for_clip")) + a.humanBoneName.C_Str()), RF_Transactional);

				FKAggregateGeom agg;
				FKSphereElem SphereElem;
				SphereElem.Center = FVector(0);
				SphereElem.Radius = 1.f;// center.Size();// 1.f;
				agg.SphereElems.Add(SphereElem);
				SphereElem.SetName(TEXT("dummy_for_clip"));

				bs->Modify();
				bs->BoneName = a.nodeName.C_Str();
				bs->AddCollisionFrom(agg);
				bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
				// newly created bodies default to simulating
				bs->PhysicsType = PhysType_Kinematic;	// fix
														//bs->get
				bs->CollisionReponse = EBodyCollisionResponse::BodyCollision_Disabled;
				bs->DefaultInstance.InertiaTensorScale.Set(2, 2, 2);
				bs->DefaultInstance.LinearDamping = 0.f;
				bs->DefaultInstance.AngularDamping = 0.f;

				bs->InvalidatePhysicsData();
				bs->CreatePhysicsMeshes();
				BodyIndex1 = pa->SkeletalBodySetups.Add(bs);

				//break;
			}

			pa->UpdateBoundsBodiesArray();
			pa->UpdateBodySetupIndexMap();
			RefreshSkelMeshOnPhysicsAssetChange(sk);
#if WITH_EDITOR
			pa->RefreshPhysicsAssetChange();
#endif

#if WITH_EDITOR
			if (VRMConverter::IsImportMode()) {
				pa->PostEditChange();
			}
#endif
		}
	}// collistion

	return true;
}

