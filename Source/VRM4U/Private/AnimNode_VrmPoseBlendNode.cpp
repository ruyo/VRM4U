// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "AnimNode_VrmPoseBlendNode.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

#include "VrmMetaObject.h"
#include "VrmAssetListObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone


FAnimNode_VrmPoseBlendNode::FAnimNode_VrmPoseBlendNode() {
}

void FAnimNode_VrmPoseBlendNode::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
	bCallInitialized = true;

	if (EnableAutoSearchMetaData) {
		auto v = VRMUtil::GetAssetListObject(VRMGetSkinnedAsset(Context.AnimInstanceProxy->GetSkelMeshComponent()));
		if (v) {
			PoseAsset = v->PoseFace;
			//SetPoseAsset(v->PoseFace);
		}
	}

	Super::Initialize_AnyThread(Context);
}

void FAnimNode_VrmPoseBlendNode::Evaluate_AnyThread(FPoseContext& Output) {
	Super::Evaluate_AnyThread(Output);

	if (bRemovePoseCurve) {
		auto* sk = VRMGetSkinnedAsset(Output.AnimInstanceProxy->GetSkelMeshComponent());



#if	UE_VERSION_OLDER_THAN(5,0,0)
#elif	UE_VERSION_OLDER_THAN(5,3,0)
		auto& MorphList = sk->GetMorphTargets();
		TArray<SmartName::UID_Type> removeList;

		auto* k = sk->GetSkeleton();

		auto CurveList = k->GetDefaultCurveUIDList();

		//k->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, *p.expressionName, sm);

		auto* CurveMappingPtr = k->GetSmartNameContainer(USkeleton::AnimCurveMappingName);

		// 
		TArray<FName> nameList;
		CurveMappingPtr->FillUIDToNameArray(nameList);

		TArray<SmartName::UID_Type> uidList;
		CurveMappingPtr->FillUidArray(uidList);

		for (int i = 0; i < nameList.Num(); ++i) {
			auto* ind = MorphList.FindByPredicate([&nameList, &i](const TObjectPtr<UMorphTarget > morph) {
				if (morph->GetName().Compare(nameList[i].ToString())) return false;
				return true;
				});

			if (ind == nullptr) {
				removeList.Add(uidList[i]);
			}
		}
#else
		auto& MorphList = sk->GetMorphTargets();
		TArray<FName> removeList;

		Output.Curve.ForEachElement([&removeList, &MorphList](const UE::Anim::FCurveElement& InCurveElement)
			{
				FString CurveElementName = InCurveElement.Name.ToString();
				auto* ind = MorphList.FindByPredicate([&CurveElementName](const TObjectPtr<UMorphTarget > morph) {
					if (CurveElementName.Compare(morph->GetName(), ESearchCase::IgnoreCase) == 0) return true;
					return false;
					});

				if (ind == nullptr) {
					removeList.Add(InCurveElement.Name);
				}
			});
#endif

#if	UE_VERSION_OLDER_THAN(5,0,0)

#elif	UE_VERSION_OLDER_THAN(5,1,0)
		for (auto a : removeList) {
			Output.Curve.Set(a, 0);
		}
#else
		for (auto a : removeList) {
			Output.Curve.InvalidateCurveWeight(a);
		}
#endif
	}
}
