// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "AnimNode_VrmVMC.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

#include "VRM4U_VMCSubsystem.h"
#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone


FAnimNode_VrmVMC::FAnimNode_VrmVMC()
{
}


FAnimNode_VrmVMC::~FAnimNode_VrmVMC()
{
}


//void FAnimNode_VrmVMC::Update_AnyThread(const FAnimationUpdateContext& Context) {
//	Super::Update_AnyThread(Context);
//	//Context.GetDeltaTime();
//}

void FAnimNode_VrmVMC::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
	Super::Initialize_AnyThread(Context);

	VrmMetaObject_Internal = VrmMetaObject;
	if (VrmMetaObject_Internal == nullptr && EnableAutoSearchMetaData) {
		VrmAssetListObject_Internal = VRMUtil::GetAssetListObject(VRMGetSkinnedAsset(Context.AnimInstanceProxy->GetSkelMeshComponent()));
		if (VrmAssetListObject_Internal) {
			VrmMetaObject_Internal = VrmAssetListObject_Internal->VrmMetaObject;
		}
	}

	UVRM4U_VMCSubsystem* subsystem = GEngine->GetEngineSubsystem<UVRM4U_VMCSubsystem>();
	if (subsystem == nullptr) return;
	{
		auto *s = subsystem->FindOrAddServer(ServerAddress, Port);
		if (s) {
			s->bForceUpdate = bForceUpdate;
		}
	}
	bCreateServer = true;

	// init global reftransform
	{
		const auto Skeleton = Context.AnimInstanceProxy->GetSkeleton();
		const auto RefSkeleton = Skeleton->GetReferenceSkeleton();
		const auto& RefSkeletonTransform = RefSkeleton.GetRefBonePose();

		RefSkeletonTransform_global = RefSkeletonTransform;
		{
			auto& g = RefSkeletonTransform_global;
			for (int i = 0; i < RefSkeletonTransform.Num(); ++i) {
				int parent = RefSkeleton.GetParentIndex(i);
				if (parent < 0) continue;
				g[i] = g[i] * g[parent];
			}
		}

	}
}
void FAnimNode_VrmVMC::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
	Super::CacheBones_AnyThread(Context);
}


void FAnimNode_VrmVMC::UpdateInternal(const FAnimationUpdateContext& Context){
	Super::UpdateInternal(Context);
}


void FAnimNode_VrmVMC::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmVMC::EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output) {
	Super::EvaluateComponentPose_AnyThread(Output);
}

void FAnimNode_VrmVMC::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	if (VrmMetaObject_Internal == nullptr) {
		return;
	}

	UVRM4U_VMCSubsystem* subsystem = GEngine->GetEngineSubsystem<UVRM4U_VMCSubsystem>();
	if (subsystem == nullptr) return;

	const auto Skeleton = Output.AnimInstanceProxy->GetSkeleton();
	const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
	const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	const auto& RefSkeletonTransform = RefSkeleton.GetRefBonePose();

	if (RefSkeletonTransform_global.Num() != RefSkeletonTransform.Num()) {
		return;
	}

	TArray<int> boneIndexTable;
	TArray<FBoneTransform> tmpOutTransform;

	FVMCData VMCData;
	if (subsystem->CopyVMCData(VMCData, ServerAddress, Port) == false) {
		return;
	}

	if (VMCData.BoneData.Num() == 0 && VMCData.CurveData.Num() == 0) {
		return;
	}
	if (bApplyPerfectSync) {
		for (auto& c : VMCData.CurveData) {
			if (c.Key.Contains(TEXT("BlendShape.")) == false) continue;

			c.Key.RightChopInline(11); // [blendahape.]
		}
	}

	TMap<FString, FTransform>& BoneTrans = VMCData.BoneData;

	const auto &MorphList = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetSkinnedAsset()->GetMorphTargets();
	for (auto& c : VMCData.CurveData) {
#if	UE_VERSION_OLDER_THAN(5,3,0)
		{
			SmartName::UID_Type NewUID;
			FName NewName = *c.Key;

			NewUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, NewName);

			Output.Curve.Set(NewUID, c.Value);
		}
#else
		auto m = MorphList.FindByPredicate([&c](const TObjectPtr<UMorphTarget> &m) {
			FString s = c.Key;

			if (m->GetName().Compare(s, ESearchCase::IgnoreCase)) {
				return false;
			}
			return true;
		});
		if (m) {
			Output.Curve.Set(*m->GetName(), c.Value);
		} else {
			Output.Curve.Set(*c.Key, c.Value);
		}
#endif
	}

	{
		bool bFirstBone = true;

		for (const auto &t : VrmMetaObject_Internal->humanoidBoneTable) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
			auto *tmpVal = BoneTrans.Find(t.Key.ToLower());
			if (tmpVal == nullptr) continue;

			auto modelBone = *tmpVal;
#else
			auto filterList= BoneTrans.FilterByPredicate([&t](TPair<FString, FTransform> a) {
				return a.Key.Compare(t.Key, ESearchCase::IgnoreCase) == 0;
			}
			);
			if (filterList.Num() != 1) continue;
			auto modelBone = filterList.begin()->Value;
#endif


			int index = RefSkeleton.FindBoneIndex(*t.Value);
			if (index < 0) continue;

			FBoneTransform f(FCompactPoseBoneIndex(index), modelBone);
			//f.Transform.SetRotation(FQuat::Identity);

			if (bFirstBone) {
				bFirstBone = false;

				if (bUseRemoteCenterPos) {
					auto v = f.Transform.GetLocation() * ModelRelativeScale;
					f.Transform.SetTranslation(v);
				} else {
					auto v = RefSkeletonTransform[index].GetLocation();
					f.Transform.SetTranslation(v);
				}


				// root bone
				FTransform RootTrans;
				for (auto& a : BoneTrans) {
					if (a.Key.Compare(TEXT("root"), ESearchCase::IgnoreCase)) {
						continue;
					}
					RootTrans = a.Value;
					break;
				}
				if (index == 0) {
					// hip == root
					f.Transform.SetTranslation(f.Transform.GetLocation() + RootTrans.GetLocation());
				} else {
					// orig root
					FBoneTransform bt(FCompactPoseBoneIndex(0), RootTrans);
					tmpOutTransform.Add(bt);
					boneIndexTable.Add(0);
				}

			} else {
				FVector v = RefSkeletonTransform[index].GetLocation();
				f.Transform.SetTranslation(v);
			}

			if (bIgnoreLocalRotation){
				auto r_refg = RefSkeletonTransform_global[index].GetRotation();
				auto r_ref = RefSkeletonTransform[index].GetRotation();
				auto r_vmc = f.Transform.GetRotation();

				auto r_dif = r_refg.Inverse() * r_vmc * r_refg;
				
				f.Transform.SetRotation(r_ref * r_dif);
			}
			//f.Transform.SetTranslation(RefSkeletonTransform[index].GetLocation());
			tmpOutTransform.Add(f);
			boneIndexTable.Add(index);
		}

		// bone hierarchy
		for (int i = 1; i < tmpOutTransform.Num(); ++i) {
			int parentBoneIndex = RefSkeleton.GetParentIndex(boneIndexTable[i]);
			int parentInTable = boneIndexTable.Find(parentBoneIndex);

			for (int j = 0; j < 1000; ++j) {
				if (parentInTable >= 0) {
					break;
				}
				if (parentBoneIndex < 0) break;

				// add outtransform with ref bone
				FBoneTransform f(FCompactPoseBoneIndex(parentBoneIndex), RefSkeletonTransform[parentBoneIndex]);
				tmpOutTransform.Add(f);
				boneIndexTable.Add(parentBoneIndex);

				parentBoneIndex = RefSkeleton.GetParentIndex(parentBoneIndex);
				parentInTable = boneIndexTable.Find(parentBoneIndex);
			}
		}

		// sort
		tmpOutTransform.Sort(FCompareBoneTransformIndex());
		boneIndexTable.Sort();
	}

	for (int i = 0; i < tmpOutTransform.Num(); ++i) {
		auto& a = tmpOutTransform[i];

		int parentBoneIndex = RefSkeleton.GetParentIndex(boneIndexTable[i]);

		int parentInHandTable = boneIndexTable.Find(parentBoneIndex);
		if (parentInHandTable >= 0) {
			a.Transform *= tmpOutTransform[parentInHandTable].Transform;
		} else {
			// root
			auto BoneSpace = EBoneControlSpace::BCS_ParentBoneSpace;
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, a.Transform, a.BoneIndex, BoneSpace);
		}
		OutBoneTransforms.Add(a);
	}

}

bool FAnimNode_VrmVMC::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmVMC::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}

void FAnimNode_VrmVMC::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp, bool bPreviewForeground) const
{
#if WITH_EDITOR

	if (VrmMetaObject_Internal == nullptr || PreviewSkelMeshComp == nullptr) {
		return;
	}
	if (PreviewSkelMeshComp->GetWorld() == nullptr) {
		return;
	}

	ESceneDepthPriorityGroup Priority = SDPG_World;
	if (bPreviewForeground) Priority = SDPG_Foreground;

#endif
}
