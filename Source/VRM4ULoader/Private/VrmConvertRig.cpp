// VRM4U Copyright (c) 2021-2023 Haruyoshi Yamamoto. This software is released under the MIT License.

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
#include "Animation/Rig.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UnrealType.h"
#include "UObject/UnrealTypePrivate.h"

#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#if	UE_VERSION_OLDER_THAN(5,0,0)

#elif UE_VERSION_OLDER_THAN(5,2,0)

#include "IKRigDefinition.h"
#include "IKRigSolver.h"
#include "Retargeter/IKRetargeter.h"
#if WITH_EDITOR
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Solvers/IKRig_PBIKSolver.h"
#endif

#elif UE_VERSION_OLDER_THAN(5,3,0)

#include "IKRigDefinition.h"
#include "IKRigSolver.h"
#include "Retargeter/IKRetargeter.h"
#if WITH_EDITOR
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Solvers/IKRig_FBIKSolver.h"
#endif

#else
#include "Rig/IKRigDefinition.h"
#include "Rig/Solvers/IKRigSolver.h"
#include "Retargeter/IKRetargeter.h"
#if WITH_EDITOR
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Rig/Solvers/IKRig_FBIKSolver.h"
#endif

#endif



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

#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else

	static void LocalSolverSetup(UIKRigController* rigcon, UVrmAssetListObject *assetList) {
		if (rigcon == nullptr) return;

		int sol_index = 0;
#if	UE_VERSION_OLDER_THAN(5,2,0)
		auto* sol = rigcon->GetSolver(sol_index);
#else
		auto* sol = rigcon->GetSolverAtIndex(sol_index);
#endif

		if (sol == nullptr) {
#if	UE_VERSION_OLDER_THAN(5,2,0)
			sol_index = rigcon->AddSolver(UIKRigPBIKSolver::StaticClass());
			sol = rigcon->GetSolver(sol_index);
#else
			sol_index = rigcon->AddSolver(UIKRigFBIKSolver::StaticClass());
			sol = rigcon->GetSolverAtIndex(sol_index);
#endif
		}
		if (sol == nullptr) return;

		sol->SetEnabled(false);

		// hip
		for (auto& modelName : assetList->VrmMetaObject->humanoidBoneTable) {
			if (modelName.Key == "" || modelName.Value == "") {
				continue;
			}
			if (modelName.Key == TEXT("hips")) {
				sol->SetRootBone(*modelName.Value);
			}
		}

		{
			TArray<FString> a = {
				TEXT("leftHand"),
				TEXT("rightHand"),
				TEXT("leftToes"),
				TEXT("rightToes"),
			};
			for (int i = 0; i < a.Num(); ++i) {
				for (auto& t : assetList->VrmMetaObject->humanoidBoneTable) {
					if (t.Key.ToLower() == a[i].ToLower()) {

#if	UE_VERSION_OLDER_THAN(5,2,0)
						auto* goal = rigcon->AddNewGoal(*(a[i] + TEXT("_Goal")), *t.Value);
						if (goal) {
							rigcon->ConnectGoalToSolver(*goal, sol_index);

							// arm chain
							if (i == 0 || i == 1) {
								UIKRig_FBIKEffector* e = Cast<UIKRig_FBIKEffector>(sol->GetGoalSettings(goal->GoalName));
								if (e) {
									e->PullChainAlpha = 0.f;
								}
							}

							const auto &chain = rigcon->GetRetargetChains();
							for (auto& c : chain) {
								if (c.EndBone.BoneName == *t.Value) {
									rigcon->SetRetargetChainGoal(c.ChainName, goal->GoalName);
								}
							}
						}
#else
						auto goal = rigcon->AddNewGoal(*(a[i] + TEXT("_Goal")), *t.Value);
						if (goal != NAME_None) {
							rigcon->ConnectGoalToSolver(goal, sol_index);

							// arm chain
							if (i == 0 || i == 1) {
								UIKRig_FBIKEffector* e = Cast<UIKRig_FBIKEffector>(sol->GetGoalSettings(goal));
								if (e) {
									e->PullChainAlpha = 0.f;
								}
							}

							const auto& chain = rigcon->GetRetargetChains();
							for (auto& c : chain) {
								if (c.EndBone.BoneName == *t.Value) {
									rigcon->SetRetargetChainGoal(c.ChainName, goal);
								}
							}
						}
#endif
					}
				}
			}
		}
		{
			TArray<FString> a = {
				TEXT("leftShoulder"),
				TEXT("rightShoulder"),
				TEXT("leftLowerArm"),
				TEXT("rightLowerArm"),

				TEXT("leftUpperLeg"),
				TEXT("rightUpperLeg"),
				TEXT("leftLowerLeg"),
				TEXT("rightLowerLeg"),

				TEXT("hips"),
				TEXT("spine"),
				TEXT("chest"),
				TEXT("upperChest"),
			};
			for (int i = 0; i < a.Num(); ++i) {
				for (auto& t : assetList->VrmMetaObject->humanoidBoneTable) {
					if (t.Key.ToLower() == a[i].ToLower()) {

						if (t.Value == "") continue;

#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
						typedef UIKRig_FBIKBoneSettings UIKRig_PBIKBoneSettings;
#endif

						// shoulder
						if (i == 0 || i == 1) {
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							s->RotationStiffness = 1.f;
						}

						// arm
						if (i == 2 || i == 3) {
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							s->bUsePreferredAngles = true;
							if (i == 2) {
								s->PreferredAngles.Set(0, 0, 90);
							} else {
								s->PreferredAngles.Set(0, 0, -90);
							}
						}

						// upperleg
						if (i == 4 || i == 5 || i == 6 || i == 7) {
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							s->bUsePreferredAngles = true;
							if (i == 4 || i == 5) {
								s->PreferredAngles.Set(-180, 0, 0);
							} else {
								s->PreferredAngles.Set(180, 0, 0);
								s->Y = EPBIKLimitType::Locked;
								s->Z = EPBIKLimitType::Locked;
							}
						}

						// shoulder
						if (i >= 8) {
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							if (i == 8) {
								s->RotationStiffness = 1.f;
							} else {
								s->RotationStiffness = 0.9f;
							}
						}

					}
				}
			}
		}
	}
#endif
#endif

}

#define VRM4U_USE_EDITOR_RIG WITH_EDITOR

#if WITH_EDITOR
#else
class UIKRetargeterController {
public:
	static void setSourceRig(UIKRetargeter* retargeter, UIKRigDefinition* rig){
		retargeter->SourceIKRigAsset = rig;
	}
};
#endif


class SimpleRetargeterController {
	//friend class UIKRetargeter;

	UIKRetargeter *Retargeter = nullptr;
public:
	SimpleRetargeterController(UIKRetargeter* rig) {
		Retargeter = rig;
	}

#if	UE_VERSION_OLDER_THAN(5,2,0)
	void SetIKRig(UIKRigDefinition* IKRig) const {
#if VRM4U_USE_EDITOR_RIG || WITH_EDITOR
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		c->SetIKRig(IKRig);
#else
#endif
	}
#else
	void SetIKRig(const ERetargetSourceOrTarget SourceOrTarget, UIKRigDefinition* IKRig) const {
#if VRM4U_USE_EDITOR_RIG || WITH_EDITOR
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		c->SetIKRig(SourceOrTarget, IKRig);
#else
		UIKRetargeterController::setSourceRig(Retargeter, IKRig);
		/*
		//FScopeLock Lock(&ControllerLock);

		if (SourceOrTarget == ERetargetSourceOrTarget::Source)
		{
			UIKRetargeterController::setSourceRig(setSourceRig, IKRig);
			//Retargeter->SourceIKRigAsset = IKRig;
			//Retargeter->SourcePreviewMesh = IKRig ? IKRig->GetPreviewMesh() : nullptr;
		}
		else
		{
			//Retargeter->TargetIKRigAsset = IKRig;
			//Retargeter->TargetPreviewMesh = IKRig ? IKRig->GetPreviewMesh() : nullptr;
		}

		// re-ask to fix root height for this mesh
		if (IKRig)
		{
			SetAskedToFixRootHeightForMesh(GetPreviewMesh(SourceOrTarget), false);
		}

		CleanChainMapping();

		constexpr bool bForceRemap = false;
		AutoMapChains(EAutoMapChainType::Fuzzy, bForceRemap);

		// update any editors attached to this asset
		//BroadcastIKRigReplaced(SourceOrTarget);
		//BroadcastPreviewMeshReplaced(SourceOrTarget);
		//BroadcastNeedsReinitialized();
		*/
#endif
	}
#endif
};

class SimpleRigController {

public:
	UIKRigDefinition* RigDefinition = nullptr;

	SimpleRigController(UIKRigDefinition* rig) {
		RigDefinition = rig;
	}

#if VRM4U_USE_EDITOR_RIG

	UIKRigController* LocalGetController(UIKRigDefinition* rig) {
#if	UE_VERSION_OLDER_THAN(5,2,0)
		return UIKRigController::GetIKRigController(rig);
#else
		return UIKRigController::GetController(rig);
#endif
	}
	UIKRigController* LocalGetController(UIKRigDefinition* rig) const {
#if	UE_VERSION_OLDER_THAN(5,2,0)
		return UIKRigController::GetIKRigController(rig);
#else
		return UIKRigController::GetController(rig);
#endif
	}


#endif


	void SetSkeletalMesh(USkeletalMesh* sk) {

#if VRM4U_USE_EDITOR_RIG
		auto *r = LocalGetController(RigDefinition);
		r->SetSkeletalMesh(sk);
#else
		RigDefinition->PreviewSkeletalMesh = sk;
		const_cast<FIKRigSkeleton*>(&RigDefinition->GetSkeleton())->SetInputSkeleton(sk, RigDefinition->GetSkeleton().ExcludedBones);
		ResetInitialGoalTransforms();
#endif
	}

	bool SetRetargetRoot(const FName RootBoneName) const
	{
#if VRM4U_USE_EDITOR_RIG
		auto* r = LocalGetController(RigDefinition);
		return r->SetRetargetRoot(RootBoneName);
#else

		FName NewRootBone = RootBoneName;
		if (RootBoneName != NAME_None && RigDefinition->GetSkeleton().GetBoneIndexFromName(RootBoneName) == INDEX_NONE)
		{
			NewRootBone = NAME_None;
		}

		//FScopedTransaction Transaction(LOCTEXT("SetRetargetRootBone_Label", "Set Retarget Root Bone"));
		RigDefinition->Modify();

		*const_cast<FName*>(&RigDefinition->GetRetargetRoot()) = NewRootBone;

		//BroadcastNeedsReinitialized();

		return true;
#endif
	}

	FName VRMAddRetargetChain(FName name, FName begin, FName end) {
		FBoneChain c;
		FBoneReference r1, r2;
		r1.BoneName = begin;
		r2.BoneName = end;

		auto k = RigDefinition->GetPreviewMesh()->GetSkeleton();

		r1.Initialize(k);
		r2.Initialize(k);

		c.ChainName = name;
		c.StartBone = r1;
		c.EndBone = r2;

#if VRM4U_USE_EDITOR_RIG
		auto* r = LocalGetController(RigDefinition);
		r->AddRetargetChain(c);
		return NAME_None;
#else
		if (c.StartBone.BoneName != NAME_None && RigDefinition->GetSkeleton().GetBoneIndexFromName(c.StartBone.BoneName) == INDEX_NONE)
		{
			//UE_LOG(LogIKRigEditor, Warning, TEXT("Could not create retarget chain. Start Bone does not exist, %s."), *c.StartBone.BoneName.ToString());
			return NAME_None; // bone doesn't exist
		}

		if (c.EndBone.BoneName != NAME_None && RigDefinition->GetSkeleton().GetBoneIndexFromName(c.EndBone.BoneName) == INDEX_NONE)
		{
			//UE_LOG(LogIKRigEditor, Warning, TEXT("Could not create retarget chain. End Bone does not exist, %s."), *c.EndBone.BoneName.ToString());
			return NAME_None; // bone doesn't exist
		}

		FBoneChain ChainToAdd = c;

		// if no name specified, use a default
		if (ChainToAdd.ChainName == NAME_None)
		{
			const FName DefaultChainName = FName("DefaultChainName");
			ChainToAdd.ChainName = DefaultChainName;
		}

		ChainToAdd.ChainName = GetUniqueRetargetChainName(c.ChainName);

		//FScopedTransaction Transaction(LOCTEXT("AddRetargetChain_Label", "Add Retarget Chain"));
		RigDefinition->Modify();

		const int32 NewChainIndex = const_cast<TArray<FBoneChain>*>(&RigDefinition->GetRetargetChains())->Emplace(ChainToAdd);

		//RetargetChainAdded.Broadcast(RigDefinition);
		//BroadcastNeedsReinitialized();

		return NAME_None;// RigDefinition->RetargetDefinition.BoneChains[NewChainIndex].ChainName;
#endif
	}
	void LocalSolverSetup(UVrmAssetListObject *vrmAssetList) {
#if VRM4U_USE_EDITOR_RIG
		auto* r = LocalGetController(RigDefinition);
		::LocalSolverSetup(r, vrmAssetList);
#else
#endif
	}


	FTransform GetRefPoseTransformOfBone(const FName BoneName) const
	{
		const int32 BoneIndex = RigDefinition->GetSkeleton().GetBoneIndexFromName(BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			//UE_LOG(LogIKRigEditor, Warning, TEXT("Tried to get the ref pose of bone that is not loaded into this rig."));
			return FTransform::Identity;
		}

		return RigDefinition->GetSkeleton().RefPoseGlobal[BoneIndex];
	}
	void ResetInitialGoalTransforms() const
	{
		for (UIKRigEffectorGoal* Goal : RigDefinition->GetGoalArray())
		{
			// record the current delta rotation
			const FQuat DeltaRotation = Goal->CurrentTransform.GetRotation() * Goal->InitialTransform.GetRotation().Inverse();
			// update the initial transform based on the new ref pose
			const FTransform InitialTransform = GetRefPoseTransformOfBone(Goal->BoneName);
			Goal->InitialTransform = InitialTransform;
			// restore the current transform
			Goal->CurrentTransform.SetRotation(Goal->InitialTransform.GetRotation() * DeltaRotation);
		}
	}
	FName GetUniqueRetargetChainName(const FName NameToMakeUnique) const
	{
		auto IsNameBeingUsed = [this](const FName NameToTry)->bool
			{
				for (const FBoneChain& Chain : RigDefinition->GetRetargetChains())
				{
					if (Chain.ChainName == NameToTry)
					{
						return true;
					}
				}
				return false;
			};

		// check if name is already unique
		if (!IsNameBeingUsed(NameToMakeUnique))
		{
			return NameToMakeUnique;
		}

		// keep concatenating an incremented integer suffix until name is unique
		int32 Number = NameToMakeUnique.GetNumber() + 1;
		while (IsNameBeingUsed(FName(NameToMakeUnique, Number)))
		{
			Number++;
		}

		return FName(NameToMakeUnique, Number);
	}
};


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


#if	UE_VERSION_OLDER_THAN(4,20,0)
#else

#if WITH_EDITOR
	auto* k = VRMGetSkeleton(vrmAssetList->SkeletalMesh);

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

	FString PelvisBoneName;
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

				if (PelvisBoneName.Len() == 0) {
					if (ue4.Compare(TEXT("Pelvis"), ESearchCase::IgnoreCase) == 0) {
						PelvisBoneName = target;
					}
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

	{
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
		while(bone >= 0){
			if (first) {
				k->SetBoneTranslationRetargetingMode(bone, EBoneTranslationRetargetingMode::AnimationScaled);
			} else {
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

			for (auto &s : n) {
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

		if (VRMConverter::Options::Get().IsVRMModel() == false) {
			k->SetBoneTranslationRetargetingMode(0, EBoneTranslationRetargetingMode::Animation, false);
		}

	}
	//mc->AddMapping
	if (mc) {
		mc->PostEditChange();
		vrmAssetList->HumanoidRig = mc;
	}
#endif // editor
#endif //420

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

	if (vrmAssetList && VRMConverter::Options::Get().IsGenerateRigIK()) {
		// ikrig
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
		const VRM::VRMMetadata* meta = reinterpret_cast<VRM::VRMMetadata*>(aiData->mVRMMeta);
		USkeletalMesh* sk = vrmAssetList->SkeletalMesh;

		UIKRigDefinition* rig = nullptr;
		{
			FString name = FString(TEXT("IK_")) + vrmAssetList->BaseFileName + TEXT("_VrmHumanoid");
			rig = VRM4U_NewObject<UIKRigDefinition>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);

			SimpleRigController rigcon(rig);
			rigcon.SetSkeletalMesh(sk);

			if (vrmAssetList->VrmMetaObject->humanoidBoneTable.Num() == 0) {
				// immediate generate bone table
				/*
				if (mc) {
					for (auto& boneMap : VRMUtil::table_ue4_vrm) {
						FString s;
						for (auto& table : mc->GetNodeMappingTable()) {
							if (table.Key.ToString().ToLower() == boneMap.BoneUE4.ToLower()) {
								s = table.Value.ToString();
							}
						}

						if (s != "" && boneMap.BoneVRM != "") {
							vrmAssetList->VrmMetaObject->humanoidBoneTable.Add(boneMap.BoneVRM, s);
						}
					}
				}
				*/
			}
			{
				auto s = VRMGetRefSkeleton(sk).GetBoneName(0);
				rigcon.VRMAddRetargetChain(TEXT("root"), s, s);
			}
			for (auto& modelName : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
				if (modelName.Key == "" || modelName.Value == "") {
					continue;
				}

				// spine
				int type = 0;
				if (modelName.Key == TEXT("spine")) {
					type = 1;
				}
				if (modelName.Key == TEXT("chest") || modelName.Key == TEXT("upperChest")) {
					type = 2;
				}

				switch (type) {
				case 0:
					rigcon.VRMAddRetargetChain(*modelName.Key, *modelName.Value, *modelName.Value);
					break;
				case 1:
				{
					auto *s = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("upperChest"));
					if (s == nullptr) {
						s = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("chest"));
					}

					if (s) {
						// spine chain
						rigcon.VRMAddRetargetChain(*modelName.Key, *modelName.Value, **s);
					}
				}
					break;
				default:
					break;
				}


				if (modelName.Key == TEXT("hips")) {
					rigcon.SetRetargetRoot(*modelName.Value);
				}
			}
		}

		{
			UIKRigDefinition* rig_epic = nullptr;

			FString name = FString(TEXT("IK_")) + vrmAssetList->BaseFileName + TEXT("_MannequinBone");
			rig_epic = VRM4U_NewObject<UIKRigDefinition>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);

			SimpleRigController rigcon = SimpleRigController(rig_epic);
			rigcon.SetSkeletalMesh(sk);

			{
				auto s = VRMGetRefSkeleton(sk).GetBoneName(0);
				rigcon.VRMAddRetargetChain(TEXT("root"), s, s);
			}
			for (auto& modelName : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
				if (modelName.Key == "" || modelName.Value == "") {
					continue;
				}
				for (auto& a : VRMUtil::table_ue4_vrm) {
					if (a.BoneUE4 == "" || a.BoneVRM == "") {
						continue;
					}
					if (a.BoneVRM.ToLower() == modelName.Key.ToLower()) {
						rigcon.VRMAddRetargetChain(*a.BoneUE4, *modelName.Value, *modelName.Value);
						if (modelName.Key == TEXT("hips")) {
							rigcon.SetRetargetRoot(*modelName.Value);
						}
						break;
					}
				}
			}
		}

		UIKRigDefinition* rig_ik = nullptr;
		{
			FString name = FString(TEXT("IK_")) + vrmAssetList->BaseFileName + TEXT("_Mannequin");
			rig_ik = VRM4U_NewObject<UIKRigDefinition>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);

			SimpleRigController rigcon = SimpleRigController(rig_ik);
			rigcon.SetSkeletalMesh(sk);

			{
				auto s = VRMGetRefSkeleton(sk).GetBoneName(0);
				rigcon.VRMAddRetargetChain(TEXT("Root"), s, s);
			}
			struct TT {
				FString chain;
				FString s1;
				FString s2;
			};
			TArray<TT> table = {
				{TEXT("Spine"),		TEXT("spine"),				TEXT("chest"),},
				{TEXT("Head"),		TEXT("neck"),				TEXT("head"),},
				{TEXT("RightArm"),	TEXT("rightUpperArm"),		TEXT("rightHand"),},
				{TEXT("LeftArm"),	TEXT("leftUpperArm"),		TEXT("leftHand"),},
				{TEXT("RightLeg"),	TEXT("rightUpperLeg"),		TEXT("rightToes"),},
				{TEXT("LeftLeg"),	TEXT("leftUpperLeg"),		TEXT("leftToes"),},

				{TEXT("LeftThumb"),		TEXT("leftThumbProximal"),		TEXT("leftThumbDistal"),},
				{TEXT("LeftIndex"),		TEXT("leftIndexProximal"),		TEXT("leftIndexDistal"),},
				{TEXT("LeftMiddle"),	TEXT("leftMiddleProximal"),	TEXT("leftMiddleDistal"),},
				{TEXT("LeftRing"),		TEXT("leftRingProximal"),		TEXT("leftRingDistal"),},
				{TEXT("LeftPinky"),		TEXT("leftLittleProximal"),	TEXT("leftLittleDistal"),},

				{TEXT("RightThumb"),	TEXT("rightThumbProximal"),	TEXT("rightThumbDistal"),},
				{TEXT("RightIndex"),	TEXT("rightIndexProximal"),	TEXT("rightIndexDistal"),},
				{TEXT("RightMiddle"),	TEXT("rightMiddleProximal"),	TEXT("rightMiddleDistal"),},
				{TEXT("RightRing"),		TEXT("rightRingProximal"),		TEXT("rightRingDistal"),},
				{TEXT("RightPinky"),	TEXT("rightLittleProximal"),	TEXT("rightLittleDistal"),},

				{TEXT("LeftClavicle"),		TEXT("leftShoulder"),	TEXT("leftShoulder"),},
				{TEXT("RightClavicle"),		TEXT("rightShoulder"),	TEXT("rightShoulder"),},
			};

			for (auto& t : table) {
				TT conv;
				for (auto& modelName : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
					if (modelName.Key == "" || modelName.Value == "") {
						continue;
					}
					if (t.s1.ToLower() == modelName.Key.ToLower()) {
						conv.s1 = modelName.Value;
					}
					if (t.s2.ToLower() == modelName.Key.ToLower()) {
						conv.s2 = modelName.Value;
					}
				}

				FString baseBone;
				for (auto& a : VRMUtil::table_ue4_vrm) {
					if (a.BoneUE4 == "" || a.BoneVRM == "") {
						continue;
					}
					if (a.BoneVRM.ToLower() == t.s1.ToLower()) {
						baseBone = a.BoneUE4;
					}
				}
				if (baseBone != "" && conv.s1 != "" && conv.s2 != "") {
					FString s2 = conv.s2;
					if (t.chain == TEXT("Spine")) {
						// neck parent
						for (auto& modelName : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
							if (modelName.Key == "neck") {
								auto& ref = VRMGetRefSkeleton(sk);
									auto index = ref.FindRawBoneIndex(*modelName.Value);
									auto tmp = ref.GetBoneName(ref.GetParentIndex(index));
									s2 = tmp.ToString();
							}
						}
					}
					rigcon.VRMAddRetargetChain(*t.chain, *conv.s1, *s2);
				}
			}
			rigcon.LocalSolverSetup(vrmAssetList);

			for (auto& modelName : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
				if (modelName.Key == "" || modelName.Value == "") {
					continue;
				}
				if (modelName.Key == TEXT("hips")) {
					rigcon.SetRetargetRoot(*modelName.Value);
				}
			}
		} // skeleton ik


		UIKRetargeter* ikr = nullptr;
		{
			FString name = FString(TEXT("RTG_")) + vrmAssetList->BaseFileName;
			ikr = VRM4U_NewObject<UIKRetargeter>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);

			SimpleRetargeterController c = SimpleRetargeterController(ikr);

#if	UE_VERSION_OLDER_THAN(5,2,0)
			if (VRMConverter::Options::Get().IsVRMModel() || VRMConverter::Options::Get().IsBVHModel()) {
				c.SetSourceIKRig(rig_ik);
			}
			else {
				c.SetSourceIKRig(rig);
			}
#else
			if (VRMConverter::Options::Get().IsVRMModel() || VRMConverter::Options::Get().IsBVHModel()) {
				c.SetIKRig(ERetargetSourceOrTarget::Target, rig_ik);

				//static ConstructorHelpers::FObjectFinder<UVrmAssetListObject> MatClass(TEXT("Blueprint'/VRM4U/VrmObjectListBP'"));

				//static ConstructorHelpers::FObjectFinder<UMaterialInterface> OpaqueMaterialRef(TEXT("/Paper2D/OpaqueUnlitSpriteMaterial"));
				//AlternateMaterial = OpaqueMaterialRef.Object;

				//static ConstructorHelpers::FObjectFinder<UIKRigDefinition> test(TEXT("/Game/Characters/Mannequins/Rigs/IK_Mannequin"));
				//auto r2 = test.Object;
				
				FSoftObjectPath r(TEXT("/Game/Characters/Mannequins/Rigs/IK_Mannequin.IK_Mannequin"));
				UObject* u = r.TryLoad();
				if (u) {
					auto r2 = Cast<UIKRigDefinition>(u);
					if (r2) {
						c.SetIKRig(ERetargetSourceOrTarget::Source, r2);
					}
					//c = (UClass*)(Cast<UBlueprint>(u)->GeneratedClass);
				}

				/// Script / IKRig.IKRigDefinition'/Game/Characters/Mannequins/Rigs/IK_Mannequin.IK_Mannequin'
			}
			else {
				c.SetIKRig(ERetargetSourceOrTarget::Target, rig);
			}
#endif
		}
#endif
	} // ikrig

	return true;

}

