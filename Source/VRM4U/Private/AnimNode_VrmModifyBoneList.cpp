// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "AnimNode_VrmModifyBoneList.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone


FAnimNode_VrmModifyBoneList::FAnimNode_VrmModifyBoneList()
{
}

//void FAnimNode_VrmModifyBoneList::Update_AnyThread(const FAnimationUpdateContext& Context) {
//	Super::Update_AnyThread(Context);
//	//Context.GetDeltaTime();
//}

void FAnimNode_VrmModifyBoneList::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
	Super::Initialize_AnyThread(Context);
}
void FAnimNode_VrmModifyBoneList::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
	Super::CacheBones_AnyThread(Context);
}


void FAnimNode_VrmModifyBoneList::UpdateInternal(const FAnimationUpdateContext& Context){
	Super::UpdateInternal(Context);
}


void FAnimNode_VrmModifyBoneList::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmModifyBoneList::EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output) {
	Super::EvaluateComponentPose_AnyThread(Output);
}

void FAnimNode_VrmModifyBoneList::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
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
		for (const auto &t : BoneTrans) {

			int index = RefSkeleton.FindBoneIndex(*t.Key);
			if (index < 0) continue;

			FBoneTransform f(FCompactPoseBoneIndex(index), t.Value);

			FVector v = RefSkeletonTransform[index].GetLocation();
			//f.Transform.SetTranslation(v);

			//f.Transform.SetTranslation(RefSkeletonTransform[index].GetLocation());
			tmpOutTransform.Add(f);
			boneIndexTable.Add(index);
		}

		// bone hierarchy
		// start with 1
		for (int i = 0; i < tmpOutTransform.Num(); ++i) {
			auto& a = tmpOutTransform[i];
			int parentBoneIndex = RefSkeleton.GetParentIndex(boneIndexTable[i]);
			if (parentBoneIndex < 0) continue;

			int parentInTable = boneIndexTable.Find(parentBoneIndex);

			for (int j = 0; j < 100; ++j) {
				if (parentInTable >= 0) {
					// find in table
					break;
				}
				if (parentBoneIndex < 0) {
					// root bone
					break;
				}

				// add outtransform with ref bone
				//FBoneTransform f(FCompactPoseBoneIndex(parentBoneIndex), RefSkeletonTransform[parentBoneIndex]);

				// add outtransform with input bone
				FCompactPoseBoneIndex CompactPoseBoneToModify(parentBoneIndex);
				FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
				FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, EBoneControlSpace::BCS_ParentBoneSpace);

				FBoneTransform f(CompactPoseBoneToModify, NewBoneTM);


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
		if (parentInHandTable < 0) {
			// root
			auto BoneSpace = EBoneControlSpace::BCS_ParentBoneSpace;
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, a.Transform, a.BoneIndex, BoneSpace);
		}else{
			bool bUseInputTrans = false;

#if	UE_VERSION_OLDER_THAN(4,27,0)
#else
			FString t = RefSkeleton.GetBoneName(boneIndexTable[i]).ToString();

			auto ret = UseInputTransBoneList.FilterByPredicate([&t](FString a) {
				return t.Compare(a, ESearchCase::IgnoreCase) == 0;
			}
			);
			if (ret.Num() > 0) {
				bUseInputTrans = true;
			}
#endif
			if (bUseInputTrans) {
				FCompactPoseBoneIndex CompactPoseBoneToModify(boneIndexTable[i]);
				FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
				FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, EBoneControlSpace::BCS_ParentBoneSpace);
				a.Transform = NewBoneTM * tmpOutTransform[parentInHandTable].Transform;
			} else {
				a.Transform *= tmpOutTransform[parentInHandTable].Transform;
			}
		}
		OutBoneTransforms.Add(a);
	}

}

bool FAnimNode_VrmModifyBoneList::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmModifyBoneList::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}

void FAnimNode_VrmModifyBoneList::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp, bool bPreviewForeground) const
{
#if WITH_EDITOR

	if (PreviewSkelMeshComp->GetWorld() == nullptr) {
		return;
	}

	ESceneDepthPriorityGroup Priority = SDPG_World;
	if (bPreviewForeground) Priority = SDPG_Foreground;

#endif
}
