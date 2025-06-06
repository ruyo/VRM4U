// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "AnimNode_VrmRetargetFromMannequin.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone

FAnimNode_VrmRetargetFromMannequin::FAnimNode_VrmRetargetFromMannequin() {
}

void FAnimNode_VrmRetargetFromMannequin::PreUpdate(const UAnimInstance* InAnimInstance)
{
	//DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(PreUpdate)

	Super::PreUpdate(InAnimInstance);

	//FAnimNode_RetargetPoseFromMesh::EnsureInitialized
	// if user hasn't explicitly connected a source mesh, optionally use the parent mesh component (if there is one) 
	if (!srcMannequinMesh.IsValid() && bUseAttachedParent)
	{
		USkeletalMeshComponent* TargetMesh = InAnimInstance->GetSkelMeshComponent();

		// Walk up the attachment chain until we find a skeletal mesh component
		USkeletalMeshComponent* ParentMeshComponent = nullptr;
		for (USceneComponent* AttachParentComp = TargetMesh->GetAttachParent(); AttachParentComp != nullptr; AttachParentComp = AttachParentComp->GetAttachParent())
		{
			ParentMeshComponent = Cast<USkeletalMeshComponent>(AttachParentComp);
			if (ParentMeshComponent)
			{
				break;
			}
		}
		if (ParentMeshComponent == nullptr) {
			AActor *targetActor = nullptr;
			AActor *myActor = TargetMesh->GetOwner();
			for (USceneComponent* AttachParentComp = TargetMesh->GetAttachParent(); AttachParentComp != nullptr; AttachParentComp = AttachParentComp->GetAttachParent()) {
				targetActor = AttachParentComp->GetOwner();
				if (targetActor != myActor) {
					break;
				}
				targetActor = nullptr;
			}
			if (targetActor) {
				TArray<USkeletalMeshComponent*> c;
				targetActor->GetComponents<USkeletalMeshComponent>(c);
				if (c.IsValidIndex(0)) {
					ParentMeshComponent = c[0];
				}
			}
		}

		if (ParentMeshComponent)
		{
			srcMannequinMesh = ParentMeshComponent;
		}
	}

}


void FAnimNode_VrmRetargetFromMannequin::UpdateCache(FComponentSpacePoseContext& Output) {

	if (srcMannequinMesh == nullptr){
		srcSkeletalMesh = nullptr;
		return;
	}

	if (srcSkeletalMesh == VRMGetSkeletalMeshAsset(srcMannequinMesh)) {
		return;
	}

	srcSkeletalMesh = VRMGetSkeletalMeshAsset(srcMannequinMesh);

	if (srcSkeletalMesh == nullptr) return;
	if (dstSkeletalMesh == nullptr) return;

	auto& srcRefSkeleton = VRMGetRefSkeleton(srcSkeletalMesh);
	auto& dstRefSkeleton = VRMGetRefSkeleton(dstSkeletalMesh);

	// ref pose
	const auto& srcRefSkeletonTransform = srcRefSkeleton.GetRefBonePose();
	const auto& dstRefSkeletonTransform = dstRefSkeleton.GetRefBonePose();

	// ref component
	srcRefSkeletonCompTransform = srcRefSkeletonTransform;
	dstRefSkeletonCompTransform = dstRefSkeletonTransform;
	for (int i = 1; i < srcRefSkeletonCompTransform.Num(); ++i) {
		int parent = srcRefSkeleton.GetParentIndex(i);
		srcRefSkeletonCompTransform[i] *= srcRefSkeletonCompTransform[parent];
	}
	for (int i = 1; i < dstRefSkeletonCompTransform.Num(); ++i) {
		int parent = dstRefSkeleton.GetParentIndex(i);
		dstRefSkeletonCompTransform[i] *= dstRefSkeletonCompTransform[parent];
	}

	// current A-pose compTransform
	dstCurrentSkeletonCompTransform = dstRefSkeletonTransform;
	for (int i = 0; i < dstRefSkeletonCompTransform.Num(); ++i) {
		FCompactPoseBoneIndex ii(i);

		auto t = Output.Pose.GetLocalSpaceTransform(ii);
		if (i == 0) {
			dstCurrentSkeletonCompTransform[i] = t;
		} else {
			int parent = dstRefSkeleton.GetParentIndex(i);
			dstCurrentSkeletonCompTransform[i] = t * dstCurrentSkeletonCompTransform[parent];
		}
	}
}

void FAnimNode_VrmRetargetFromMannequin::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
	Super::Initialize_AnyThread(Context);

	dstSkeletalMesh = nullptr;
	srcSkeletalMesh = nullptr;

	if (Context.AnimInstanceProxy == nullptr) return;
	if (Context.AnimInstanceProxy->GetSkelMeshComponent() == nullptr) return;

	dstSkeletalMesh = VRMGetSkinnedAsset(Context.AnimInstanceProxy->GetSkelMeshComponent());
	srcSkeletalMesh = nullptr;
}
void FAnimNode_VrmRetargetFromMannequin::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
	Super::CacheBones_AnyThread(Context);

}

#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
void FAnimNode_VrmRetargetFromMannequin::ResetDynamics(ETeleportType InTeleportType) {
	Super::ResetDynamics(InTeleportType);
}
#endif

void FAnimNode_VrmRetargetFromMannequin::UpdateInternal(const FAnimationUpdateContext& Context){
	Super::UpdateInternal(Context);
}


void FAnimNode_VrmRetargetFromMannequin::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneToModify.BoneName.ToString());
	//DebugLine += FString::Printf(TEXT(" Target: %s)"), *BoneNameToModify.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_VrmRetargetFromMannequin::EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output) {
	Super::EvaluateComponentPose_AnyThread(Output);
}

void FAnimNode_VrmRetargetFromMannequin::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	UpdateCache(Output);

	if (srcSkeletalMesh == nullptr) return;
	if (dstSkeletalMesh == nullptr) return;

	const UVrmMetaObject* dstMeta = VrmMetaObject;
	if (dstMeta == nullptr) {
		return;
	}

	auto srcSkeletalMeshComp = Cast<USkeletalMeshComponent>(srcMannequinMesh);
	auto srcAsSkinnedMeshComp = srcMannequinMesh;
	if (srcAsSkinnedMeshComp == nullptr) {
		return;
	}

	auto &srcRefSkeleton = VRMGetRefSkeleton(VRMGetSkeletalMeshAsset(srcAsSkinnedMeshComp) );
	auto &dstRefSkeleton = VRMGetRefSkeleton(VRMGetSkeletalMeshAsset(Output.AnimInstanceProxy->GetSkelMeshComponent()) );

	// ref pose
	const auto& dstRefSkeletonTransform = dstRefSkeleton.GetRefBonePose();
	const auto& srcRefSkeletonTransform = srcRefSkeleton.GetRefBonePose();

	const TArray<FString>* SrcBoneList = &VRMUtil::vrm_humanoid_bone_list;

	// is ue5 mannequin remap
	enum class RemapType {
		E_None,
		E_UpperChest,
		E_Chest,
	};
	RemapType remapType = RemapType::E_None;

	{
		bool bUE5Mannequin = false;
		for (int i = 1; i <= 5; ++i) {
			int index = srcAsSkinnedMeshComp->GetBoneIndex(*(FString(TEXT("spine_0"))+FString::FromInt(i)));
			if (index == INDEX_NONE) {
				break;
			}
			if (i == 5) {
				bUE5Mannequin = true;
			}
		}

		if (bUE5Mannequin) {
			TArray<FString>table = { TEXT("spine"), TEXT("chest"), TEXT("upperChest") };
			bool b[3] = {false, false, false};
			for (int i = 0; i < 3; ++i) {
				auto* a = dstMeta->humanoidBoneTable.Find(*table[i]);
				if (a && (*a) != "") {
					b[i] = true;
				}
			}
			if (b[0] && b[1] && b[2]) {
				remapType = RemapType::E_UpperChest;
			}
			if (b[0] && b[1] && b[2]==false) {
				remapType = RemapType::E_Chest;
			}
		}
	}


	int BoneCount = 0;
	for (const auto& humanoidName : VRMUtil::vrm_humanoid_bone_list) {
		FName srcName, dstName;
		srcName = dstName = NAME_None;

		++BoneCount;
		{
			for (auto& bonemap : VRMUtil::table_ue4_vrm) {
				if (bonemap.BoneVRM == humanoidName) {
					srcName = *bonemap.BoneUE4;

					// replace src bone name by srcMeta object
					if (MannequinVrmMetaObject) {
						auto *p = MannequinVrmMetaObject->humanoidBoneTable.Find(bonemap.BoneVRM);
						if (p) {
							srcName = **p;
						}
					}
					break;
				}
			}
		}
		if (srcName == NAME_None) {
			continue;
		}

		switch (remapType) {
		case RemapType::E_UpperChest:
			if (humanoidName == TEXT("upperChest")) {
				srcName = TEXT("spine_05");
			}
			if (humanoidName == TEXT("chest")) {
				srcName = TEXT("spine_03");
				//3,4
			}
			if (humanoidName == TEXT("spine")) {
				srcName = TEXT("spine_01");
				//1,2
			}
			break;
		case RemapType::E_Chest:
			if (humanoidName == TEXT("chest")) {
				srcName = TEXT("spine_05");
			}
			if (humanoidName == TEXT("spine")) {
				srcName = TEXT("spine_01");
			}
			break;
		}

		{
			auto a = dstMeta->humanoidBoneTable.Find(humanoidName);
			if (a) {
				dstName = **a;
			}
		}
		if (dstName == NAME_None) {
			continue;
		}

		auto dstIndex = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetBoneIndex(dstName);
		if (dstIndex < 0) {
			continue;
		}
		auto srcIndex = srcAsSkinnedMeshComp->GetBoneIndex(srcName);
		if (srcIndex < 0) {
			continue;
		}

		FCompactPoseBoneIndex srcPoseBoneIndex(srcIndex);
		FCompactPoseBoneIndex dstPoseBoneIndex(dstIndex);


		const auto srcCurrentTrans = srcAsSkinnedMeshComp->GetSocketTransform(srcName, RTS_Component);

		auto modelBone = srcCurrentTrans;
		{
			
			{
				if (bIgnoreCenterLocation) {
					modelBone.SetLocation(CenterLocationOffset);
				} else {
					auto dd = Output.Pose.GetComponentSpaceTransform(dstPoseBoneIndex);
					modelBone.SetLocation(dd.GetLocation());

					if (BoneCount == 1) {
						// RootBone transform
						float s = dd.GetLocation().Z / FMath::Max(1.0f, srcRefSkeletonCompTransform[srcIndex].GetLocation().Z);
#if	UE_VERSION_OLDER_THAN(4,24,0)
						FVector ComponentScale = Output.AnimInstanceProxy->GetSkelMeshComponent()->RelativeScale3D;
#else
						FVector ComponentScale = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetRelativeScale3D();
#endif
						if (ComponentScale.X == 0.f) {
							ComponentScale.X = 0.001f;
						}
						if (ComponentScale.Y == 0.f) {
							ComponentScale.Y = 0.001f;
						}
						if (ComponentScale.Z == 0.f) {
							ComponentScale.Z = 0.001f;
						}

						modelBone.SetLocation(
							FMath::Lerp(
								srcCurrentTrans.GetLocation(),		// original location
								srcCurrentTrans.GetLocation() * s,	// scled by height
								CenterLocationScaleByHeightScale)
							/ ComponentScale	// inv model scale
							+ CenterLocationOffset
						);
					} // RootBone transform
				}
			}

			modelBone.SetRotation(
				srcCurrentTrans.GetRotation()
				* srcRefSkeletonCompTransform[srcIndex].GetRotation().Inverse()
				* dstCurrentSkeletonCompTransform[dstIndex].GetRotation()
			);

			Output.Pose.SetComponentSpaceTransform(dstPoseBoneIndex, modelBone);
		}
	}
}

bool FAnimNode_VrmRetargetFromMannequin::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	//return (BoneToModify.IsValidToEvaluate(RequiredBones));
	return true;
}

void FAnimNode_VrmRetargetFromMannequin::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	//BoneToModify.Initialize(RequiredBones);
}

void FAnimNode_VrmRetargetFromMannequin::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp, bool bPreviewForeground) const
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
