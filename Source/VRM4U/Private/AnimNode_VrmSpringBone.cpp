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
#include "VrmAssetListObject.h"
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

	if (Context.AnimInstanceProxy == nullptr) return;

	VrmMetaObject_Internal = VrmMetaObject;
	if (VrmMetaObject_Internal == nullptr && EnableAutoSearchMetaData) {
		VrmAssetListObject_Internal = VRMUtil::GetAssetListObject(VRMGetSkinnedAsset(Context.AnimInstanceProxy->GetSkelMeshComponent()));
		if (VrmAssetListObject_Internal) {
			VrmMetaObject_Internal = VrmAssetListObject_Internal->VrmMetaObject;
		}
	}

	if (SpringManager.Get()) {
		SpringManager.Get()->reset();
	} else {
		if (VrmMetaObject_Internal) {
			if (VrmMetaObject_Internal->GetVRMVersion() >= 1) {
				SpringManager = MakeShareable(new VRM1Spring::VRM1SpringManager());
			}
		}
		if (SpringManager.IsValid() == false) {
			SpringManager = MakeShareable(new VRMSpringBone::VRMSpringManager());
		}
		if (SpringManager.Get()) {
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
		if (VrmMetaObject_Internal) {
			if (VrmMetaObject_Internal->GetVRMVersion() >= 1) {
				SpringManager = MakeShareable(new VRM1Spring::VRM1SpringManager());
			}
		}
		if (SpringManager.IsValid() == false) {
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

		if (VrmMetaObject_Internal == nullptr) {
			return;
		}
		if (VRMGetSkeleton(VrmMetaObject_Internal->SkeletalMesh) != Output.AnimInstanceProxy->GetSkeleton()) {
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
				SpringManager->init(VrmMetaObject_Internal.Get(), Output);
				return;
			}

			SpringManager->update(this, CurrentDeltaTime, Output, OutBoneTransforms);

			SpringManager->applyToComponent(Output, OutBoneTransforms);

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

	auto MetaObjectLocal = VrmMetaObject_Internal;

	if (MetaObjectLocal == nullptr && EnableAutoSearchMetaData) {
		auto *p = VRMUtil::GetAssetListObject(VRMGetSkinnedAsset(PreviewSkelMeshComp));
		if (p) {
			MetaObjectLocal = p->VrmMetaObject;
		}
	}


	if (MetaObjectLocal == nullptr || PreviewSkelMeshComp == nullptr) {
		return;
	}
	if (PreviewSkelMeshComp->GetWorld() == nullptr) {
		return;
	}

	ESceneDepthPriorityGroup Priority = SDPG_World;
	if (bPreviewForeground) Priority = SDPG_Foreground;

	if (MetaObjectLocal->VRM1SpringBoneMeta.Springs.Num() > 0) {
		// vrm1

		// col
		for (const auto& c : MetaObjectLocal->VRM1SpringBoneMeta.Colliders) {
			const FTransform t = PreviewSkelMeshComp->GetSocketTransform(*c.boneName);

			float r = (c.radius) * 100.f;
			auto offs = c.offset;
			offs.Set(offs.X, -offs.Z, offs.Y);
			offs *= 100;
			offs = t.TransformVector(offs);

			if (c.shapeType == TEXT("sphere")) {

				FTransform tt = t;
				tt.AddToTranslation(offs);

				DrawWireSphere(PDI, tt, FLinearColor(1,1,0), r, 32, Priority);
			}
			else {

				auto tail = c.tail;
				tail.Set(tail.X, -tail.Z, tail.Y);
				tail *= 100;
				tail = t.TransformVector(tail);

				FTransform t1 = t;
				t1.AddToTranslation(offs);

				FTransform t2 = t;
				t2.AddToTranslation(tail);

				if (0) {
					// sphere and line
					DrawWireSphere(PDI, t1, FLinearColor(1, 1, 1), r, 32, Priority);
					DrawWireSphere(PDI, t2, FLinearColor::Green, r, 32, Priority);
					PDI->DrawLine(
						t1.GetLocation(),
						t2.GetLocation(),
						FLinearColor::Green,
						Priority);
				}
				else {
					// capsule
					FVector center = (t1.GetLocation() + t2.GetLocation()) / 2.f;
					FVector Up = (t1.GetLocation() - t2.GetLocation()).GetSafeNormal();
					FVector Forward, Right;

					Up.FindBestAxisVectors(Forward, Right);
					const FVector X = (Forward);
					const FVector Y = (Right);
					const FVector Z = (Up);
					float halfheight = (t1.GetLocation() - center).Size() + r;
					DrawWireCapsule(PDI, center, X, Y, Z, FLinearColor::Green, r, halfheight, 32, Priority);
				}
			}
		}

		// spring col

		for (int springNo = 0; springNo < MetaObjectLocal->VRM1SpringBoneMeta.Springs.Num(); ++springNo) {
			const auto& s = MetaObjectLocal->VRM1SpringBoneMeta.Springs[springNo];


			const TArray<FLinearColor> color = {
				FLinearColor(1.f, 0, 0),
				FLinearColor(1.f, 0.5, 0),
				FLinearColor(1.f, 0, 0.5),
			};

			for (int jointNo = 0; jointNo < s.joints.Num()-1; ++jointNo) {
				auto& j1 = s.joints[jointNo];
				auto& j2 = s.joints[jointNo + 1];

				const FTransform t = PreviewSkelMeshComp->GetSocketTransform(*j2.boneName);

				float r = j1.hitRadius * 100.f;

				DrawWireSphere(PDI, t.GetLocation(), color[springNo%color.Num()], r, 32, Priority);

				const FTransform t2 = PreviewSkelMeshComp->GetSocketTransform(*j1.boneName);

				PDI->DrawLine(
					t.GetLocation(),
					t2.GetLocation(),
					color[springNo % color.Num()] / 2.f,
					Priority);
			}
		}
	}

	// vrm0
	for (const auto &colMeta : MetaObjectLocal->VRMColliderMeta) {
		const FTransform t = PreviewSkelMeshComp->GetSocketTransform(*colMeta.boneName);

		for (const auto &col : colMeta.collider) {
			float r = (col.radius) * 100.f;
			auto offs = col.offset;
			//offs.Set(offs.X, -offs.Z, offs.Y);	// 本来はこれが正しいが、VRM0の座標が間違っている
			offs.Set(-offs.X, offs.Z, offs.Y);		// VRM0の仕様としては これ
			offs *= 100;
			FVector v = t.TransformPosition(offs);

			FTransform tt = t;
			tt.AddToTranslation(offs);

			DrawWireSphere(PDI, tt, FLinearColor::Green, r, 32, Priority);
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

		for (const auto &spr : MetaObjectLocal->VRMSpringMeta) {
			for (const auto &boneName : spr.boneNames) {
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
						for (const auto &cc : c) {
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
		const auto &RefBonePose = VRMGetRefSkeleton(VRMGetSkinnedAsset(PreviewSkelMeshComp)).GetRefBonePose();

		for (int i = 0; i < dataList.Num(); ++i) {
			FTransform t;

			{
				TArray<int32> c;
				VRMUtil::GetDirectChildBones(VRMGetRefSkeleton(VRMGetSkinnedAsset(PreviewSkelMeshComp)), dataList[i].bone, c);
				if (c.IsValidIndex(0)) {
					const auto& name = PreviewSkelMeshComp->GetBoneName(c[0]);
					t = PreviewSkelMeshComp->GetSocketTransform(name);
				}
				else {
					const auto& name = PreviewSkelMeshComp->GetBoneName(dataList[i].bone);
					t = PreviewSkelMeshComp->GetSocketTransform(name);
					//t.SetLocation(t.GetLocation() * 1.7);
					t.SetLocation(t.GetLocation() + RefBonePose[dataList[i].bone].GetLocation() * 0.7);
				}
			}



			float r = dataList[i].radius * 100.f;
			FVector v = t.GetLocation();

			DrawWireSphere(PDI, t, FLinearColor::Red, r, 32, Priority);
		}
	}



	if (PreviewSkelMeshComp && PreviewSkelMeshComp->GetWorld())
	{
		FVector const CSEffectorLocation = FVector(0, 0, 0);//CachedEffectorCSTransform.GetLocation();

		FVector const Precision = FVector(1);//CachedEffectorCSTransform.GetLocation();

		FTransform CachedEffectorCSTransform;

		// Show end effector position.
		//DrawDebugBox(PreviewSkelMeshComp->GetWorld(), CSEffectorLocation, FVector(Precision), FColor::Green, true, 0.1f);
		//DrawDebugCoordinateSystem(PreviewSkelMeshComp->GetWorld(), CSEffectorLocation, CachedEffectorCSTransform.GetRotation().Rotator(), 5.f, true, 0.1f);
	}
#endif
}
