// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmHMDTrackingComponent.h"

#if WITH_VRM4U_HMD_TRACKER
#include "Misc/EngineVersionComparison.h"

#include "../Private/OculusHMDModule.h"
#include "../Private/OculusHMD.h"
#include "../Private/OculusHMD_Settings.h"

namespace VRM4ULocal {
	FORCEINLINE FQuat ToFQuat(const ovrpQuatf& InQuat)
	{
		return FQuat(-InQuat.z, InQuat.x, InQuat.y, -InQuat.w);
	}

	/** Converts FQuat to ovrpQuatf */
	FORCEINLINE ovrpQuatf ToOvrpQuatf(const FQuat& InQuat)
	{
		return ovrpQuatf{ InQuat.Y, InQuat.Z, -InQuat.X, -InQuat.W };
	}

	/** Converts vector from Oculus to Unreal */
	FORCEINLINE FVector ToFVector(const ovrpVector3f& InVec)
	{
		return FVector(-InVec.z, InVec.x, InVec.y);
	}
}

#endif


UVrmHMDTrackingComponent::UVrmHMDTrackingComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	//PrimaryComponentTick.TickGroup;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}

void UVrmHMDTrackingComponent::OnRegister() {
	Super::OnRegister();
}
void UVrmHMDTrackingComponent::OnUnregister() {
	Super::OnUnregister();
}



void UVrmHMDTrackingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_VRM4U_HMD_TRACKER
	bEnableLeft = false;
	bEnableRight = false;
	bEnableHead = false;

	float WoldToMetersScale = 100.f;
	{
		OculusHMD::FOculusHMD* OculusHMD = (OculusHMD::FOculusHMD*)(GEngine->XRSystem.Get());
		if (!OculusHMD)
			return;

		OculusHMD::FSettings* s = OculusHMD->GetSettings();

		ovrpPoseStatef PoseState;
		OculusHMD::FPose Pose;

		WoldToMetersScale = OculusHMD->GetWorldToMetersScale();

		if (OVRP_FAILURE(FOculusHMDModule::GetPluginWrapper().GetNodePoseState3(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, ovrpNode_Head, &PoseState)) ||
			!OculusHMD->ConvertPose_Internal(PoseState.Pose, Pose, s, WoldToMetersScale))
		{
			return;
		}

		HeadPos = Pose.Position;
		HeadRot = Pose.Orientation.Rotator();
		bEnableHead = true;
	}
	{
		ovrpHand eHandState[] = {
			ovrpHand_Left,
			ovrpHand_Right,
		};

		ovrpSkeletonType eSkeletonType[] = {
			ovrpSkeletonType_HandLeft,
			ovrpSkeletonType_HandRight,
		};

		TArray<FTransform>* handTrans[] = {
			&leftHand,
			&rightHand,
		};

		TArray<FTransform>* handReferenceTrans[] = {
			&leftHandReference,
			&rightHandReference,
		};

		TArray<float>* pinch[] = {
			&leftPinch,
			&rightPinch,
		};

		{
			ovrpControllerState4 OvrpControllerState;
			if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetControllerState4((ovrpController)(ovrpController_LHand | ovrpController_RHand | ovrpController_Hands), &OvrpControllerState)))			{
				OvrpControllerState.ConnectedControllerTypes++;
			}
		}

		for (int handCount = 0; handCount < 2; ++handCount) {

			ovrpHandState handState;
			if (OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetHandState(ovrpStep_Render, eHandState[handCount], &handState)) == false) {
				continue;
			}
			if (handCount == 0) {
				bEnableLeft = true;
			} else {
				bEnableRight = true;
			}

			rootScale = handState.HandScale;


			(*(pinch[handCount])).SetNum(ovrpHandFinger_Max);
			for (int i = 0; i < leftPinch.Num(); ++i) {
				(*(pinch[handCount]))[i] = handState.PinchStrength[i];
			}

#if	UE_VERSION_OLDER_THAN(4,27,0)
			ovrpSkeleton skeleton;
			if (FOculusHMDModule::GetPluginWrapper().GetSkeleton(eSkeletonType[handCount], &skeleton) == 0) {
#elif UE_VERSION_OLDER_THAN(5,0,0)
			ovrpSkeleton2 skeleton;
			if (FOculusHMDModule::GetPluginWrapper().GetSkeleton2(eSkeletonType[handCount], &skeleton) == 0) {
#else
			ovrpSkeleton skeleton;
			if (FOculusHMDModule::GetPluginWrapper().GetSkeleton(eSkeletonType[handCount], &skeleton) == 0) {
#endif
				handTrans[handCount]->SetNum(skeleton.NumBones);
				handReferenceTrans[handCount]->SetNum(skeleton.NumBones);
				for (uint32_t i = 0; i < skeleton.NumBones; ++i) {

					int boneID = skeleton.Bones[i].BoneId;

					//auto aa = VRM4ULocal::ToFQuat(handState.BoneRotations[boneID]);
					auto curRot = VRM4ULocal::ToFQuat(handState.BoneRotations[boneID]);
					auto skeletonRot = VRM4ULocal::ToFQuat(skeleton.Bones[i].Pose.Orientation);
					auto skeletonPos = VRM4ULocal::ToFVector(skeleton.Bones[i].Pose.Position) * WoldToMetersScale;

					auto t = FTransform(curRot, skeletonPos);
					auto t_ref = FTransform(skeletonRot, skeletonPos);

					int parentBone = skeleton.Bones[i].ParentBoneIndex;
					if (parentBone >= 0) {
						t = t * (*(handTrans[handCount]))[parentBone];
						t_ref = t_ref * (*(handReferenceTrans[handCount]))[parentBone];
					} else {
						{
							auto q1 = VRM4ULocal::ToFQuat(handState.RootPose.Orientation);
							auto p1 = VRM4ULocal::ToFVector(handState.RootPose.Position) * WoldToMetersScale;

							t = t * FTransform(q1, p1);
						}

						{
							auto q2 = VRM4ULocal::ToFQuat(skeleton.Bones[i].Pose.Orientation);
							auto p2 = VRM4ULocal::ToFVector(skeleton.Bones[i].Pose.Position) * WoldToMetersScale;

							t_ref = t_ref * FTransform(q2, p2);
						}
					}

					(*(handTrans[handCount]))[i] = t;
					(*(handReferenceTrans[handCount]))[i] = t_ref;
				}
			}
		}
	}
	bEnableTracking = true;

	//ovrpBool handTrackingEnabled = false;
	//auto res = FOculusHMDModule::GetPluginWrapper().GetHandTrackingEnabled(&handTrackingEnabled);

	//str2 = FString::Printf(TEXT("%d"), handTrackingEnabled);
	//str2 = handTrackingEnabled;
#endif

}

