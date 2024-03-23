// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "AnimNode_VrmModifyHumanoidBone.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"

#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "VrmUtil.h"

/////////////////////////////////////////////////////
// FAnimNode_ModifyBone

FAnimNode_VrmModifyHumanoidBone::FAnimNode_VrmModifyHumanoidBone()
	:
	//TranslationMode(BMM_Ignore)
	//, RotationMode(BMM_Ignore)
	//, TranslationSpace(BCS_ComponentSpace)
	RotationSpace(BCS_WorldSpace)
{
}

void FAnimNode_VrmModifyHumanoidBone::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmModifyHumanoidBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	// the way we apply transform is same as FMatrix or FTransform
	// we apply scale first, and rotation, and translation
	// if you'd like to translate first, you'll need two nodes that first node does translate and second nodes to rotate.
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	if (VrmAssetList == nullptr) {
		return;
	}

	const auto &Transform_hum = HumanoidBoneTransform.Transform;
	const auto &Transform_ext = ExtBoneTransform.Transform;

	const auto &a = VRMUtil::vrm_humanoid_bone_list;
	const auto &table = VrmAssetList->VrmMetaObject->humanoidBoneTable;

	int boneTableNo_hum = 0;
	int boneTableNo_ext = 0;

	while (boneTableNo_hum <= a.Num()) {
		int currentBone = -1;

		FTransform trans;
		int boneIndex[2] = {-1, -1};

		if (boneTableNo_hum < Transform_hum.Num() && boneTableNo_hum<a.Num()) {
			auto boneName = table.Find(a[boneTableNo_hum]);
			if (boneName) {
				boneIndex[0] = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(**boneName);
			}
		}
		if (boneTableNo_ext < Transform_ext.Num()) {
			boneIndex[1] = boneTableNo_ext;
		}

		if (boneIndex[0] < 0 && boneIndex[1] < 0) {
			// no bone
			++boneTableNo_hum;
			continue;
			//break;
		}

		if (boneIndex[0] == boneIndex[1]) {
			currentBone = boneIndex[0];
			trans = Transform_hum[boneTableNo_hum];
			++boneTableNo_hum;
			++boneTableNo_ext;

		} else {
			if (boneIndex[0] < 0) {
				// find ext
				currentBone = boneIndex[1];
				trans = Transform_ext[boneTableNo_ext];
				++boneTableNo_ext;
			} else if (boneIndex[1] < 0) {
				// find hum
				currentBone = boneIndex[0];
				trans = Transform_hum[boneTableNo_hum];
				++boneTableNo_hum;
			} else {
				if (boneIndex[0] < boneIndex[1]) {
					// find hum
					currentBone = boneIndex[0];
					trans = Transform_hum[boneTableNo_hum];
					++boneTableNo_hum;
				} else {
					// find ext
					currentBone = boneIndex[1];
					trans = Transform_ext[boneTableNo_ext];
					++boneTableNo_ext;
				}
			}
		}

		const FCompactPoseBoneIndex CompactPoseBoneToModify(currentBone);
		FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
		const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
		const FTransform orgTM = NewBoneTM;

		// Convert to Bone Space.
		FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, RotationSpace);

		//const FQuat BoneQuat(Rotation);
		const FQuat BoneQuat = trans.GetRotation();
		//if (RotationMode == BMM_Additive)
		//{
			//NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
		//} else
		//{
		NewBoneTM.SetRotation(BoneQuat);
		//} 

		// Convert back to Component Space.
		FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, RotationSpace);

		{
			TArray<FBoneTransform> bt;

			bt.Add(FBoneTransform(CompactPoseBoneToModify, NewBoneTM));

			const float BlendWeight = FMath::Clamp<float>(ActualAlpha, 0.f, 1.f);
			Output.Pose.LocalBlendCSBoneTransforms(bt, BlendWeight);
		}
		//OutBoneTransforms.Add(FBoneTransform(CompactPoseBoneToModify, NewBoneTM));

	}

	if (0) {
		for (int boneNo = 0; boneNo < Transform_hum.Num(); ++boneNo) {
			if (boneNo >= a.Num()) {
				continue;
			}
			auto boneName = table.Find(a[boneNo]);

			if (boneName == nullptr) {
				continue;
			}

			int32 boneIndex = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(**boneName);

			if (boneIndex < 0) {
				continue;
			}
			auto rot = Transform_hum[boneNo].GetRotation();



			const FCompactPoseBoneIndex CompactPoseBoneToModify(boneIndex);
			FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
			const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
			const FTransform orgTM = NewBoneTM;

			// Convert to Bone Space.
			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, RotationSpace);

			//const FQuat BoneQuat(Rotation);
			const FQuat BoneQuat = rot;
			//if (RotationMode == BMM_Additive)
			//{
				//NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
			//} else
			//{
			NewBoneTM.SetRotation(BoneQuat);
			//} 

			// Convert back to Component Space.
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, RotationSpace);

			{
				TArray<FBoneTransform> bt;

				bt.Add(FBoneTransform(CompactPoseBoneToModify, NewBoneTM));

				const float BlendWeight = FMath::Clamp<float>(ActualAlpha, 0.f, 1.f);
				Output.Pose.LocalBlendCSBoneTransforms(bt, BlendWeight);
			}
			//OutBoneTransforms.Add(FBoneTransform(CompactPoseBoneToModify, NewBoneTM));
		}
	}

	//OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

bool FAnimNode_VrmModifyHumanoidBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmModifyHumanoidBone::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}
