// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertIKRig.h"
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

#if WITH_EDITOR
#if UE_VERSION_OLDER_THAN(5,4,0)
#else
#define VRM4U_USE_AUTOALIGN 1
#endif
#endif

#ifndef VRM4U_USE_AUTOALIGN
#define VRM4U_USE_AUTOALIGN 0
#endif


#define VRM4U_USE_EDITOR_RIG WITH_EDITOR

namespace {

#if VRM4U_USE_EDITOR_RIG && WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else

	static void LocalSolverSetup(UIKRigController* rigcon, UVrmAssetListObject *assetList, int table_no=0xFFFF) {
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
#elif UE_VERSION_OLDER_THAN(5,6,0)
			sol_index = rigcon->AddSolver(UIKRigFBIKSolver::StaticClass());
			sol = rigcon->GetSolverAtIndex(sol_index);
#else
			sol_index = rigcon->AddSolver(FIKRigFullBodyIKSolver::StaticStruct());
			sol = rigcon->GetSolverAtIndex(sol_index);
#endif
		}
		if (sol == nullptr) return;

#if UE_VERSION_OLDER_THAN(5,4,0)
		sol->SetEnabled(false);
#else
		sol->SetEnabled(true);
#endif

		// hip
		for (auto& modelName : assetList->VrmMetaObject->humanoidBoneTable) {
			if (modelName.Key == "" || modelName.Value == "") {
				continue;
			}
			if (modelName.Key == TEXT("hips")) {
#if UE_VERSION_OLDER_THAN(5,6,0)
				sol->SetRootBone(*modelName.Value);
#else
				sol->SetStartBone(*modelName.Value);
#endif

			}
		}

		{
			TArray<FString> a = {
				TEXT("leftHand"),
				TEXT("rightHand"),
				TEXT("leftToes"),
				TEXT("rightToes"),
			};

			if (table_no == 0){
				// not change
			}else if (table_no == 1){
				TArray < FString > tmp = {
					TEXT("leftHand"),
					TEXT("rightHand"),
					TEXT("leftFoot"),
					TEXT("rightFoot"),
				};
				a = tmp;
			}


			if (VRMConverter::Options::Get().IsBVHModel()) {
				{
					TArray < FString > tmp = {
						TEXT("l_hand"),
						TEXT("r_hand"),
						TEXT("l_foot"),
						TEXT("r_foot"),
					};
					a = tmp;
				}
				
				if (table_no == 1) {
					TArray < FString > tmp = {
						TEXT("l_hand"),
						TEXT("r_hand"),
						TEXT("l_toes"),
						TEXT("r_toes"),
					};
					a = tmp;
				}
			}

			for (int i = 0; i < a.Num(); ++i) {
				for (auto& t : assetList->VrmMetaObject->humanoidBoneTable) {
					if (t.Key.ToLower() == a[i].ToLower()) {

#if	UE_VERSION_OLDER_THAN(5,1,0)
#elif	UE_VERSION_OLDER_THAN(5,2,0)
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
#elif UE_VERSION_OLDER_THAN(5,6,0)
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
#else
						auto goal = rigcon->AddNewGoal(*(a[i] + TEXT("_Goal")), *t.Value);
						if (goal != NAME_None) {
							rigcon->ConnectGoalToSolver(goal, sol_index);

							// arm chain
							if (i == 0 || i == 1) {
								auto *sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));

								if (sc) {
									auto settings = sc->GetGoalSettings(goal);
									settings.PullChainAlpha = 0.f;
									sc->SetGoalSettings(goal, settings);
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
			// BVH
			if (VRMConverter::Options::Get().IsBVHModel()) {
#if	UE_VERSION_OLDER_THAN(5,2,0)
#elif UE_VERSION_OLDER_THAN(5,6,0)
				for (int i = 0; i < a.Num(); ++i) {


					USkeletalMesh* sk = assetList->SkeletalMesh;

					//auto boneList = VRMGetRefSkeleton(sk).GetRefBonePose();
					auto ind = VRMGetRefSkeleton(sk).FindBoneIndex(*a[i]);

					if (ind < 0) continue;

					auto goal = rigcon->AddNewGoal(*(a[i] + TEXT("_Goal")), *a[i]);
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
							if (c.EndBone.BoneName == *a[i]) {
								rigcon->SetRetargetChainGoal(c.ChainName, goal);
							}
						}
					}
					
				}
#else
				for (int i = 0; i < a.Num(); ++i) {

					USkeletalMesh* sk = assetList->SkeletalMesh;

					//auto boneList = VRMGetRefSkeleton(sk).GetRefBonePose();
					auto ind = VRMGetRefSkeleton(sk).FindBoneIndex(*a[i]);

					if (ind < 0) continue;

					auto goal = rigcon->AddNewGoal(*(a[i] + TEXT("_Goal")), *a[i]);
					if (goal != NAME_None) {
						rigcon->ConnectGoalToSolver(goal, sol_index);

						// arm chain
						if (i == 0 || i == 1) {
							auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));

							if (sc) {
								auto settings = sc->GetGoalSettings(goal);
								settings.PullChainAlpha = 0.f;
								sc->SetGoalSettings(goal, settings);
							}

							//UIKRig_FBIKEffector* e = Cast<UIKRig_FBIKEffector>(sol->GetGoalSettings(goal));
							//if (e) {
							//	e->PullChainAlpha = 0.f;
							//}
						}

						const auto& chain = rigcon->GetRetargetChains();
						for (auto& c : chain) {
							if (c.EndBone.BoneName == *a[i]) {
								rigcon->SetRetargetChainGoal(c.ChainName, goal);
							}
						}
					}

				}
#endif
			}// bvh
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

				TEXT("leftFoot"),
				TEXT("rightFoot"),
				

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
#if	UE_VERSION_OLDER_THAN(5,6,0)
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							s->RotationStiffness = 0.95f;
#else
							sol->AddSettingsToBone(*t.Value);
							auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
							if (sc) {
								auto settings = sc->GetBoneSettings(*t.Value);
								settings.RotationStiffness = 0.95f;
								sc->SetBoneSettings(*t.Value, settings);
							}
#endif
						}

						// arm
						if (i == 2 || i == 3) {
#if	UE_VERSION_OLDER_THAN(5,6,0)
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							s->bUsePreferredAngles = true;
							if (i == 2) {
								s->PreferredAngles.Set(0, 0, 90);
							}
							else {
								s->PreferredAngles.Set(0, 0, -90);
							}
#else
							sol->AddSettingsToBone(*t.Value);
							auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
							if (sc) {
								auto settings = sc->GetBoneSettings(*t.Value);
								settings.bUsePreferredAngles = true;
								if (i == 2) {
									settings.PreferredAngles.Set(0, 0, 90);
								} else {
									settings.PreferredAngles.Set(0, 0, -90);
								}
								sc->SetBoneSettings(*t.Value, settings);
							}
#endif
						}

						// only lower leg
						if (i == 6 || i == 7) {
#if	UE_VERSION_OLDER_THAN(5,6,0)
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							s->bUsePreferredAngles = true;
							if (i == 4 || i == 5) {
								s->PreferredAngles.Set(-180, 0, 0);
							}
							else {
								s->PreferredAngles.Set(180, 0, 0);
								s->Y = EPBIKLimitType::Locked;
								s->Z = EPBIKLimitType::Locked;
							}
#else
							sol->AddSettingsToBone(*t.Value);
							auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
							if (sc) {
								auto settings = sc->GetBoneSettings(*t.Value);
								settings.bUsePreferredAngles = true;
								if (i == 4 || i == 5) {
									settings.PreferredAngles.Set(-180, 0, 0);
								}
								else {
									settings.PreferredAngles.Set(180, 0, 0);
									settings.Y = EPBIKLimitType::Locked;
									settings.Z = EPBIKLimitType::Locked;
								}
								sc->SetBoneSettings(*t.Value, settings);
							}
#endif
						}

						// foot
						if (i == 8 || i == 9) {
#if	UE_VERSION_OLDER_THAN(5,6,0)
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							s->RotationStiffness = 0.85f;
#else
							sol->AddSettingsToBone(*t.Value);
							auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
							if (sc) {
								auto settings = sc->GetBoneSettings(*t.Value);
								settings.RotationStiffness = 0.85f;
								sc->SetBoneSettings(*t.Value, settings);
							}
#endif
						}

						// spine
						if (i >= 10) {
#if	UE_VERSION_OLDER_THAN(5,6,0)
							sol->AddBoneSetting(*t.Value);
							UIKRig_PBIKBoneSettings* s = Cast<UIKRig_PBIKBoneSettings>(sol->GetBoneSetting(*t.Value));
							if (s == nullptr) continue;

							if (i == 10) {
								s->RotationStiffness = 1.f;
							}
							else {
								s->RotationStiffness = 0.9f;
							}
#else
							sol->AddSettingsToBone(*t.Value);
							auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
							if (sc) {
								auto settings = sc->GetBoneSettings(*t.Value);
								if (i == 10) {
									settings.RotationStiffness = 1.f;
								}
								else {
									settings.RotationStiffness = 0.9f;
								}
								sc->SetBoneSettings(*t.Value, settings);
							}
#endif
						}

					}
				}
			}
		}
	}
#endif
#endif
}

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else

#if WITH_EDITOR
#else
class UIKRetargeterController {
public:
	static void setSourceRig(UIKRetargeter* retargeter, UIKRigDefinition* rig){
		retargeter->SourceIKRigAsset = rig;
	}
};
#endif
#endif


#if	UE_VERSION_OLDER_THAN(5,1,0)
#else

class SimpleRetargeterController {
	//friend class UIKRetargeter;

	UIKRetargeter *Retargeter = nullptr;
public:
	SimpleRetargeterController(UIKRetargeter* rig) {
		Retargeter = rig;
	}

	FName CreateRetargetPose(const FName& NewPoseName, const ERetargetSourceOrTarget SourceOrTarget) const {
#if	UE_VERSION_OLDER_THAN(5,2,0)
		return NAME_None;
#else

#if VRM4U_USE_EDITOR_RIG
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		return c->CreateRetargetPose(NewPoseName, SourceOrTarget);
#else
		return NewPoseName;
#endif

#endif // 5.2
	}

	FIKRetargetPose* GetRetargetPosesByName(const ERetargetSourceOrTarget SourceOrTarget, FName poseName) const
	{
#if VRM4U_USE_EDITOR_RIG
		//FScopeLock Lock(&ControllerLock);
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		return &c->GetRetargetPoses(SourceOrTarget)[poseName];
#else
		//return SourceOrTarget == ERetargetSourceOrTarget::Source ? Retargeter->SourceRetargetPoses : Retargeter->TargetRetargetPoses;
		return const_cast<FIKRetargetPose*>(Retargeter->GetRetargetPoseByName(SourceOrTarget, poseName));

#endif
		//return SourceOrTarget == ERetargetSourceOrTarget::Source ? GetAsset()->SourceRetargetPoses : GetAsset()->TargetRetargetPoses;
	}


#if	UE_VERSION_OLDER_THAN(5,2,0)
	void SetIKRig(UIKRigDefinition* IKRig) const {
	}
#else
	void SetIKRig(const ERetargetSourceOrTarget SourceOrTarget, UIKRigDefinition* IKRig) const {
#if VRM4U_USE_EDITOR_RIG
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		c->SetIKRig(SourceOrTarget, IKRig);
#else
#if	UE_VERSION_OLDER_THAN(5,6,0)
		UIKRetargeterController::setSourceRig(Retargeter, IKRig);
#else
		// no rig
#endif
#endif
	}
#endif

	void AutoAlignAllBones(const ERetargetSourceOrTarget SourceOrTarget) const {
#if	!WITH_EDITOR || UE_VERSION_OLDER_THAN(5,4,0)
#else
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		c->AutoAlignAllBones(SourceOrTarget);
#endif
	}

	void SetCurrentRetargetPose(FName NewCurrentPose, const ERetargetSourceOrTarget SourceOrTarget) const {
#if	!WITH_EDITOR || UE_VERSION_OLDER_THAN(5,4,0)
#else
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		c->SetCurrentRetargetPose(NewCurrentPose, SourceOrTarget);
#endif
	}

	TMap<FName, FIKRetargetPose>& GetRetargetPoses(const ERetargetSourceOrTarget SourceOrTarget) const {
#if	!WITH_EDITOR || UE_VERSION_OLDER_THAN(5,4,0)
		static TMap<FName, FIKRetargetPose> a;
		return a;
#else
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		return c->GetRetargetPoses(SourceOrTarget);
#endif
	};
	void SetRotationOffsetForRetargetPoseBone(
		const FName& BoneName,
		const FQuat& RotationOffset,
		const ERetargetSourceOrTarget SourceOrTarget) const {
#if	!WITH_EDITOR || UE_VERSION_OLDER_THAN(5,4,0)
		return;
#else
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		return c->SetRotationOffsetForRetargetPoseBone(BoneName, RotationOffset, SourceOrTarget);
#endif

	}


	void SetChainSetting() {
#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
#if VRM4U_USE_EDITOR_RIG
		UIKRetargeterController* c = UIKRetargeterController::GetController(Retargeter);
		{
			auto cs = c->GetRetargetChainSettings(TEXT("Root"));
			cs.FK.TranslationMode = ERetargetTranslationMode::GloballyScaled;
			c->SetRetargetChainSettings(TEXT("Root"), cs);
		}
		{
			TArray<FString> table = {
				TEXT("FootRootIK"),
				TEXT("LeftFootIK"),
				TEXT("RightFootIK"),
				TEXT("HandRootIK"),
				TEXT("LeftHandIK"),
				TEXT("RightHandIK"),
			};
			for (auto s : table) {
				auto cs = c->GetRetargetChainSettings(*s);
				cs.FK.TranslationMode = ERetargetTranslationMode::GloballyScaled;
				c->SetRetargetChainSettings(*s, cs);
			}
		}
		/*
		{
			TArray<FString> table = {
				TEXT("LeftLeg"),
				TEXT("RightLeg"),
				TEXT("LeftArm"),
				TEXT("RightArm"),
			};
			for (auto s : table) {
				auto cs = c->GetRetargetChainSettings(*s);
				cs.FK.PoleVectorMatching = 1.f;
				c->SetRetargetChainSettings(*s, cs);
			}
		}
		{
			TArray<FString> table = {
				TEXT("LeftArm"),
				TEXT("RightArm"),
			};
			for (auto s : table) {
				auto cs = c->GetRetargetChainSettings(*s);
				cs.IK.bAffectedByIKWarping = false;
				c->SetRetargetChainSettings(*s, cs);
			}
		}
		*/

#else
		auto r = Retargeter->GetChainMapByName(TEXT("Root"));
		r->Settings.FK.TranslationMode = ERetargetTranslationMode::GloballyScaled;
#endif // rig
#endif // 5.2
	}
};
#endif // 5.0

#if	UE_VERSION_OLDER_THAN(5,0,0) 
#else

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
#if	UE_VERSION_OLDER_THAN(5,2,0)
#if VRM4U_USE_EDITOR_RIG
		auto * r = LocalGetController(RigDefinition);
		r->SetSkeletalMesh(sk);
#endif
#else

#if VRM4U_USE_EDITOR_RIG
		auto *r = LocalGetController(RigDefinition);
		r->SetSkeletalMesh(sk);
#else
		RigDefinition->PreviewSkeletalMesh = sk;
		const_cast<FIKRigSkeleton*>(&RigDefinition->GetSkeleton())->SetInputSkeleton(sk, RigDefinition->GetSkeleton().ExcludedBones);
		ResetInitialGoalTransforms();
#endif
#endif
	}

	bool SetRetargetRoot(const FName RootBoneName) const
	{
#if	UE_VERSION_OLDER_THAN(5,2,0)
		return true;
#else
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

#if	UE_VERSION_OLDER_THAN(5,6,0)
		*const_cast<FName*>(&RigDefinition->GetRetargetRoot()) = NewRootBone;
#else
#endif

		//BroadcastNeedsReinitialized();

		return true;
#endif
#endif
	}

	FName VRMAddRetargetChain(FName name, FName begin, FName end) {
#if	UE_VERSION_OLDER_THAN(5,2,0)
		return NAME_None;
#else
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
#if UE_VERSION_OLDER_THAN(5,4,0)
		r->AddRetargetChain(c);
#else
		r->AddRetargetChain(name, begin, end, NAME_None);
#endif
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
#endif
	}
	void LocalSolverSetup(UVrmAssetListObject *vrmAssetList, int table_no=0xFFFF) {
#if VRM4U_USE_EDITOR_RIG
		auto* r = LocalGetController(RigDefinition);
		::LocalSolverSetup(r, vrmAssetList, table_no);
#else
#endif
	}


	FTransform GetRefPoseTransformOfBone(const FName BoneName) const
	{
#if	UE_VERSION_OLDER_THAN(5,2,0)
		return FTransform::Identity;
#else
		const int32 BoneIndex = RigDefinition->GetSkeleton().GetBoneIndexFromName(BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			//UE_LOG(LogIKRigEditor, Warning, TEXT("Tried to get the ref pose of bone that is not loaded into this rig."));
			return FTransform::Identity;
		}

		return RigDefinition->GetSkeleton().RefPoseGlobal[BoneIndex];
#endif
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

#endif // 5.0


#if WITH_EDITOR
#if UE_VERSION_OLDER_THAN(5,6,0)
#else
static UIKRigDefinition* GenerateMannequinIK(UVrmAssetListObject* vrmAssetList) {

	TArray<FString> MannyAssetNameList;
	MannyAssetNameList.Add(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	MannyAssetNameList.Add(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	MannyAssetNameList.Add(TEXT("/Game/Characters/UE5_Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	MannyAssetNameList.Add(TEXT("/Game/Characters/UE5_Mannequins/Meshes/SKM_Manny.SKM_Manny"));

	UObject* u = nullptr;
	for (auto manny : MannyAssetNameList) {
		FSoftObjectPath r2(manny);
		u = r2.TryLoad();
		if (u != nullptr) {
			break;
		}
	}

	if (u == nullptr) return nullptr;

	USkeletalMesh* sk = nullptr;
	if (u) {
		auto r2 = Cast<USkeletalMesh>(u);
		if (r2) {
			sk = r2;
		}
	}
	if (sk == nullptr) return nullptr;

	FString name = FString(TEXT("IK_")) + TEXT("Default") + TEXT("_Mannequin");

	UIKRigDefinition* rig_ik = nullptr;
	{
		// 生成済なら返す
		FSoftObjectPath r2(name);
		rig_ik = Cast <UIKRigDefinition>(r2.TryLoad());
		if (rig_ik) return rig_ik;
	}

	rig_ik = VRM4U_NewObject<UIKRigDefinition>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);

	{
		SimpleRigController rigcon = SimpleRigController(rig_ik);
		rigcon.SetSkeletalMesh(sk);

		// root
		{
			auto s = VRMGetRefSkeleton(sk).GetBoneName(0);
			rigcon.VRMAddRetargetChain(TEXT("Root"), s, s);
		}

		struct TT {
			FString chain;
			FString s1;
			FString s2;
			uint32_t mask = 0xFFFF;
		};

		TArray<FString> AddedChainList;

		TArray<TT> table = {
			{TEXT("Spine"),		TEXT("spine_01"),				TEXT("spine_05"),},
			{TEXT("Neck"),		TEXT("neck_01"),				TEXT("neck_02"),},
			{TEXT("Head"),		TEXT("head"),				TEXT("head"),},

			{TEXT("LeftClavicle"),	TEXT("clavicle_l"),		TEXT("clavicle_l"),},
			{TEXT("LeftArm"),		TEXT("upperarm_l"),		TEXT("hand_l"),},

			{TEXT("RightClavicle"),	TEXT("clavicle_r"),		TEXT("clavicle_r"),},
			{TEXT("RightArm"),		TEXT("upperarm_r"),		TEXT("hand_r"),},


			{TEXT("LeftIndexMetacarpal"),	TEXT("index_metacarpal_l"),		TEXT("index_metacarpal_l"),},
			{TEXT("LeftIndex"),				TEXT("index_01_l"),				TEXT("index_03_l"),},
			{TEXT("LeftMiddleMetacarpal"),	TEXT("middle_metacarpal_l"),	TEXT("middle_metacarpal_l"),},
			{TEXT("LeftMiddle"),			TEXT("middle_01_l"),			TEXT("middle_03_l"),},
			{TEXT("LeftThumb"),				TEXT("thumb_01_l"),				TEXT("thumb_03_l"),},
			{TEXT("LeftPinkyMetacarpal"),	TEXT("pinky_metacarpal_l"),		TEXT("pinky_metacarpal_l"),},
			{TEXT("LeftPinky"),				TEXT("pinky_01_l"),				TEXT("pinky_03_l"),},
			{TEXT("LeftRingMetacarpal"),	TEXT("ring_metacarpal_l"),		TEXT("ring_metacarpal_l"),},
			{TEXT("LeftRing"),				TEXT("ring_01_l"),				TEXT("ring_03_l"),},

			{TEXT("RightIndexMetacarpal"),	TEXT("index_metacarpal_r"),		TEXT("index_metacarpal_r"),},
			{TEXT("RightIndex"),			TEXT("index_01_r"),				TEXT("index_03_r"),},
			{TEXT("RightMiddleMetacarpal"),	TEXT("middle_metacarpal_r"),	TEXT("middle_metacarpal_r"),},
			{TEXT("RightMiddle"),			TEXT("middle_01_r"),			TEXT("middle_03_r"),},
			{TEXT("RightThumb"),			TEXT("thumb_01_r"),				TEXT("thumb_03_r"),},
			{TEXT("RightPinkyMetacarpal"),	TEXT("pinky_metacarpal_r"),		TEXT("pinky_metacarpal_r"),},
			{TEXT("RightPinky"),			TEXT("pinky_01_r"),				TEXT("pinky_03_r"),},
			{TEXT("RightRingMetacarpal"),	TEXT("ring_metacarpal_r"),		TEXT("ring_metacarpal_r"),},
			{TEXT("RightRing"),				TEXT("ring_01_r"),				TEXT("ring_03_r"),},

			{TEXT("LeftLeg"),	TEXT("thigh_l"),		TEXT("ball_l"),},
			{TEXT("RightLeg"),	TEXT("thigh_r"),		TEXT("ball_r"),},

		};

		for (auto& t : table) {
			rigcon.VRMAddRetargetChain(*t.chain, *t.s1, *t.s2);
		}
	}

	{
		auto* rigcon = UIKRigController::GetController(rig_ik);

		int sol_index = 0;

		sol_index = rigcon->AddSolver(FIKRigFullBodyIKSolver::StaticStruct());
		auto *sol = rigcon->GetSolverAtIndex(sol_index);

		{
			auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
			auto s = sc->GetSolverSettings();
			s.RootBehavior = EPBIKRootBehavior::Free;
			s.GlobalPullChainAlpha = 0;
			sol->SetSolverSettings(&s);
		}

		sol->SetEnabled(true);
		{
			FName boneName = TEXT("pelvis");
			sol->SetStartBone(boneName);
			rigcon->SetRetargetRoot(boneName);
		}
		{
			FName boneList[] = {
				TEXT("pelvis"),
				TEXT("clavicle_l"),
				TEXT("clavicle_r"),
			};

			for (auto& boneName : boneList) {
				sol->AddSettingsToBone(boneName);
				auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
				if (sc) {
					auto settings = sc->GetBoneSettings(boneName);
					settings.RotationStiffness = 0.95f;
					sc->SetBoneSettings(boneName, settings);
				}
			}
		}
		{
			FName boneList[] = {
				TEXT("lowerarm_r"),
				TEXT("lowerarm_l"),
				TEXT("calf_r"),
				TEXT("calf_l"),
			};

			for (auto& boneName : boneList) {
				sol->AddSettingsToBone(boneName);
				auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
				if (sc) {
					auto settings = sc->GetBoneSettings(boneName);
					settings.X = EPBIKLimitType::Locked;
					settings.Y = EPBIKLimitType::Locked;
					settings.bUsePreferredAngles = true;
					settings.PreferredAngles.Set(0, 0, 90);
					sc->SetBoneSettings(boneName, settings);
				}
			}
		}
		{
			FName boneList[] = {
				TEXT("foot_r"),
				TEXT("foot_l"),
			};

			for (auto& boneName : boneList) {
				sol->AddSettingsToBone(boneName);
				auto* sc = Cast<UIKRigFBIKController>(rigcon->GetSolverController(sol_index));
				if (sc) {
					auto settings = sc->GetBoneSettings(boneName);
					settings.RotationStiffness = 0.85f;
					sc->SetBoneSettings(boneName, settings);
				}
			}
		}

		{
			auto goal = rigcon->AddNewGoal(TEXT("RightHandIK"), TEXT("hand_r"));
			rigcon->SetRetargetChainGoal(TEXT("RightArm"), goal);
			rigcon->ConnectGoalToSolver(goal, sol_index);
		}
		{
			auto goal = rigcon->AddNewGoal(TEXT("LeftHandIK"), TEXT("hand_l"));
			rigcon->SetRetargetChainGoal(TEXT("LeftArm"), goal);
			rigcon->ConnectGoalToSolver(goal, sol_index);
		}
		{
			auto goal = rigcon->AddNewGoal(TEXT("RightFootIK"), TEXT("ball_r"));
			rigcon->SetRetargetChainGoal(TEXT("RightLeg"), goal);
			rigcon->ConnectGoalToSolver(goal, sol_index);
		}
		{
			auto goal = rigcon->AddNewGoal(TEXT("LeftFootIK"), TEXT("ball_l"));
			rigcon->SetRetargetChainGoal(TEXT("LeftLeg"), goal);
			rigcon->ConnectGoalToSolver(goal, sol_index);
		}
	}
	return rig_ik;
}
#endif
#endif


bool VRMConverter::ConvertIKRig(UVrmAssetListObject *vrmAssetList) {

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

		UIKRigDefinition* table_rig_ik[2] = {};
		{
			FString table_name[2] = {
				FString(TEXT("IK_")) + vrmAssetList->BaseFileName + TEXT("_Mannequin"),
				FString(TEXT("IK_UEFN_")) + vrmAssetList->BaseFileName + TEXT("_Mannequin"),
			};

			for (int ik_no=0; ik_no<2; ik_no++){
				FString name = table_name[ik_no];
				table_rig_ik[ik_no] = VRM4U_NewObject<UIKRigDefinition>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);
				auto rig_ik = table_rig_ik[ik_no];

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
					uint32_t mask = 0xFFFF;
				};

				TArray<FString> AddedChainList;

				TArray<TT> table = {
					{TEXT("Spine"),		TEXT("spine"),				TEXT("chest"),},
	#if	UE_VERSION_OLDER_THAN(5,4,0)
					{TEXT("Head"),		TEXT("neck"),				TEXT("head"),},
	#else
					{TEXT("Neck"),		TEXT("neck"),				TEXT("neck"),},
					{TEXT("Head"),		TEXT("head"),				TEXT("head"),},
	#endif
					{TEXT("RightArm"),	TEXT("rightUpperArm"),		TEXT("rightHand"),},
					{TEXT("LeftArm"),	TEXT("leftUpperArm"),		TEXT("leftHand"),},

					// 0
					{TEXT("RightLeg"),	TEXT("rightUpperLeg"),		TEXT("rightToes"),	0x01},
					{TEXT("LeftLeg"),	TEXT("leftUpperLeg"),		TEXT("leftToes"),	0x01},

					// 1
					{TEXT("RightLeg"),	TEXT("rightUpperLeg"),		TEXT("rightFoot"),	0x02},
					{TEXT("LeftLeg"),	TEXT("leftUpperLeg"),		TEXT("leftFoot"),	0x02},
					{TEXT("RightToe"),	TEXT("rightToes"),		TEXT("rightToes"),		0x02},
					{TEXT("LeftToe"),	TEXT("leftToes"),		TEXT("leftToes"),		0x02},

					{TEXT("LeftThumb"),		TEXT("leftThumbMetacarpal"),	TEXT("leftThumbDistal"),},	// vrm1 thumb
					{TEXT("LeftThumb"),		TEXT("leftThumbProximal"),		TEXT("leftThumbDistal"),},
					{TEXT("LeftIndex"),		TEXT("leftIndexProximal"),		TEXT("leftIndexDistal"),},
					{TEXT("LeftMiddle"),	TEXT("leftMiddleProximal"),	TEXT("leftMiddleDistal"),},
					{TEXT("LeftRing"),		TEXT("leftRingProximal"),		TEXT("leftRingDistal"),},
					{TEXT("LeftPinky"),		TEXT("leftLittleProximal"),	TEXT("leftLittleDistal"),},

					{TEXT("RightThumb"),	TEXT("rightThumbMetacarpal"),	TEXT("rightThumbDistal"),},	// vrm1 thumb
					{TEXT("RightThumb"),	TEXT("rightThumbProximal"),	TEXT("rightThumbDistal"),},
					{TEXT("RightIndex"),	TEXT("rightIndexProximal"),	TEXT("rightIndexDistal"),},
					{TEXT("RightMiddle"),	TEXT("rightMiddleProximal"),	TEXT("rightMiddleDistal"),},
					{TEXT("RightRing"),		TEXT("rightRingProximal"),		TEXT("rightRingDistal"),},
					{TEXT("RightPinky"),	TEXT("rightLittleProximal"),	TEXT("rightLittleDistal"),},

					{TEXT("LeftClavicle"),		TEXT("leftShoulder"),	TEXT("leftShoulder"),},
					{TEXT("RightClavicle"),		TEXT("rightShoulder"),	TEXT("rightShoulder"),},

					{TEXT("LeftHandIK"),		TEXT("ik_hand_l"),	TEXT("ik_hand_l"),},
					{TEXT("RightHandIK"),		TEXT("ik_hand_r"),	TEXT("ik_hand_r"),},
					{TEXT("HandGunIK"),			TEXT("ik_hand_gun"),	TEXT("ik_hand_gun"),},
					{TEXT("FootRootIK"),		TEXT("ik_foot_root"),	TEXT("ik_foot_root"),},
					{TEXT("HandRootIK"),		TEXT("ik_hand_root"),	TEXT("ik_hand_root"),},
				};

				TArray<TT> table_BVH = {
					{TEXT("Spine"),		TEXT("torso_1"),				TEXT("torso_7"),},
	#if	UE_VERSION_OLDER_THAN(5,4,0)
					{TEXT("Head"),		TEXT("neck_1"),				TEXT("head"),},
	#else
					{TEXT("Neck"),		TEXT("neck_1"),				TEXT("neck_2"),},
					{TEXT("Head"),		TEXT("head"),				TEXT("head"),},
	#endif
					{TEXT("RightArm"),	TEXT("r_up_arm"),		TEXT("r_hand"),},
					{TEXT("LeftArm"),	TEXT("l_up_arm"),		TEXT("l_hand"),},

					// 0
					{TEXT("RightLeg"),	TEXT("r_up_leg"),		TEXT("r_toes"),	0x01},
					{TEXT("LeftLeg"),	TEXT("l_up_leg"),		TEXT("l_toes"),	0x01},

					// 1
					{TEXT("RightLeg"),	TEXT("r_up_leg"),		TEXT("r_foot"),	0x02},
					{TEXT("LeftLeg"),	TEXT("l_up_leg"),		TEXT("l_foot"),	0x02},
					{TEXT("RightToe"),	TEXT("r_toes"),		TEXT("r_toes"),		0x02},
					{TEXT("LeftToe"),	TEXT("l_toes"),		TEXT("l_toes"),		0x02},

					{TEXT("LeftClavicle"),		TEXT("l_shoulder"),	TEXT("l_shoulder"),},
					{TEXT("RightClavicle"),		TEXT("r_shoulder"),	TEXT("r_shoulder"),},
				};

				// mocopi bvh  bone name
				if (Options::Get().IsBVHModel()) {
					table.Append(table_BVH);

					rigcon.SetRetargetRoot("root");
				}

				for (auto& t : table) {
					if ((t.mask & (1 << ik_no)) == 0) {
						continue;
					}
					if (AddedChainList.Contains(t.chain)) {
						continue;
					}
					if (t.chain == TEXT("LeftThumb") || (t.chain == TEXT("RightThumb"))) {
						bool b0 = (t.s1 == TEXT("leftThumbProximal"))  || (t.s1 == TEXT("rightThumbProximal"));
						bool b1 = (t.s1 == TEXT("leftThumbMetacarpal")) || (t.s1 == TEXT("rightThumbMetacarpal"));

						if (Options::Get().IsVRM10Model() && b0) {
							continue;
						}
						if (Options::Get().IsVRM0Model() && b1) {
							continue;
						}
					}

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
					if (conv.s1 == "" || conv.s2 == "") {
						// for ik bone
						if (VRMGetRefSkeleton(sk).FindBoneIndex(*t.s1) >= 0) {
							conv.s1 = t.s1;
						}
						if (VRMGetRefSkeleton(sk).FindBoneIndex(*t.s2) >= 0) {
							conv.s2 = t.s2;
						}
					}
					if (conv.s1 == "" || conv.s2 == "") continue;

					{
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
						AddedChainList.Add(t.chain);
					}
				}
				rigcon.LocalSolverSetup(vrmAssetList, ik_no);

				for (auto& modelName : vrmAssetList->VrmMetaObject->humanoidBoneTable) {
					if (modelName.Key == "" || modelName.Value == "") {
						continue;
					}
					if (modelName.Key == TEXT("hips")) {
						rigcon.SetRetargetRoot(*modelName.Value);
					}
				}
			} // ik_no
		} // skeleton ik


#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
		{
			struct RTGdata{
				FString NewAssetName;
				TArray<FString> BaseIKRigName;
				int ModelType;
			};

			RTGdata rtgDataTable[3];

			rtgDataTable[0].NewAssetName = FString(TEXT("RTG_")) + vrmAssetList->BaseFileName;
			rtgDataTable[1].NewAssetName = FString(TEXT("RTG_UE4_")) + vrmAssetList->BaseFileName;
			rtgDataTable[2].NewAssetName = FString(TEXT("RTG_UEFN_")) + vrmAssetList->BaseFileName;
			
			rtgDataTable[0].BaseIKRigName.Add(TEXT("/Game/Characters/Mannequins/Rigs/IK_Mannequin.IK_Mannequin"));
			rtgDataTable[0].BaseIKRigName.Add(TEXT("/Game/Characters/UE5_Mannequins/Rigs/IK_UE5_Mannequin_Retarget.IK_UE5_Mannequin_Retarget"));
			rtgDataTable[1].BaseIKRigName.Add(TEXT("/Game/Characters/Mannequin_UE4/Rigs/IK_UE4_Mannequin.IK_UE4_Mannequin"));
			rtgDataTable[1].BaseIKRigName.Add(TEXT("/Game/Characters/UE4_Mannequin/Rigs/IK_UE4_Mannequin_Retarget.IK_UE4_Mannequin_Retarget"));
			rtgDataTable[2].BaseIKRigName.Add(TEXT("/Game/Characters/UEFN_Mannequin/Rigs/IK_UEFN_Mannequin.IK_UEFN_Mannequin"));

			rtgDataTable[0].ModelType = 0;	// mannnequin
			rtgDataTable[1].ModelType = 0;	// mannnequin
			rtgDataTable[2].ModelType = 1;	// UEFN mannnequin

			for (int ikr_no = 0; ikr_no < 3; ikr_no++) {

				RTGdata& rtgData = rtgDataTable[ikr_no];

				auto *vrm_ikrig = table_rig_ik[rtgData.ModelType];

				UObject* uobjectIK = nullptr;

				for (auto rigName : rtgData.BaseIKRigName) {
					FSoftObjectPath r(rigName);
					uobjectIK = r.TryLoad();
					if (uobjectIK) {
						break;
					}
				}
				if (uobjectIK == nullptr) {
					if (ikr_no == 0) {
#if WITH_EDITOR
#if UE_VERSION_OLDER_THAN(5,6,0)
#else
						uobjectIK = GenerateMannequinIK(vrmAssetList);
#endif
#endif
					}
				}

				if (uobjectIK == nullptr) {
					// ベースのIKRigが無ければスキップ
					continue;
				}

				FString name = rtgData.NewAssetName;
				UIKRetargeter* ikr = VRM4U_NewObject<UIKRetargeter>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);

#if WITH_EDITOR
				ikr->TargetMeshOffset.Set(100, 0, 0);
#endif

				auto SourceOrTargetVRM = ERetargetSourceOrTarget::Target;
				auto SourceOrTargetMannequin = ERetargetSourceOrTarget::Source;

				if (Options::Get().IsVRMAModel() || Options::Get().IsBVHModel()) {
					SourceOrTargetVRM = ERetargetSourceOrTarget::Source;
					SourceOrTargetMannequin = ERetargetSourceOrTarget::Target;
				}

				SimpleRetargeterController c = SimpleRetargeterController(ikr);
				c.SetIKRig(SourceOrTargetVRM, vrm_ikrig);

				if (uobjectIK) {
					auto r2 = Cast<UIKRigDefinition>(uobjectIK);
					if (r2) {
						c.SetIKRig(SourceOrTargetMannequin, r2);
					}
				}

				if (vrmAssetList && vrmAssetList->VrmMetaObject) {

					auto& humanoidBoneTable = vrmAssetList->VrmMetaObject->humanoidBoneTable;
					TArray<FString> KeyArray;
					TArray<FString> ValueArray;
					humanoidBoneTable.GenerateKeyArray(KeyArray);
					humanoidBoneTable.GenerateValueArray(ValueArray);

					auto FindKeyByValue = [humanoidBoneTable](const FString& SearchValue)
						{
							for (const auto& Pair : humanoidBoneTable)
							{
								if (Pair.Value.Compare(SearchValue, ESearchCase::IgnoreCase) == 0)
								{
									return Pair.Key;
								}
							}
							return FString("");
						};

					VRMRetargetData retargetData;
					retargetData.Setup(vrmAssetList,
						VRMConverter::Options::Get().IsVRMModel(),
						VRMConverter::Options::Get().IsBVHModel(),
						VRMConverter::Options::Get().IsPMXModel());

					auto FindRotData = [retargetData](const FString& boneVRM, FRotator& rot) {
						for (auto& a : retargetData.retargetTable) {
							if (a.BoneVRM.Compare(boneVRM, ESearchCase::IgnoreCase)) continue;

							rot = a.rot;
							return true;
						}
						return false;
						};

					{
						//name
						FName PoseName = "POSE_A";
						const FName NewPoseName = c.CreateRetargetPose(PoseName, SourceOrTargetVRM);
						FIKRetargetPose* NewPose = c.GetRetargetPosesByName(SourceOrTargetVRM, NewPoseName);
						if (NewPose == nullptr) continue;

						FReferenceSkeleton& RefSkeleton = sk->GetRefSkeleton();
						const TArray<FTransform>& RefPose = RefSkeleton.GetRefBonePose();
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,6,0)
						const FName RetargetRootBoneName = vrm_ikrig->GetRetargetRoot();
#else
						auto rigc = UIKRigController::GetController(vrm_ikrig);
						const FName RetargetRootBoneName = rigc->GetRetargetRoot();
#endif
#else
						const FName RetargetRootBoneName = "";
#endif
						for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
						{
							auto BoneName = RefSkeleton.GetBoneName(BoneIndex);

							auto str = FindKeyByValue(BoneName.ToString());
							if (str == "") {
								continue;
							}
							FRotator rot;
							if (FindRotData(*str, rot) == false) {
								continue;
							}


							// record a global space translation offset for the root bone
							if (BoneName == RetargetRootBoneName)
							{
								//	FTransform GlobalRefTransform = FAnimationRuntime::GetComponentSpaceTransform(RefSkeleton, RefPose, BoneIndex);
								//	FTransform GlobalImportedTransform = PoseAsset->GetComponentSpaceTransform(BoneName, LocalBoneTransformFromPose);
								//	NewPose.SetRootTranslationDelta(GlobalImportedTransform.GetLocation() - GlobalRefTransform.GetLocation());
							}

							// record a local space delta rotation (if there is one)

							const FTransform& LocalRefTransform = RefPose[BoneIndex];
							const FTransform& LocalImportedTransform = FTransform(rot, FVector(0, 0, 0), FVector(1, 1, 1));
							//const FQuat DeltaRotation = LocalRefTransform.GetRotation().Inverse() * LocalImportedTransform.GetRotation();

							FQuat DeltaRotation = FQuat(rot);
							if (VRMConverter::Options::Get().IsVRM10Model()) {

								FTransform dstTrans;
								auto dstIndex = BoneIndex;
								while (dstIndex >= 0)
								{
									dstTrans = RefSkeleton.GetRefBonePose()[dstIndex].GetRelativeTransform(dstTrans);
									dstIndex = RefSkeleton.GetParentIndex(dstIndex);
									if (dstIndex < 0) {
										break;
									}
								}

								//DeltaRotation = LocalRefTransform.GetRotation().Inverse() * DeltaRotation * LocalRefTransform.GetRotation();
								//DeltaRotation = LocalRefTransform.GetRotation() * DeltaRotation * LocalRefTransform.GetRotation().Inverse();
								//DeltaRotation = dstTrans.GetRotation() * DeltaRotation * dstTrans.GetRotation().Inverse();
								DeltaRotation = dstTrans.GetRotation().Inverse() * DeltaRotation * dstTrans.GetRotation();
							}

							const float DeltaAngle = FMath::RadiansToDegrees(DeltaRotation.GetAngle());
							constexpr float MinAngleThreshold = 0.05f;
							if (DeltaAngle >= MinAngleThreshold)
							{
								NewPose->SetDeltaRotationForBone(BoneName, DeltaRotation);
#if UE_VERSION_OLDER_THAN(5,4,0)
								NewPose->SortHierarchically(ikr->GetTargetIKRig()->GetSkeleton());
#else
								NewPose->SortHierarchically(ikr->GetIKRig(SourceOrTargetVRM)->GetSkeleton());
#endif
							}
						}
					}

#if VRM4U_USE_AUTOALIGN

#if UE_VERSION_OLDER_THAN(5,6,0)
#else
					{
						UIKRetargeterController* cp = UIKRetargeterController::GetController(ikr);
						//UIKRetargeterController& c = *cp;
						cp->AddDefaultOps();
					}
#endif
					c.SetCurrentRetargetPose(UIKRetargeter::GetDefaultPoseName(), SourceOrTargetVRM);
					c.SetCurrentRetargetPose(UIKRetargeter::GetDefaultPoseName(), SourceOrTargetMannequin);

					// 自動で姿勢を作る。

					if (VRMConverter::Options::Get().IsBVHModel()) {
						// bvhは bvh側をalignする
						c.AutoAlignAllBones(SourceOrTargetVRM);
					}else{
						// 足首はそのまま。自動設定がうまく動作しないため。
						c.AutoAlignAllBones(SourceOrTargetMannequin);
						c.SetRotationOffsetForRetargetPoseBone(TEXT("foot_l"), FQuat::Identity, SourceOrTargetMannequin);
						c.SetRotationOffsetForRetargetPoseBone(TEXT("foot_r"), FQuat::Identity, SourceOrTargetMannequin);
					}
#endif
				}
				c.SetChainSetting();
			}
		}
#endif // 5.2
#endif
	} // ikrig

	return true;

}

