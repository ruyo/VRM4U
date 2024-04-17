// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.



#include "VrmAnimInstance.h"
#include "VrmMetaObject.h"
#include "VrmUtil.h"
#include "Animation/AnimNodeBase.h"
#include "BoneControllers/AnimNode_Fabrik.h"
#include "BoneControllers/AnimNode_TwoBoneIK.h"
#include "BoneControllers/AnimNode_SplineIK.h"
#include "Misc/EngineVersionComparison.h"


void FVrmAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance) {
}
bool FVrmAnimInstanceProxy::Evaluate(FPoseContext& Output) {
	Output.ResetToRefPose();

	UVrmAnimInstance *animInstance = Cast<UVrmAnimInstance>(GetAnimInstanceObject());
	if (animInstance == nullptr) {
		return false;
	}

	/*
	enum BoneTarget{
		E_Hand_L,
		E_UpperArm_L,
		E_Hand_R,
		E_UpperArm_R,
		E_Head,
		E_Spine,

		E_MAX,
	};

	// boneName
	FString targetBoneTable[] = {
		TEXT("leftHand"),
		TEXT("leftUpperArm"),
		TEXT("rightHand"),
		TEXT("rightUpperArm"),
		TEXT("head"),
		TEXT("spine"),
	};
	{
		const UVrmMetaObject *meta = animInstance->MetaObject;
		if (meta) {
			for (auto &humanoidName : targetBoneTable) {
				for (auto &modelName : meta->humanoidBoneTable) {
					if (humanoidName.Compare(modelName.Key, ESearchCase::IgnoreCase) != 0) {
						continue;
					}
					// bone rename
					humanoidName = modelName.Value;
					break;
				}
			}
		}
	}

	// tracking point
	FTransform targetTracking[] = {
		animInstance->TransHandLeft,
		animInstance->TransHandRight,
		animInstance->TransHead,
	};
	{
		const USceneComponent *targetComponent[] = {
			animInstance->ComponentHandLeft,
			animInstance->ComponentHandRight,
			animInstance->ComponentHead,
		};

		for (int i = 0; i < 3; ++i) {
			if (targetComponent[i] == nullptr) continue;
			targetTracking[i] = targetComponent[i]->GetComponentTransform();
		}
	}

	// mainBone Transform
	//FTeransform boneTransform[E_MAX] = {

	//};



	USkeletalMeshComponent *srcMesh = animInstance->BaseSkeletalMeshComponent;
	if (srcMesh == nullptr) {
	//	return false;
	}

	const auto &RefSkeletonTransform = GetSkelMeshComponent()->SkeletalMesh->RefSkeleton.GetRefBonePose();

	auto &pose = Output.Pose;

	FComponentSpacePoseContext ComponentSpacePoseContext(Output.AnimInstanceProxy);
	ComponentSpacePoseContext.Pose.InitPose(Output.Pose);

	for (int i = 2; i >=0 ; --i) {

		if (targetTracking[i].GetLocation().Size() == 0) {
			continue;
		}

		FAnimNode_Fabrik f;
		{

			f.EffectorTransformSpace = EBoneControlSpace::BCS_WorldSpace;
			f.EffectorTransform = targetTracking[i];

			//f.EffectorTarget;

			f.EffectorRotationSource = EBoneRotationSource::BRS_CopyFromTarget;

			f.TipBone.BoneName = *(targetBoneTable[i * 2]);
			f.TipBone.Initialize(this->GetSkeleton());

			f.RootBone.BoneName = *(targetBoneTable[i * 2 + 1]);
			f.RootBone.Initialize(this->GetSkeleton());

			f.Precision = 1.f;
			f.MaxIterations = 10;
			f.bEnableDebugDraw = false;
		}
		FAnimNode_TwoBoneIK t;
		{
			t.IKBone.BoneName = *(targetBoneTable[i * 2]);

			t.bAllowStretching = true;
			t.StartStretchRatio = 1.f;
			t.MaxStretchScale = 1.1f;
			//t.bTakeRotationFromEffectorSpace = false;
			t.bMaintainEffectorRelRot = false;
			t.bAllowTwist = true;

			t.EffectorLocationSpace = EBoneControlSpace::BCS_WorldSpace;

			t.EffectorLocation = targetTracking[i].GetLocation();

			//f.EffectorTarget;

			t.JointTargetLocationSpace = EBoneControlSpace::BCS_WorldSpace;

			const USceneComponent *tmp[] = {
				animInstance->ComponentHandJointTargetLeft,
				animInstance->ComponentHandJointTargetRight,
				nullptr,
			};
			if (tmp[i]) {
				t.JointTargetLocation = tmp[i]->GetComponentLocation();
			}
			//f.JointTarget;
		}
		FAnimNode_SplineIK s;
		{
			s.StartBone.BoneName = *targetBoneTable[E_Spine];
			s.StartBone.Initialize(this->GetSkeleton());
			s.EndBone.BoneName = *targetBoneTable[E_Head];
			s.EndBone.Initialize(this->GetSkeleton());

			s.BoneAxis = ESplineBoneAxis::Y;

			s.bAutoCalculateSpline = false;

			s.PointCount = 2;

			s.ControlPoints.SetNum(2);
			s.ControlPoints[0].SetIdentity();
			s.ControlPoints[1] = targetTracking[2];
			s.ControlPoints[1].SetToRelativeTransform(GetComponentTransform());

			//ComponentSpacePoseContext.

			s.Roll = 0.f;

			s.TwistStart = 0.f;

			s.TwistEnd = 0.f;

			//s.TwistBlend;

			s.Stretch = 0.01f;

			s.Offset = 0.f;
		}

		{

			//ApplyBoneControllers(GetCurveBoneControllers(), ComponentSpacePoseContext);
			//ApplyBoneControllers(BoneControllers, ComponentSpacePoseContext);
			if (USkeleton* LocalSkeleton = ComponentSpacePoseContext.AnimInstanceProxy->GetSkeleton())
			{
				//for (auto& SingleBoneController : InBoneControllers)
				if (i != 2) {
					auto &SingleBoneController = t;

					TArray<FBoneTransform> BoneTransforms;
					FAnimationCacheBonesContext Proxy(this);
					SingleBoneController.CacheBones_AnyThread(Proxy);
					if (SingleBoneController.IsValidToEvaluate(LocalSkeleton, ComponentSpacePoseContext.Pose.GetPose().GetBoneContainer()))
					{
						SingleBoneController.EvaluateSkeletalControl_AnyThread(ComponentSpacePoseContext, BoneTransforms);
						if (BoneTransforms.Num() > 0)
						{
							ComponentSpacePoseContext.Pose.LocalBlendCSBoneTransforms(BoneTransforms, 1.0f);
						}
					}
				} else {
					auto &SingleBoneController = s;

					TArray<FBoneTransform> BoneTransforms;
					FAnimationCacheBonesContext Proxy(this);
					SingleBoneController.CacheBones_AnyThread(Proxy);
					if (SingleBoneController.IsValidToEvaluate(LocalSkeleton, ComponentSpacePoseContext.Pose.GetPose().GetBoneContainer()))
					{
						SingleBoneController.EvaluateSkeletalControl_AnyThread(ComponentSpacePoseContext, BoneTransforms);
						if (BoneTransforms.Num() > 0)
						{
							ComponentSpacePoseContext.Pose.LocalBlendCSBoneTransforms(BoneTransforms, 1.0f);
						}
					}
				}
			}
		}
	}
	//
#if	UE_VERSION_OLDER_THAN(4,21,0)
	ComponentSpacePoseContext.Pose.ConvertToLocalPoses(Output.Pose);
#endif

	for (int i=0; i<3; ++i){
		auto b = GetSkelMeshComponent()->GetBoneIndex(*targetBoneTable[i*2]);
		if (b == INDEX_NONE) {
			continue;
		}
		FCompactPose::BoneIndexType bi(b);
		auto t = Output.Pose[bi];
		auto tmp = targetTracking[i];
		//tmp.SetToRelativeTransform(GetComponentTransform());
		//t.SetRotation( GetComponentTransform().GetRotation().Inverse() * (tmp.GetRotation()) );

		
		//const FTransform& BoneTM = ComponentSpacePoseContext.Pose.GetComponentSpaceTransform(bi);
		t.SetRotation( t.GetRotation() * (tmp.GetRotation()) );
		Output.Pose[bi] = t;
	}
	*/




/*
	for (auto &boneName : meta->humanoidBoneTable) {

		if (boneName.Key == TEXT("leftEye") || boneName.Key == TEXT("rightEye")) {
			continue;
		}
		//auto i = srcMesh->GetBoneIndex();
		if (srcMesh->GetBoneIndex(*(boneName.Key)) < 0) {
			continue;
		}

		auto t = srcMesh->GetSocketTransform(*(boneName.Key), RTS_ParentBoneSpace);
		
		auto i = GetSkelMeshComponent()->GetBoneIndex(*(boneName.Value));
		if (i < 0) {
			continue;
		}
		auto refLocation = RefSkeletonTransform[i].GetLocation();
		
		FVector newLoc = t.GetLocation();
		if (newLoc.Normalize()) {
			newLoc *= refLocation.Size();
			t.SetLocation(newLoc);
		}

		FCompactPose::BoneIndexType bi(i);
		pose[bi] = t;
	}
*/
	return true;
}

#if	UE_VERSION_OLDER_THAN(4,24,0)
void FVrmAnimInstanceProxy::UpdateAnimationNode(float DeltaSeconds) {
}
#else
void FVrmAnimInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext){
}
#endif

/////

UVrmAnimInstance::UVrmAnimInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FAnimInstanceProxy* UVrmAnimInstance::CreateAnimInstanceProxy() {

	return new FVrmAnimInstanceProxy(this);
}


void UVrmAnimInstance::NativeInitializeAnimation() {
}
void UVrmAnimInstance::NativeUpdateAnimation(float DeltaSeconds) {

	{
		if (BaseSkeletalMeshComponent == nullptr) {
			return;
		}
		auto a = BaseSkeletalMeshComponent->GetAnimInstance();
		if (a == nullptr) {
			return;
		}

		if (VRMGetSkinnedAsset(BaseSkeletalMeshComponent) == nullptr) {
			return;
		}

		a->CurrentSkeleton = VRMGetSkeleton( VRMGetSkinnedAsset(BaseSkeletalMeshComponent) );
	}
}
void UVrmAnimInstance::NativePostEvaluateAnimation() {
	if (BaseSkeletalMeshComponent) {
		if (BaseSkeletalMeshComponent->AnimScriptInstance) {
			IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(this->GetClass());
			//const USkeleton* AnimSkeleton = (AnimClassInterface) ? AnimClassInterface->GetTargetSkeleton() : nullptr;
			if (AnimClassInterface) {
				//AnimClassInterface->
			}
			//BaseSkeletalMeshComponent->AnimScriptInstance->target
			//BaseSkeletalMeshComponent->AnimScriptInstance->CurrentSkeleton = 
				//BaseSkeletalMeshComponent->SkeletalMesh->Skeleton;

		}
	}
}
void UVrmAnimInstance::NativeUninitializeAnimation() {
}
void UVrmAnimInstance::NativeBeginPlay() {
}

namespace {

	static TArray<FString> shapeBlend = {
		"Neutral","A","I","U","E","O","Blink","Joy","Angry","Sorrow","Fun","LookUp","LookDown","LookLeft","LookRight","Blink_L","Blink_R",
	};
}


void UVrmAnimInstance::SetMorphTargetVRM(EVRMBlendShapeGroup type, float Value) {

	if (MetaObject == nullptr){
		return;
	}

	USkeletalMeshComponent *skc = GetOwningComponent();
	//auto &morphMap = skc->GetMorphTargetCurves();
	for (auto &a : MetaObject->BlendShapeGroup) {
		if (a.name != shapeBlend[(int)type]) {
			continue;
		}

		for (auto &b : a.BlendShape) {
			//b.meshName

			SetMorphTarget(*(b.morphTargetName), Value);

			/*
			for (auto &m : skc->SkeletalMesh->MorphTargets) {
				auto s = m->GetName();
				//if (s.Find(b.meshName) < 0) {
				//	continue;
				//}
				auto s1 = FString::Printf(TEXT("%02d_"), b.meshID);
				if (s.Find(s1) < 0) {
					continue;
				}
				auto s2 = FString::Printf(TEXT("_%02d"), b.shapeIndex);
				if (s.Find(s2) < 0) {
					continue;
				}
				SetMorphTarget(*s, Value);
			}
			*/
		}
	}


}

void UVrmAnimInstance::SetVrmData(USkeletalMeshComponent *baseSkeletalMesh, UVrmMetaObject *meta) {
	IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(this->GetClass());
	//const USkeleton* AnimSkeleton = (AnimClassInterface) ? AnimClassInterface->GetTargetSkeleton() : nullptr;

	USkeletalMeshComponent *skc = GetOwningComponent();

	skc->SetSkeletalMesh(meta->SkeletalMesh);
	BaseSkeletalMeshComponent = baseSkeletalMesh;
	MetaObject = meta;

	//if (AnimClassInterface) {
	//	AnimClassInterface->
	//}
	//BaseSkeletalMeshComponent->AnimScriptInstance->target
	//BaseSkeletalMeshComponent->AnimScriptInstance->CurrentSkeleton = 
	//BaseSkeletalMeshComponent->SkeletalMesh->Skeleton;
}
