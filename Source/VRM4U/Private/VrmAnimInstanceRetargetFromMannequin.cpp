// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "VrmAnimInstanceRetargetFromMannequin.h"

#if	UE_VERSION_OLDER_THAN(5,2,0)
#else

#include "VrmMetaObject.h"
#include "VrmAssetListObject.h"
#include "VrmUtil.h"
#include "VrmBPFunctionLibrary.h"
#include "Animation/AnimNodeBase.h"

//#include "BoneControllers/AnimNode_Fabrik.h"
//#include "BoneControllers/AnimNode_TwoBoneIK.h"
//#include "BoneControllers/AnimNode_SplineIK.h"
#include "AnimNode_VrmSpringBone.h"
#include "AnimNode_VrmConstraint.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"

#include "VrmRigHeader.h"

#if	UE_VERSION_OLDER_THAN(5,4,0)
#include "Animation/Rig.h"
#endif


namespace {
	template<class BaseType, class PoseType>
	static void ConvertToLocalPoses2(const BaseType &basePose, PoseType& OutPose)
	{
		checkSlow(basePose.GetPose().IsValid());
		OutPose = basePose.GetPose();

		// now we need to convert back to local bases
		// only convert back that has been converted to mesh base
		// if it was local base, and if it hasn't been modified
		// that is still okay even if parent is changed, 
		// that doesn't mean this local has to change
		// go from child to parent since I need parent inverse to go back to local
		// root is same, so no need to do Index == 0
		const FCSPose<FCompactPose>::BoneIndexType RootBoneIndex(0);
		if (basePose.GetComponentSpaceFlags()[RootBoneIndex])
		{
			OutPose[RootBoneIndex] = basePose.GetPose()[RootBoneIndex];
		}

		const int32 NumBones = basePose.GetPose().GetNumBones();
		for (int32 Index = NumBones - 1; Index > 0; Index--)
		{
			const FCSPose<FCompactPose>::BoneIndexType BoneIndex(Index);
			if (basePose.GetComponentSpaceFlags()[BoneIndex])
			{
				const FCSPose<FCompactPose>::BoneIndexType ParentIndex = basePose.GetPose().GetParentBoneIndex(BoneIndex);
				OutPose[BoneIndex].SetToRelativeTransform(OutPose[ParentIndex]);
				OutPose[BoneIndex].NormalizeRotation();
			}
		}
	}
}

FVrmAnimInstanceRetargetFromMannequinProxy::FVrmAnimInstanceRetargetFromMannequinProxy()
{
}

FVrmAnimInstanceRetargetFromMannequinProxy::FVrmAnimInstanceRetargetFromMannequinProxy(UAnimInstance* InAnimInstance)
	: FAnimInstanceProxy(InAnimInstance)
{

}


void FVrmAnimInstanceRetargetFromMannequinProxy::Initialize(UAnimInstance* InAnimInstance) {
	Super::Initialize(InAnimInstance);
}

void FVrmAnimInstanceRetargetFromMannequinProxy::CustomInitialize() {

	if (Node_SpringBone.Get() == nullptr) {
		Node_SpringBone = MakeShareable(new FAnimNode_VrmSpringBone());
	}
	if (Node_Constraint.Get() == nullptr) {
		Node_Constraint = MakeShareable(new FAnimNode_VrmConstraint());
	}

	if (Node_Retarget.Get() == nullptr) {
		Node_Retarget = MakeShareable(new FAnimNode_RetargetPoseFromMesh());
		FAnimationInitializeContext InitContext(this);
		Node_Retarget->Initialize_AnyThread(InitContext);
	}
}

bool FVrmAnimInstanceRetargetFromMannequinProxy::Evaluate(FPoseContext& Output) {

	//Output.Pose.CopyBonesFrom(CachedPose);

	UVrmAnimInstanceRetargetFromMannequin* animInstance = Cast<UVrmAnimInstanceRetargetFromMannequin>(GetAnimInstanceObject());
	if (animInstance == nullptr) {
		return false;
	}
	if (animInstance->DstVrmAssetList == nullptr) {
		if (IsInGameThread())
		{
			//FMessageLog("AnimBlueprintLog").Warning(
			// FText::Format(LOCTEXT("AnimInstance_SlotNode", "SLOTNODE: '{0}' in animation instance class {1} already exists. Remove duplicates from the animation graph for this class."),
			// FText::FromString(SlotNodeName.ToString()), FText::FromString(ClassNameString)));
			//FMessageLog("AnimBlueprintLog").Warning(FText::Format("UVrmAnimInstanceRetargetFromMannequin")));
		}
		else
		{
			//UE_LOG(LogAnimation, Warning, TEXT("SLOTNODE: '%s' in animation instance class %s already exists. Remove duplicates from the animation graph for this class."), *SlotNodeName.ToString(), *ClassNameString);
		}
		UE_LOG(LogAnimation, Warning, TEXT("UVrmAnimInstanceRetargetFromMannequin:: no DestVrmAssetList"));
		return false;
	}

	const UVrmMetaObject *dstMeta = animInstance->DstVrmAssetList->VrmMetaObject;
	if (dstMeta) {
		if (animInstance->DstVrmMetaForCustomSpring) {
			dstMeta = animInstance->DstVrmMetaForCustomSpring;
		}
	}

	if (dstMeta == nullptr) {
		UE_LOG(LogAnimation, Warning, TEXT("UVrmAnimInstanceRetargetFromMannequin:: no DstMeta"));
		return false;
	}

	// morph copy
	if (animInstance->SrcSkeletalMeshComponent){
		if (animInstance->SrcSkeletalMeshComponent->GetAnimInstance()) {
			EAnimCurveType types[] = {
				EAnimCurveType::AttributeCurve,
				EAnimCurveType::MaterialCurve,
				EAnimCurveType::MorphTargetCurve,
			};

			for (int i = 0; i < 3; ++i) {
				const TMap<FName, float>& t = animInstance->SrcSkeletalMeshComponent->GetAnimInstance()->GetAnimationCurveList(types[i]);
				for (auto& a : t) {
					GetSkelMeshComponent()->SetMorphTarget(a.Key, a.Value, true);
				}
			}
		}
	}

#if	UE_VERSION_OLDER_THAN(5,2,0)
#else

	//retarget
	if (bUseRetargeter && Retargeter && Node_Retarget.Get()) {

		if (Node_Retarget.Get()) {
			auto node = Node_Retarget.Get();

			FAnimationCacheBonesContext c(this);
			node->CacheBones_AnyThread(c);

			node->Evaluate_AnyThread(Output);
		}
	} else {
		Node_Retarget = nullptr;
	}
#endif



	// constraint
	if (bIgnoreVRMConstraint == false) {
		bool bCalc = true;
		{
			bool bPlay, bSIE, bEditor;
			bPlay = bSIE = bEditor = true;
			UVrmBPFunctionLibrary::VRMGetPlayMode(bPlay, bSIE, bEditor);
			if (bPlay == false) {
				bCalc = false;
			}
		}

		if (bCalc) {
			if (Node_Constraint.Get() == nullptr) {
				Node_Constraint = MakeShareable(new FAnimNode_VrmConstraint());
			}

			if (Node_Constraint.Get()) {
				if (animInstance->PendingDynamicResetTeleportType != ETeleportType::None)
				{
					Node_Constraint->ResetDynamics(animInstance->PendingDynamicResetTeleportType);
				}

				auto& constraint = *Node_Constraint.Get();

				constraint.VrmMetaObject_Internal = dstMeta;
				constraint.bCallByAnimInstance = true;

				FAnimationInitializeContext InitContext(this);

				if (constraint.	bCallInitialized  == false) {
					constraint.Initialize_AnyThread(InitContext);
					constraint.ComponentPose.SetLinkNode(&constraint);
				}

				{
					FComponentSpacePoseContext InputCSPose(this);
					InputCSPose.Pose.InitPose(Output.Pose);
					constraint.EvaluateComponentSpace_AnyThread(InputCSPose);
					ConvertToLocalPoses2(InputCSPose.Pose, Output.Pose);
				}

			}
		}
	}
	else {
		Node_Constraint = nullptr;
	}



	if (bIgnoreVRMSwingBone == false) {
	//if (0){
		bool bCalc = true;
		{
			bool bPlay, bSIE, bEditor;
			bPlay = bSIE = bEditor = true;
			UVrmBPFunctionLibrary::VRMGetPlayMode(bPlay, bSIE, bEditor);
			if (bPlay == false) {
				bCalc = false;
			}
		}

		if (bCalc) {
			if (Node_SpringBone.Get() == nullptr) {
				Node_SpringBone = MakeShareable(new FAnimNode_VrmSpringBone());
			}

			if (Node_SpringBone.Get()) {
				if(animInstance->PendingDynamicResetTeleportType != ETeleportType::None)
				{
					Node_SpringBone->ResetDynamics(animInstance->PendingDynamicResetTeleportType);
				}

				auto& springBone = *Node_SpringBone.Get();

				springBone.VrmMetaObject_Internal = dstMeta;
				springBone.bCallByAnimInstance = true;
				springBone.CurrentDeltaTime = CurrentDeltaTime;

				FAnimationInitializeContext InitContext(this);

				if (springBone.IsSpringInit() == false) {
					springBone.Initialize_AnyThread_local(InitContext);
					springBone.ComponentPose.SetLinkNode(&springBone);
					CalcCount = 0;
					springBone.CurrentDeltaTime = 1.f / 20.f;
				}
				if (CalcCount < 3) {
					FVector bak = springBone.gravityAdd;
					++CalcCount;
					FComponentSpacePoseContext InputCSPose(this);
					for (int i = 0; i < 20; ++i) {
						InputCSPose.Pose.InitPose(Output.Pose);
						springBone.EvaluateComponentSpace_AnyThread(InputCSPose);
						ConvertToLocalPoses2(InputCSPose.Pose, Output.Pose);
					}
					springBone.gravityAdd = bak;

				} else {
					FComponentSpacePoseContext InputCSPose(this);
					InputCSPose.Pose.InitPose(Output.Pose);
					springBone.EvaluateComponentSpace_AnyThread(InputCSPose);
					ConvertToLocalPoses2(InputCSPose.Pose, Output.Pose);
				}

			}
		}
	} else {
		Node_SpringBone = nullptr;
	}

	return true;
}
void FVrmAnimInstanceRetargetFromMannequinProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext) {
	CurrentDeltaTime = InContext.GetDeltaTime();
	CustomInitialize();

}

void FVrmAnimInstanceRetargetFromMannequinProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) {
	Super::PreUpdate(InAnimInstance, DeltaSeconds);


	if (Node_Retarget.Get()) {
		auto node = Node_Retarget.Get();

		node->IKRetargeterAsset = Retargeter.Get();
		//node->SourceMeshComponent = InAnimInstance->SrcSkeletalMeshComponent; // null ok

		FAnimationInitializeContext InitContext(this);

		if (node->GetRetargetProcessor() == nullptr) {
			node->Initialize_AnyThread(InitContext);
		}

		node->PreUpdate(InAnimInstance);
	}
}

/////

UVrmAnimInstanceRetargetFromMannequin::UVrmAnimInstanceRetargetFromMannequin(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	bUsingCopyPoseFromMesh = true;
	bUseMultiThreadedAnimationUpdate = false;
}

FAnimInstanceProxy* UVrmAnimInstanceRetargetFromMannequin::CreateAnimInstanceProxy() {
	myProxy = new FVrmAnimInstanceRetargetFromMannequinProxy(this);
	myProxy->bIgnoreVRMSwingBone = bIgnoreVRMSwingBone;
	myProxy->bIgnoreWindDirectionalSource = bIgnoreWindDirectionalSource;
	return myProxy;
}


void UVrmAnimInstanceRetargetFromMannequin::NativeInitializeAnimation() {
	for (int i = 0; i < FMath::Min(InitialMorphValue.Num(), InitialMorphName.Num()); ++i) {
		SetMorphTarget(InitialMorphName[i], InitialMorphValue[i]);
	}
}
void UVrmAnimInstanceRetargetFromMannequin::NativeUpdateAnimation(float DeltaSeconds) {
}
void UVrmAnimInstanceRetargetFromMannequin::NativePostEvaluateAnimation() {
}

void UVrmAnimInstanceRetargetFromMannequin::NativeUninitializeAnimation() {
	if (GetOwningComponent()) {
		GetOwningComponent()->ClearMorphTargets();
	}

	if (!SrcSkeletalMeshComponent && bUseAttachedParent)
	{
		USkeletalMeshComponent* TargetMesh = GetOwningComponent();

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
			SrcSkeletalMeshComponent = ParentMeshComponent;
		}
	}
	if (SrcAsSkinnedMeshComponent == nullptr) {
		SrcAsSkinnedMeshComponent = SrcSkeletalMeshComponent;
	}
}
void UVrmAnimInstanceRetargetFromMannequin::NativeBeginPlay() {
	if (GetOwningComponent()) {
		GetOwningComponent()->ClearMorphTargets();
	}
}

void UVrmAnimInstanceRetargetFromMannequin::PreUpdateAnimation(float DeltaSeconds) {
	Super::PreUpdateAnimation(DeltaSeconds);
}

void UVrmAnimInstanceRetargetFromMannequin::SetRetargetData(bool bUseRetargeter, UIKRetargeter* IKRetargeter) {
	myProxy->Retargeter = IKRetargeter;
	myProxy->bUseRetargeter = bUseRetargeter;
}


void UVrmAnimInstanceRetargetFromMannequin::SetVrmAssetList(UVrmAssetListObject *dstAssetList) {
	DstVrmAssetList = dstAssetList;
}

void UVrmAnimInstanceRetargetFromMannequin::SetSkeletalMeshCopyDataForCustomSpring(UVrmMetaObject *dstMetaForCustomSpring) {
	DstVrmMetaForCustomSpring = dstMetaForCustomSpring;
}



void UVrmAnimInstanceRetargetFromMannequin::SetVrmSpringBoneParam(float gravityScale, FVector gravityAdd, float stiffnessScale, float stiffnessAdd, float randomWindRange) {
	if (myProxy == nullptr) return;
	auto a = myProxy->Node_SpringBone.Get();
	if (a == nullptr) return;

	a->gravityScale = gravityScale;
	a->gravityAdd = gravityAdd;
	a->stiffnessScale = stiffnessScale;
	a->stiffnessAdd = stiffnessAdd;
	a->randomWindRange = randomWindRange;
}

void UVrmAnimInstanceRetargetFromMannequin::SetVrmSpringBoneBool(bool bIgnoreVrmSpringBone, bool bIgnorePhysicsCollision, bool bIgnoreVRMCollision, bool bIgnoreWind) {
	if (myProxy == nullptr) return;
	auto a = myProxy->Node_SpringBone.Get();
	if (a == nullptr) return;

	this->bIgnoreVRMSwingBone = bIgnoreVrmSpringBone;
	myProxy->bIgnoreVRMSwingBone = bIgnoreVrmSpringBone;
	a->bIgnorePhysicsCollision = bIgnorePhysicsCollision;
	a->bIgnoreVRMCollision = bIgnoreVRMCollision;
	a->bIgnoreWindDirectionalSource = bIgnoreWind;
}

void UVrmAnimInstanceRetargetFromMannequin::SetVrmSpringBoneIgnoreWingBone(const TArray<FName> &boneNameList) {
	if (myProxy == nullptr) return;
	auto a = myProxy->Node_SpringBone.Get();
	if (a == nullptr) return;

	a->NoWindBoneNameList = boneNameList;
}


#endif // 5.3