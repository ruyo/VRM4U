// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "AnimNode_VrmModifyBoneListRetarget.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone


FAnimNode_VrmModifyBoneListRetarget::FAnimNode_VrmModifyBoneListRetarget()
{
}

//void FAnimNode_VrmModifyBoneListRetarget::Update_AnyThread(const FAnimationUpdateContext& Context) {
//	Super::Update_AnyThread(Context);
//	//Context.GetDeltaTime();
//}

void FAnimNode_VrmModifyBoneListRetarget::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
	Super::Initialize_AnyThread(Context);
}
void FAnimNode_VrmModifyBoneListRetarget::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
	Super::CacheBones_AnyThread(Context);
}


void FAnimNode_VrmModifyBoneListRetarget::UpdateInternal(const FAnimationUpdateContext& Context){
	Super::UpdateInternal(Context);
}


void FAnimNode_VrmModifyBoneListRetarget::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmModifyBoneListRetarget::EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output) {
	Super::EvaluateComponentPose_AnyThread(Output);
}

void FAnimNode_VrmModifyBoneListRetarget::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	const auto Skeleton = Output.AnimInstanceProxy->GetSkeleton();
	const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
	const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	const auto& RefSkeletonTransform = Output.Pose.GetPose().GetBoneContainer().GetRefPoseArray();

	TArray<int> boneIndexTable;
	TArray<FBoneTransform> tmpOutTransform;

	//dstRefSkeleton.GetParentIndex

	{

		if (VrmMetaObject == nullptr) {
			return;
		}

		bool bFirstBone = true;

		for (const auto &t : VrmMetaObject->humanoidBoneTable) {
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
			} else {
				FVector v = RefSkeletonTransform[index].GetLocation();
				f.Transform.SetTranslation(v);
			}

			//f.Transform.SetTranslation(RefSkeletonTransform[index].GetLocation());
			tmpOutTransform.Add(f);
			boneIndexTable.Add(index);
		}

		// bone hierarchy
		// start with 1
		for (int i = 1; i < tmpOutTransform.Num(); ++i) {
			auto& a = tmpOutTransform[i];
			int parentBoneIndex = RefSkeleton.GetParentIndex(boneIndexTable[i]);
			int parentInTable = boneIndexTable.Find(parentBoneIndex);

			for (int j = 0; j < 100; ++j) {
				if (parentInTable >= 0) {
					break;
				}

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

bool FAnimNode_VrmModifyBoneListRetarget::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmModifyBoneListRetarget::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}

void FAnimNode_VrmModifyBoneListRetarget::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp, bool bPreviewForeground) const
{
#if WITH_EDITOR

	if (VrmMetaObject == nullptr || PreviewSkelMeshComp == nullptr) {
		return;
	}
	if (PreviewSkelMeshComp->GetWorld() == nullptr) {
		return;
	}

	ESceneDepthPriorityGroup Priority = SDPG_World;
	if (bPreviewForeground) Priority = SDPG_Foreground;

#endif
}
