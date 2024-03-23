// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.



#include "AnimNode_VrmCopyHandBone.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "BonePose.h"

#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone

namespace {

	const TArray<FString> HandBoneTableLeap = {
//		"L_Wrist",
		//"L_Palm",
		"L_thumb_meta",
		"L_thumb_a",
		"L_thumb_b",
//		"L_thumb_end",
//		"L_index_meta",
		"L_index_a",
		"L_index_b",
		"L_index_c",
//		"L_index_end",
//		"L_middle_meta",
		"L_middle_a",
		"L_middle_b",
		"L_middle_c",
//		"L_middle_end",
//		"L_ring_meta",
		"L_ring_a",
		"L_ring_b",
		"L_ring_c",
//		"L_ring_end",
//		"L_pinky_meta",
		"L_pinky_a",
		"L_pinky_b",
		"L_pinky_c",
//		"L_pinky_end",

//		"R_Wrist",
		//"R_Palm",
		"R_thumb_meta",
		"R_thumb_a",
		"R_thumb_b",
//		"R_thumb_end",
//		"R_index_meta",
		"R_index_a",
		"R_index_b",
		"R_index_c",
//		"R_index_end",
//		"R_middle_meta",
		"R_middle_a",
		"R_middle_b",
		"R_middle_c",
//		"R_middle_end",
//		"R_ring_meta",
		"R_ring_a",
		"R_ring_b",
		"R_ring_c",
//		"R_ring_end",
//		"R_pinky_meta",
		"R_pinky_a",
		"R_pinky_b",
		"R_pinky_c",
//		"R_pinky_end",
	};

	const TArray<FString> HandBoneTableVRoid = {
		//"J_Bip_L_Hand",
		"J_Bip_L_Thumb1",
		"J_Bip_L_Thumb2",
		"J_Bip_L_Thumb3",
		"J_Bip_L_Index1",
		"J_Bip_L_Index2",
		"J_Bip_L_Index3",
		"J_Bip_L_Middle1",
		"J_Bip_L_Middle2",
		"J_Bip_L_Middle3",
		"J_Bip_L_Ring1",
		"J_Bip_L_Ring2",
		"J_Bip_L_Ring3",
		"J_Bip_L_Little1",
		"J_Bip_L_Little2",
		"J_Bip_L_Little3",


		//"J_Bip_R_Hand",
		"J_Bip_R_Thumb1",
		"J_Bip_R_Thumb2",
		"J_Bip_R_Thumb3",
		"J_Bip_R_Index1",
		"J_Bip_R_Index2",
		"J_Bip_R_Index3",
		"J_Bip_R_Middle1",
		"J_Bip_R_Middle2",
		"J_Bip_R_Middle3",
		"J_Bip_R_Ring1",
		"J_Bip_R_Ring2",
		"J_Bip_R_Ring3",
		"J_Bip_R_Little1",
		"J_Bip_R_Little2",
		"J_Bip_R_Little3",
	};

	const TArray<FString> HandBoneTableVRM = {
		//"leftHand",
		"leftThumbProximal",
		"leftThumbIntermediate",
		"leftThumbDistal",
		"leftIndexProximal",
		"leftIndexIntermediate",
		"leftIndexDistal",
		"leftMiddleProximal",
		"leftMiddleIntermediate",
		"leftMiddleDistal",
		"leftRingProximal",
		"leftRingIntermediate",
		"leftRingDistal",
		"leftLittleProximal",
		"leftLittleIntermediate",
		"leftLittleDistal",

		//"rightHand"
		"rightThumbProximal",
		"rightThumbIntermediate",
		"rightThumbDistal",
		"rightIndexProximal",
		"rightIndexIntermediate",
		"rightIndexDistal",
		"rightMiddleProximal",
		"rightMiddleIntermediate",
		"rightMiddleDistal",
		"rightRingProximal",
		"rightRingIntermediate",
		"rightRingDistal",
		"rightLittleProximal",
		"rightLittleIntermediate",
		"rightLittleDistal",

	};

}

FAnimNode_VrmCopyHandBone::FAnimNode_VrmCopyHandBone()
{
}

void FAnimNode_VrmCopyHandBone::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmCopyHandBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	const auto dstRefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();

	const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

	//dstRefSkeleton.GetParentIndex

	TArray<FBoneTransform> tmpOutTransform;
	tmpOutTransform.SetNum(HandBoneTableVRoid.Num());

	TArray<int> boneIndexTable;
	boneIndexTable.SetNum(HandBoneTableVRoid.Num());

	auto BoneSpace = EBoneControlSpace::BCS_ParentBoneSpace;

	TArray<USkeletalMeshComponent*> MeshTable = {
		SkeletalMeshComponentLeft,
		SkeletalMeshComponentRight,
	};

	for (int i = 0; i < HandBoneTableVRM.Num(); ++i) {

		FString BoneNameModel = HandBoneTableVRoid[i];
		FString BoneNameLeap = HandBoneTableLeap[i];

		if (VrmMetaObject) {
			FString *s = VrmMetaObject->humanoidBoneTable.Find(HandBoneTableVRM[i]);
			if (s) {
				BoneNameModel = *s;
			}
		}

		boneIndexTable[i] = -1;
		int skelNo = -1;
		for (auto SkeletalMeshComponent : MeshTable) {
			skelNo++;

			if (SkeletalMeshComponent == nullptr) {
				continue;
			}

			const auto &dstRefSkeletonTransform = dstRefSkeleton.GetRefBonePose();
			const auto &srcRefSkeletonTransform = VRMGetRefSkeleton( VRMGetSkinnedAsset(SkeletalMeshComponent) ).GetRefBonePose();

			const auto srcIndex = SkeletalMeshComponent->GetBoneIndex(*BoneNameLeap);
			const auto dstIndex = dstRefSkeleton.FindBoneIndex(*BoneNameModel);

			if (srcIndex < 0 || dstIndex < 0) {
				continue;
			}

			const auto& srcCurrentTrans = SkeletalMeshComponent->GetSocketTransform(*BoneNameLeap, RTS_ParentBoneSpace);

			const auto& srcRefTrans = srcRefSkeletonTransform[srcIndex];
			const auto& dstRefTrans = dstRefSkeletonTransform[dstIndex];

			FCompactPoseBoneIndex CompactPoseBoneToModify(dstIndex);

			//auto a = srcCurrentTrans;
			FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, BoneSpace);

			FQuat baseDiff = FQuat::FindBetween(srcRefTrans.GetLocation(), dstRefTrans.GetLocation());
			//FQuat q9 = FQuat(FVector(1, 0, 0), 3.14f / 2.f);
			FQuat q9;
			
			auto v = srcRefTrans.GetLocation().GetSafeNormal();
			if (skelNo == 0) {
				if (BoneNameLeap.Find(TEXT("thumb")) >= 0) {
					if (BoneNameLeap.Find(TEXT("meta")) < 0) {
						q9 = FQuat(FVector(1, 0, 0), 3.14f / 2.f) * FQuat(v, -3.14f / 2.f) * srcRefTrans.GetRotation().Inverse();
					} else {
						auto vv = srcRefTrans.GetLocation();
						vv.Y = srcRefTrans.GetLocation().Z;
						vv.Z = srcRefTrans.GetLocation().Y;

						FQuat baseDiff2 = FQuat::FindBetween(vv, FVector(1,0,0));
						q9 = FQuat(v, -3.14f / 2.f) * baseDiff2.Inverse() * srcRefTrans.GetRotation().Inverse();
					}
				}else {
					q9 = FQuat(v, 3.14f / 2.f) * srcRefTrans.GetRotation().Inverse();
				}
			} else {
				if (BoneNameLeap.Find(TEXT("thumb")) >= 0) {
					if (BoneNameLeap.Find(TEXT("meta")) < 0) {
						//q9 = srcRefTrans.GetRotation().Inverse();
						q9 = FQuat(FVector(1, 0, 0), 3.14f / 2.f) * FQuat(v, -3.14f / 2.f) * srcRefTrans.GetRotation().Inverse();
					} else {
						//q9 = FQuat(-v, 3.14f) * srcRefTrans.GetRotation().Inverse();

						auto vv = srcRefTrans.GetLocation();
						vv.X = srcRefTrans.GetLocation().X;
						vv.Y = srcRefTrans.GetLocation().Z;
						vv.Z = srcRefTrans.GetLocation().Y;

						FQuat baseDiff2 = FQuat::FindBetween(vv, FVector(-1,0,0));
						auto tt = srcRefTrans;
						tt.SetLocation(vv);
						//q9 = FQuat(FVector(1, 0, 0), 3.14f / 2.f) * FQuat(v, 3.14f / 2.f) * baseDiff2.Inverse() * srcRefTrans.GetRotation().Inverse();
						//q9 = FQuat(FVector(0, 0, -1), -3.14f / 2.f) * FQuat(FVector(1, 0, 0), -3.14f / 2.f) * FQuat(FVector(0, 1, 0), -3.14f / 2.f) * FQuat(v, 3.14f / 2.f) * baseDiff2.Inverse() * srcRefTrans.GetRotation().Inverse();
						q9 = FQuat(FVector(0, 0, -1), -3.14f / 2.f) * FQuat(FVector(1, 0, 0), -3.14f / 2.f) * FQuat(FVector(0, 1, 0), -3.14f / 2.f) * FQuat(v, 3.14f / 2.f) * baseDiff2.Inverse() * srcRefTrans.GetRotation().Inverse();
						//q9.X *= -1.f;
						//q9.Y *= -1.f;
						//q9.Z *= -1.f;


/*
auto vv = srcRefTrans.GetLocation();
vv.X = -srcRefTrans.GetLocation().X;
vv.Y = srcRefTrans.GetLocation().Z;
vv.Z = srcRefTrans.GetLocation().Y;

FQuat baseDiff2 = FQuat::FindBetween(vv, FVector(1,0,0));
//q9 = FQuat(FVector(1, 0, 0), 3.14f / 2.f) * FQuat(v, 3.14f / 2.f) * baseDiff2.Inverse() * srcRefTrans.GetRotation().Inverse();
q9 = FQuat(FVector(1, 0, 0), -3.14f / 2.f) * FQuat(FVector(0, 1, 0), -3.14f / 2.f) * FQuat(v, 3.14f / 2.f) * baseDiff2.Inverse() * srcRefTrans.GetRotation().Inverse();
*/
					}
				} else {
					q9 = FQuat(-v, 3.14f / 2.f) * FQuat(FVector(0, 1, 0), 3.14f) * FQuat(FVector(0, 0, 1), 3.14f) * srcRefTrans.GetRotation().Inverse();
				}
			}

			FQuat q = baseDiff * q9;
			//FQuat q = FQuat::FindBetween(srcRefTrans.GetLocation(), dstRefTrans.GetLocation());

			//NewBoneTM.SetRotation(q*FQuat(FVector(1, 0, 0), 3.14f / 2.f) * srcCurrentTrans.GetRotation() * FQuat(FVector(-1, 0, 0), 3.14f / 2.f));
			//NewBoneTM.SetRotation(FQuat(FVector(1, 0, 0), 3.14f / 2.f) * srcCurrentTrans.GetRotation() * FQuat(FVector(-1, 0, 0), 3.14f / 2.f));

			NewBoneTM.SetRotation(q * srcCurrentTrans.GetRotation() * q.Inverse());

			//NewBoneTM.SetRotation(q9 * baseDiff*srcCurrentTrans.GetRotation() * baseDiff.Inverse() * q9.Inverse());
			//NewBoneTM.SetRotation(q9 * baseDiff*srcCurrentTrans.GetRotation() * q9.Inverse());


			//NewBoneTM.SetLocation(dstRefTrans.GetLocation());

			//NewBoneTM.SetLocation(NewBoneTM.GetLocation() );
			//NewBoneTM.SetLocation(NewBoneTM.GetRotation() * dstRefTrans.GetLocation() );
			//NewBoneTM.SetLocation(dstRefTrans.GetLocation());
			//a.SetLocation(dstRefTrans.GetLocation());

			//FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, a, CompactPoseBoneToModify, EBoneControlSpace::BCS_ParentBoneSpace);

			//OutBoneTransforms.Insert
			tmpOutTransform[i] = FBoneTransform(CompactPoseBoneToModify, NewBoneTM);
			boneIndexTable[i] = dstIndex;
			break;
		}
	}
	{
		tmpOutTransform.Sort(FCompareBoneTransformIndex());
		//tmpOutTransform.Sort([](const FBoneTransform& x, const FBoneTransform& y) { return x.BoneIndex.GetInt() < y.BoneIndex.GetInt(); });

		boneIndexTable.Sort();
		for (int i=0; i<tmpOutTransform.Num(); ++i){
			if (boneIndexTable[i] < 0) {
				continue;
			}
			auto &a = tmpOutTransform[i];

			int parentBoneIndex = dstRefSkeleton.GetParentIndex(boneIndexTable[i]);

			int parentInHandTable = boneIndexTable.Find(parentBoneIndex);
			if (parentInHandTable >= 0) {
				a.Transform *= tmpOutTransform[parentInHandTable].Transform;
			}else {
				FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, a.Transform, a.BoneIndex, BoneSpace);
			}
			OutBoneTransforms.Add(a);
		}
	}
}

bool FAnimNode_VrmCopyHandBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmCopyHandBone::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}
