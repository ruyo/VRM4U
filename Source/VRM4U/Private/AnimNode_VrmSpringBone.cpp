// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


/*

// VRMSpringBone.cs
MIT License

Copyright (c) 2018 DWANGO Co., Ltd. for UniVRM
Copyright (c) 2018 ousttrue for UniGLTF, UniHumanoid
Copyright (c) 2018 Masataka SUMI for MToon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "AnimNode_VrmSpringBone.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SceneInterface.h"
#include "DrawDebugHelpers.h"

#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include "VrmSpringBone.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone

FAnimNode_VrmSpringBone::FAnimNode_VrmSpringBone()
{
	NoWindBoneNameList = TArray<FName>{
		TEXT("J_Sec_L_Bust1"),
		TEXT("J_Sec_L_Bust2"),
		TEXT("J_Sec_R_Bust1"),
		TEXT("J_Sec_R_Bust2"),
	};
}

//void FAnimNode_VrmSpringBone::Update_AnyThread(const FAnimationUpdateContext& Context) {
//	Super::Update_AnyThread(Context);
//	//Context.GetDeltaTime();
//}

bool FAnimNode_VrmSpringBone::IsSpringInit() const {
	if (SpringManager.Get()) {
		return SpringManager->bInit;
	}
	return false;
}
void FAnimNode_VrmSpringBone::Initialize_AnyThread(const FAnimationInitializeContext& Context) {

	Super::Initialize_AnyThread(Context);

	if (SpringManager.Get()) {
		SpringManager.Get()->reset();
	}
	else {
		if (VrmMetaObject) {
			if (VrmMetaObject->GetVRMVersion() >= 1) {
				SpringManager = MakeShareable(new VRM1Spring::VRM1SpringManager());
			}
		}
		if (SpringManager == nullptr) {
			SpringManager = MakeShareable(new VRMSpringBone::VRMSpringManager());
		}
	}
}
void FAnimNode_VrmSpringBone::Initialize_AnyThread_local(const FAnimationInitializeContext& Context) {
	{
		//Super::Initialize_AnyThread(Context); crash

		//FAnimNode_SkeletalControlBase::Initialize_AnyThread
		FAnimNode_Base::Initialize_AnyThread(Context);
		//ComponentPose.Initialize(Context);
		AlphaBoolBlend.Reinitialize();
		AlphaScaleBiasClamp.Reinitialize();
	}
	if (SpringManager.Get()) {
		SpringManager.Get()->reset();
	}
	else {
		if (VrmMetaObject) {
			if (VrmMetaObject->GetVRMVersion() >= 1) {
				SpringManager = MakeShareable(new VRM1Spring::VRM1SpringManager());
			}
		}
		if (SpringManager == nullptr) {
			SpringManager = MakeShareable(new VRMSpringBone::VRMSpringManager());
		}
	}
}

void FAnimNode_VrmSpringBone::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
	Super::CacheBones_AnyThread(Context);
}

#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
void FAnimNode_VrmSpringBone::ResetDynamics(ETeleportType InTeleportType) {
	Super::ResetDynamics(InTeleportType);
	if (SpringManager.Get()){
		bool bReset = true;
		if (InTeleportType == ETeleportType::TeleportPhysics) {
			if (bIgnorePhysicsResetOnTeleport) {
				bReset = false;
			}
		}
		if (bReset) {
			SpringManager.Get()->reset();
		}
	}
}
#endif

void FAnimNode_VrmSpringBone::UpdateInternal(const FAnimationUpdateContext& Context){
	Super::UpdateInternal(Context);

	CurrentDeltaTime = Context.GetDeltaTime();
}


void FAnimNode_VrmSpringBone::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmSpringBone::EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output) {
	if (bCallByAnimInstance) {
		ActualAlpha = 1.f;
	/*
		EvaluateComponentSpaceInternal(Output);

		BoneTransformsSpring.Reset(BoneTransformsSpring.Num());
		EvaluateSkeletalControl_AnyThread(Output, BoneTransformsSpring);

		if (BoneTransformsSpring.Num() > 0)
		{
			ActualAlpha = 1.f;
			const float BlendWeight = FMath::Clamp<float>(ActualAlpha, 0.f, 1.f);
			Output.Pose.LocalBlendCSBoneTransforms(BoneTransformsSpring, BlendWeight);
		}
	*/
	}
	else {
		Super::EvaluateComponentPose_AnyThread(Output);
	}
}

void FAnimNode_VrmSpringBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
	const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

	//dstRefSkeleton.GetParentIndex

	//auto BoneSpace = EBoneControlSpace::BCS_ParentBoneSpace;
	auto BoneSpace = EBoneControlSpace::BCS_WorldSpace;
	{

		if (VrmMetaObject == nullptr) {
			return;
		}
		if (VRMGetSkeleton(VrmMetaObject->SkeletalMesh) != Output.AnimInstanceProxy->GetSkeleton()) {
			//skip for renamed bone
			//return;
		}
		if (Output.Pose.GetPose().GetNumBones() <= 0) {
			return;
		}

		const auto &RefSkeletonTransform = Output.Pose.GetPose().GetBoneContainer().GetRefPoseArray();

		{
			if (SpringManager.Get() == nullptr) {
				return;
			}
			if (SpringManager->bInit == false) {
				SpringManager->init(VrmMetaObject, Output);
				return;
			}

			SpringManager->update(this, CurrentDeltaTime, Output, OutBoneTransforms);

			SpringManager->applyToComponent(Output, OutBoneTransforms);

/*
			for (auto &springRoot : SpringManager->spring) {
				for (auto &sChain : springRoot.SpringDataChain) {
					int BoneChain = 0;

					FTransform CurrentTransForm = FTransform::Identity;
					for (auto &sData : sChain) {


						//FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(sData.boneIndex);
						FCompactPoseBoneIndex uu(sData.boneIndex);

						if (Output.Pose.GetPose().IsValidIndex(uu) == false) {
							continue;
						}

						FTransform NewBoneTM;

						if (BoneChain == 0) {
							NewBoneTM = Output.Pose.GetComponentSpaceTransform(uu);
							NewBoneTM.SetRotation(sData.m_resultQuat);

							CurrentTransForm = NewBoneTM;
						}else{

							NewBoneTM = CurrentTransForm;
							
							auto c = RefSkeletonTransform[sData.boneIndex];
							NewBoneTM = c * NewBoneTM;
							NewBoneTM.SetRotation(sData.m_resultQuat);


							//const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
							//NewBoneTM.SetLocation(ComponentTransform.TransformPosition(sData.m_currentTail));

							CurrentTransForm = NewBoneTM;
						}

						FBoneTransform a(uu, NewBoneTM);

						bool bFirst = true;
						for (auto &t : OutBoneTransforms) {
							if (t.BoneIndex == a.BoneIndex) {
								bFirst = false;
								break;
							}
						}

						if (bFirst) {
							OutBoneTransforms.Add(a);
						}
						BoneChain++;
					}
				}

			}
			OutBoneTransforms.Sort(FCompareBoneTransformIndex());
			*/

		}
	}
}

bool FAnimNode_VrmSpringBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmSpringBone::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}

void FAnimNode_VrmSpringBone::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp, bool bPreviewForeground) const
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

	for (const auto &colMeta : VrmMetaObject->VRMColliderMeta) {
		const FTransform t = PreviewSkelMeshComp->GetSocketTransform(*colMeta.boneName);

		for (const auto &col : colMeta.collider) {
			float r = (col.radius) * 100.f;
			//FVector v = collisionBoneTrans.TransformPosition(c.offset*100);
			auto offs = col.offset;
			offs.Set(-offs.X, offs.Z, offs.Y);
			offs *= 100;
			FVector v = t.TransformPosition(offs);

			FTransform tt = t;
			tt.AddToTranslation(offs);

			DrawWireSphere(PDI, tt, FLinearColor::Green, r, 8, Priority);
		}
	}

	{
		struct SData{
			int32 bone = 0;
			float radius = 0.f;

			bool operator==(const SData& a) const {
				return (bone == a.bone && radius == a.radius);
			}
		};

		TArray<SData> dataList;
		TArray<int32> boneList;

		for (const auto spr : VrmMetaObject->VRMSpringMeta) {
			for (const auto boneName : spr.boneNames) {
				int32_t boneIndex = PreviewSkelMeshComp->GetBoneIndex(*boneName);
				boneList.AddUnique(boneIndex);

				{
					SData s;
					s.bone = boneIndex;
					s.radius = spr.hitRadius;

					dataList.AddUnique(s);
				}

				for (int i = 0; i < boneList.Num(); ++i) {
					TArray<int32> c;
					VRMUtil::GetDirectChildBones(VRMGetRefSkeleton( VRMGetSkinnedAsset(PreviewSkelMeshComp) ), boneList[i], c);
					if (c.Num()) {
						for (const auto cc : c) {
							boneList.AddUnique(cc);

							SData s;
							s.bone = cc;
							s.radius = spr.hitRadius;
							dataList.AddUnique(s);
						}
					}
				}
			}
		}
		for (int i = 0; i < dataList.Num(); ++i) {
			const auto &name = PreviewSkelMeshComp->GetBoneName(dataList[i].bone);

			FTransform t = PreviewSkelMeshComp->GetSocketTransform(name);
			float r = dataList[i].radius * 100.f;
			FVector v = t.GetLocation();

			DrawWireSphere(PDI, t, FLinearColor::Red, r, 8, Priority);
		}
		/*
		for (int i = 0; i < boneList.Num(); ++i) {
			const auto& name = PreviewSkelMeshComp->GetBoneName(boneList[i]);

			FTransform t = PreviewSkelMeshComp->GetSocketTransform(name);
			float r = spr.hitRadius * 100.f;
			FVector v = t.GetLocation();

			DrawDebugSphere(PreviewSkelMeshComp->GetWorld(), v, r, 8, FColor::Red, true, 0.1f);
		}
		*/
	}



	if (PreviewSkelMeshComp && PreviewSkelMeshComp->GetWorld())
	{
		FVector const CSEffectorLocation = FVector(0, 0, 0);//CachedEffectorCSTransform.GetLocation();

		FVector const Precision = FVector(1);//CachedEffectorCSTransform.GetLocation();

		FTransform CachedEffectorCSTransform;

		// Show end effector position.
		DrawDebugBox(PreviewSkelMeshComp->GetWorld(), CSEffectorLocation, FVector(Precision), FColor::Green, true, 0.1f);
		DrawDebugCoordinateSystem(PreviewSkelMeshComp->GetWorld(), CSEffectorLocation, CachedEffectorCSTransform.GetRotation().Rotator(), 5.f, true, 0.1f);
	}
#endif
}
