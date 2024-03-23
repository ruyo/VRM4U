// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.



#include "AnimNode_VrmQuestHandBone.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "BonePose.h"

#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone

namespace {

	/*
typedef enum ovrpBoneId_ {
  ovrpBoneId_Invalid                 = -1,
  ovrpBoneId_Hand_Start              = 0,
  ovrpBoneId_Hand_WristRoot          = ovrpBoneId_Hand_Start + 0, // root frame of the hand, where the wrist is located
  ovrpBoneId_Hand_ForearmStub        = ovrpBoneId_Hand_Start + 1, // frame for user's forearm
  ovrpBoneId_Hand_Thumb0             = ovrpBoneId_Hand_Start + 2, // thumb trapezium bone
  ovrpBoneId_Hand_Thumb1             = ovrpBoneId_Hand_Start + 3, // thumb metacarpal bone
  ovrpBoneId_Hand_Thumb2             = ovrpBoneId_Hand_Start + 4, // thumb proximal phalange bone
  ovrpBoneId_Hand_Thumb3             = ovrpBoneId_Hand_Start + 5, // thumb distal phalange bone
  ovrpBoneId_Hand_Index1             = ovrpBoneId_Hand_Start + 6, // index proximal phalange bone
  ovrpBoneId_Hand_Index2             = ovrpBoneId_Hand_Start + 7, // index intermediate phalange bone
  ovrpBoneId_Hand_Index3             = ovrpBoneId_Hand_Start + 8, // index distal phalange bone
  ovrpBoneId_Hand_Middle1            = ovrpBoneId_Hand_Start + 9, // middle proximal phalange bone
  ovrpBoneId_Hand_Middle2            = ovrpBoneId_Hand_Start + 10, // middle intermediate phalange bone
  ovrpBoneId_Hand_Middle3            = ovrpBoneId_Hand_Start + 11, // middle distal phalange bone
  ovrpBoneId_Hand_Ring1              = ovrpBoneId_Hand_Start + 12, // ring proximal phalange bone
  ovrpBoneId_Hand_Ring2              = ovrpBoneId_Hand_Start + 13, // ring intermediate phalange bone
  ovrpBoneId_Hand_Ring3              = ovrpBoneId_Hand_Start + 14, // ring distal phalange bone
  ovrpBoneId_Hand_Pinky0             = ovrpBoneId_Hand_Start + 15, // pinky metacarpal bone
  ovrpBoneId_Hand_Pinky1             = ovrpBoneId_Hand_Start + 16, // pinky proximal phalange bone
  ovrpBoneId_Hand_Pinky2             = ovrpBoneId_Hand_Start + 17, // pinky intermediate phalange bone
  ovrpBoneId_Hand_Pinky3             = ovrpBoneId_Hand_Start + 18, // pinky distal phalange bone
  ovrpBoneId_Hand_MaxSkinnable       = ovrpBoneId_Hand_Start + 19,
  // Bone tips are position only. They are not used for skinning but useful for hit-testing.
  // NOTE: ovrBoneId_Hand_ThumbTip == ovrBoneId_Hand_MaxSkinnable since the extended tips need to be contiguous
  ovrpBoneId_Hand_ThumbTip           = ovrpBoneId_Hand_Start + ovrpBoneId_Hand_MaxSkinnable + 0, // tip of the thumb
  ovrpBoneId_Hand_IndexTip           = ovrpBoneId_Hand_Start + ovrpBoneId_Hand_MaxSkinnable + 1, // tip of the index finger
  ovrpBoneId_Hand_MiddleTip          = ovrpBoneId_Hand_Start + ovrpBoneId_Hand_MaxSkinnable + 2, // tip of the middle finger
  ovrpBoneId_Hand_RingTip            = ovrpBoneId_Hand_Start + ovrpBoneId_Hand_MaxSkinnable + 3, // tip of the ring finger
  ovrpBoneId_Hand_PinkyTip           = ovrpBoneId_Hand_Start + ovrpBoneId_Hand_MaxSkinnable + 4, // tip of the pinky
  ovrpBoneId_Hand_End                = ovrpBoneId_Hand_Start + ovrpBoneId_Hand_MaxSkinnable + 5,

  // add other skeleton bone definitions here...

  ovrpBoneId_Max                     = ovrpBoneId_Hand_End,
  ovrpBoneId_EnumSize = 0x7fff
} ovrpBoneId;

	*/

	const TArray<VRMUtil::VRMBoneTable> handtable_l_ue4_vrm = {
		{"Hand_L","leftHand"},	// +0
		{"lowerarm_l","----"},	//{"lowerarm_l","leftLowerArm"},
		{"thumb_03_l","----"},
		{"thumb_01_l","leftThumbProximal"},
		{"thumb_02_l","leftThumbIntermediate"},
		{"thumb_03_l","leftThumbDistal"},
		{"index_01_l","leftIndexProximal"},	// +6
		{"index_02_l","leftIndexIntermediate"},
		{"index_03_l","leftIndexDistal"},
		{"middle_01_l","leftMiddleProximal"},
		{"middle_02_l","leftMiddleIntermediate"},
		{"middle_03_l","leftMiddleDistal"},
		{"ring_01_l","leftRingProximal"},
		{"ring_02_l","leftRingIntermediate"},
		{"ring_03_l","leftRingDistal"},
		{"pinky_03_l","----"},
		{"pinky_01_l","leftLittleProximal"},
		{"pinky_02_l","leftLittleIntermediate"},
		{"pinky_03_l","leftLittleDistal"},
	};


	const TArray<VRMUtil::VRMBoneTable> handtable_r_ue4_vrm = {
		{"Hand_R", "rightHand"},	// +0
		{"lowerarm_r","----"},	//{"lowerarm_r","rightLowerArm"},
		{"lowerarm_r","----"},
		{ "thumb_01_r","rightThumbProximal" },
		{ "thumb_02_r","rightThumbIntermediate" },
		{ "thumb_03_r","rightThumbDistal" },
		{ "index_01_r","rightIndexProximal" }, // +6
		{ "index_02_r","rightIndexIntermediate" },
		{ "index_03_r","rightIndexDistal" },
		{ "middle_01_r","rightMiddleProximal" },
		{ "middle_02_r","rightMiddleIntermediate" },
		{ "middle_03_r","rightMiddleDistal" },
		{ "ring_01_r","rightRingProximal" },
		{ "ring_02_r","rightRingIntermediate" },
		{ "ring_03_r","rightRingDistal" },
		{ "pinky_01_r","----" },
		{ "pinky_01_r","rightLittleProximal" },
		{ "pinky_02_r","rightLittleIntermediate" },
		{ "pinky_03_r","rightLittleDistal" },
	};


	/*
	{"lowerarm_twist_01_l",""},
	{"upperarm_twist_01_l",""},
	{"clavicle_r","rightShoulder"},
	{"UpperArm_R","rightUpperArm"},
	{"lowerarm_r","rightLowerArm"},
	{"Hand_R","rightHand"},
	{"index_01_r","rightIndexProximal"},
	{"index_02_r","rightIndexIntermediate"},
	{"index_03_r","rightIndexDistal"},
	{"middle_01_r","rightMiddleProximal"},
	{"middle_02_r","rightMiddleIntermediate"},
	{"middle_03_r","rightMiddleDistal"},
	{"pinky_01_r","rightLittleProximal"},
	{"pinky_02_r","rightLittleIntermediate"},
	{"pinky_03_r","rightLittleDistal"},
	{"ring_01_r","rightRingProximal"},
	{"ring_02_r","rightRingIntermediate"},
	{"ring_03_r","rightRingDistal"},
	{"thumb_01_r","rightThumbProximal"},
	{"thumb_02_r","rightThumbIntermediate"},
	{"thumb_03_r","rightThumbDistal"},
	{"lowerarm_twist_01_r",""},
	{"upperarm_twist_01_r",""},
	{"neck_01","neck"},
	{"head","head"},
	{"Thigh_L","leftUpperLeg"},
	{"calf_l","leftLowerLeg"},
	{"calf_twist_01_l",""},
	{"Foot_L","leftFoot"},
	{"ball_l","leftToes"},
	{"thigh_twist_01_l",""},
	{"Thigh_R","rightUpperLeg"},
	{"calf_r","rightLowerLeg"},
	{"calf_twist_01_r",""},
	{"Foot_R","rightFoot"},
	{"ball_r","rightToes"},
	{"thigh_twist_01_r",""},
	{"ik_foot_root",""},
	{"ik_foot_l",""},
	{"ik_foot_r",""},
	{"ik_hand_root",""},
	*/
}

FAnimNode_VrmQuestHandBone::FAnimNode_VrmQuestHandBone()
{
}

void FAnimNode_VrmQuestHandBone::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmQuestHandBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{

	if (VrmMetaObject == nullptr){
		return;
	}
	if (HandTransform.HandLeft.Num() < 10 || HandReferenceTransform.HandLeft.Num() < 10) {
		return;
	}
	if (HandTransform.HandRight.Num() < 10 || HandReferenceTransform.HandRight.Num() < 10) {
		return;
	}

	check(OutBoneTransforms.Num() == 0);
	// the way we apply transform is same as FMatrix or FTransform
	// we apply scale first, and rotation, and translation
	// if you'd like to translate first, you'll need two nodes that first node does translate and second nodes to rotate.
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

	const TArray<VRMUtil::VRMBoneTable>* handTable[2] = {
		&handtable_l_ue4_vrm,
		&handtable_r_ue4_vrm,
	};

	const TArray<FTransform>* handTrans[2] = {
		&HandTransform.HandLeft,
		&HandTransform.HandRight,
	};

	const TArray<FTransform>* handRefTrans[2] = {
		&HandReferenceTransform.HandLeft,
		&HandReferenceTransform.HandRight,
	};


	for (int side = 0; side < 2; ++side) {
		for (int boneNo = 0; boneNo < handTable[side]->Num(); ++boneNo) {

			FString* boneName = VrmMetaObject->humanoidBoneTable.Find((*handTable[side])[boneNo].BoneVRM);
			if (boneName == nullptr) {
				continue;
			}
			int32_t boneIndex = BoneContainer.GetPoseBoneIndexForBoneName(**boneName);
			if (boneIndex == INDEX_NONE) {
				continue;
			}

			FCompactPoseBoneIndex CompactPoseBoneToModify(boneIndex);// = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(boneIndex);
			FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
			FTransform orgBoneTM = NewBoneTM;

			// Convert to Bone Space.
			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, EBoneControlSpace::BCS_WorldSpace);
			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, orgBoneTM, CompactPoseBoneToModify, EBoneControlSpace::BCS_ParentBoneSpace);

			//		HandLeft[i].GetRotation();
					//NewBoneTM = HandTransform.HandLeft[i];// .SetRotation(BoneQuat);

			FQuat q;
			{
				FQuat q_baseOffset;
				if (side == 0) {
					q_baseOffset = FRotator(0, 90, 180).Quaternion();
				} else {
					q_baseOffset = FRotator(0, 90, 0).Quaternion();
				}

				// correct base
				{
					q = (*handTrans[side])[boneNo].GetRotation() * q_baseOffset;
				}
			}


			if (3<=boneNo && boneNo<=5){
				//thumb
				FQuat q_baseOffset;
				if (side == 0) {
					q_baseOffset = FRotator(0, 115, 0).Quaternion();
				} else {
					q_baseOffset = FRotator(-65, 0, 90).Quaternion();
				}

				//thumb
				auto q2 = (*handRefTrans[side])[boneNo].GetRotation() *q_baseOffset;
				q = q * q2.Inverse();
			}


			//for ref test
			//q = HandReferenceTransform.HandLeft[i].GetRotation() * q;

			if (boneNo > 0) {
				{
					//auto r = fromParentTM.GetRotation();
					//q = r.Inverse() * q * r;	// wrong
					//q = r * q * r.Inverse();
				}

				//q = orgBoneTM.GetRotation() * q;

				//auto ref = (*handRefTrans[side])[boneNo].GetRotation().Inverse();
				//auto ref = HandReferenceTransform.HandLeft[boneNo].GetRotation().Inverse();
				//q = ref * q;
			}

			//NewBoneTM.SetLocation(HandTransform.HandLeft[i].GetLocation()*100.f);
			NewBoneTM.SetRotation(q);// .SetRotation(BoneQuat);
			//NewBoneTM.SetRotation(HandTransform.HandLeft[i].GetRotation());// .SetRotation(BoneQuat);

			// Convert back to Component Space.
			//FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, EBoneControlSpace::BCS_ParentBoneSpace);
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, EBoneControlSpace::BCS_WorldSpace);

			OutBoneTransforms.Add(FBoneTransform(CompactPoseBoneToModify, NewBoneTM));
		}
	}

	OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

bool FAnimNode_VrmQuestHandBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmQuestHandBone::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	//BoneToModify.Initialize(RequiredBones);
}
