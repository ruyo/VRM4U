// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "AnimNode_VrmConstraint.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone


FAnimNode_VrmConstraint::FAnimNode_VrmConstraint() {
}

void FAnimNode_VrmConstraint::UpdateCache(FComponentSpacePoseContext& Output) {
}

void FAnimNode_VrmConstraint::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
	bCallInitialized = true;
	Super::Initialize_AnyThread(Context);

}
void FAnimNode_VrmConstraint::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
	Super::CacheBones_AnyThread(Context);
}

void FAnimNode_VrmConstraint::UpdateInternal(const FAnimationUpdateContext& Context){
	Super::UpdateInternal(Context);
}


void FAnimNode_VrmConstraint::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmConstraint::EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output) {
	if (bCallByAnimInstance) {
		ActualAlpha = 1.f;
	} else {
		Super::EvaluateComponentPose_AnyThread(Output);
	}
}

void FAnimNode_VrmConstraint::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	UpdateCache(Output);

	if (VrmMetaObject == nullptr) {
		return;
	}

	for (auto& a : VrmMetaObject->VRMConstraintMeta) {

		int32 dstBoneIndex = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(*(a.Key));
		FCompactPoseBoneIndex dstPoseBoneIndex(dstBoneIndex);
		auto dstRefTrans = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().GetRefBonePose()[dstBoneIndex];
		auto dstRefRot = dstRefTrans.GetRotation();

		auto dstParentBoneIndex = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().GetParentIndex(dstBoneIndex);
		FCompactPoseBoneIndex dstParentPoseBoneIndex(dstParentBoneIndex);
		auto dstParentTrans = Output.Pose.GetComponentSpaceTransform(dstParentPoseBoneIndex);

		int32 srcBoneIndex = -1;

		switch (a.Value.type) {
			case EVRMConstraintType::None:
			break;
			case EVRMConstraintType::Roll:
			{
				auto& tmp = a.Value.constraintRoll;
				srcBoneIndex = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(*(tmp.sourceName));
			}
			break;
			case EVRMConstraintType::Aim :
			{
				auto& tmp = a.Value.constraintAim;
				srcBoneIndex = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(*(tmp.sourceName));
			}
			break;
			case EVRMConstraintType::Rotation :
			{
				auto& tmp = a.Value.constraintRotation;
				srcBoneIndex = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(*(tmp.sourceName));
			}
			break;
		}

		if (srcBoneIndex < 0) {
			continue;
		}
		FCompactPoseBoneIndex srcPoseBoneIndex(srcBoneIndex);
		auto srcRefTrans = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().GetRefBonePose()[srcBoneIndex];
		auto srcRefRot = srcRefTrans.GetRotation();

		auto srcParentBoneIndex = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().GetParentIndex(srcBoneIndex);
		FCompactPoseBoneIndex srcParentPoseBoneIndex(srcParentBoneIndex);
		auto srcParentTrans = Output.Pose.GetComponentSpaceTransform(srcParentPoseBoneIndex);


		if (a.Value.type == EVRMConstraintType::Rotation) {
			auto& rot = a.Value.constraintRotation;

			auto srcCurrentTrans = Output.Pose.GetLocalSpaceTransform(srcPoseBoneIndex);
			auto srcCurrentRot = srcCurrentTrans.GetRotation();

			auto dstCurrentTrans = Output.Pose.GetComponentSpaceTransform(dstPoseBoneIndex);
			auto dstCurrentRot = dstCurrentTrans.GetRotation();

			{
				auto r = (srcRefRot.Inverse() * srcCurrentRot);
				FVector axis;
				decltype(FVector::X) angle;
				r.ToAxisAndAngle(axis, angle);
				r = FQuat(axis, angle * rot.weight);
				r = dstParentTrans.GetRotation() * dstRefRot * r;

				dstCurrentTrans.SetRotation(r);
				Output.Pose.SetComponentSpaceTransform(dstPoseBoneIndex, dstCurrentTrans);
			}
		}
		if (a.Value.type == EVRMConstraintType::Roll) {
			auto& roll = a.Value.constraintRoll;

			auto srcCurrentTrans = Output.Pose.GetLocalSpaceTransform(srcPoseBoneIndex);
			auto srcCurrentRot = srcCurrentTrans.GetRotation();

			auto dstCurrentTrans = Output.Pose.GetComponentSpaceTransform(dstPoseBoneIndex);
			auto dstCurrentRot = dstCurrentTrans.GetRotation();

			{
				auto r = (srcRefRot.Inverse() * srcCurrentRot);
				FVector axis;
				decltype(FVector::X) angle;
				r.ToAxisAndAngle(axis, angle);

				FVector v;
				if (roll.rollAxis.Find("X") >= 0) {
					v.Set(1, 0, 0);
				}
				if (roll.rollAxis.Find("Y") >= 0) {
					v.Set(0, 1, 0);
				}
				if (roll.rollAxis.Find("Z") >= 0) {
					v.Set(0, 0, 1);
				}
				if (roll.rollAxis.Find("negative") >= 0) {
					v *= -1.f;
				}
				
				auto d = FVector::DotProduct(axis, v);
				r = FQuat(v, angle * roll.weight * d);

				r = dstParentTrans.GetRotation() * dstRefRot * r;
				dstCurrentTrans.SetRotation(r);
				Output.Pose.SetComponentSpaceTransform(dstPoseBoneIndex, dstCurrentTrans);
			}
		}
		if (a.Value.type == EVRMConstraintType::Aim) {
			auto& aim = a.Value.constraintAim;

			//auto srcCurrentTrans = Output.Pose.GetComponentSpaceTransform(srcPoseBoneIndex);
			//auto srcCurrentRot = srcCurrentTrans.GetRotation();

			//auto dstCurrentTrans = Output.Pose.GetComponentSpaceTransform(dstPoseBoneIndex);
			auto dstCurrentTrans = dstRefTrans * dstParentTrans;
			//auto dstCurrentRot = dstCurrentTrans.GetRotation();

			{

				FVector v;
				if (aim.aimAxis.Find("X") >= 0) {
					v.Set(1, 0, 0);
				}
				if (aim.aimAxis.Find("Y") >= 0) {
					v.Set(0, 1, 0);
				}
				if (aim.aimAxis.Find("Z") >= 0) {
					v.Set(0, 0, 1);
				}
				if (aim.aimAxis.Find("negative") >= 0) {
					v *= -1.f;
				}

				auto fromVec = dstCurrentTrans.TransformVector(v);
				auto toVec = srcParentTrans.TransformVector(v);
				//auto toVec = (srcCurrentTrans.GetLocation() - srcRefTrans.GetLocation()).Normalize();
				auto fromToQuat = FQuat::FindBetweenNormals(fromVec, toVec);

				auto r = fromToQuat;

				FVector axis;
				decltype(FVector::X) angle;
				r.ToAxisAndAngle(axis, angle);
				r = FQuat(axis, angle * aim.weight);

				r = dstCurrentTrans.GetRotation().Inverse() * r * dstCurrentTrans.GetRotation();

				r = dstParentTrans.GetRotation() *  dstRefRot * r;
				dstCurrentTrans.SetRotation(r);
				Output.Pose.SetComponentSpaceTransform(dstPoseBoneIndex, dstCurrentTrans);
			}
		}
	}
}

bool FAnimNode_VrmConstraint::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	return true;
}

void FAnimNode_VrmConstraint::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}

void FAnimNode_VrmConstraint::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp, bool bPreviewForeground) const
{
#if WITH_EDITOR

	if (VrmMetaObject == nullptr || PreviewSkelMeshComp == nullptr) {
		return;
	}
	if (PreviewSkelMeshComp->GetWorld() == nullptr) {
		return;
	}
#endif
}
