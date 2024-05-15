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

#if WITH_EDITORONLY_DATA
	if (EnableAutoSearchMetaData) {
		auto v = VRMUtil::GetAssetListObject(VRMGetSkinnedAsset(Context.AnimInstanceProxy->GetSkelMeshComponent()));
		if (v) {
#if	UE_VERSION_OLDER_THAN(5,0,0)
			PoseAsset = v->PoseFace;
#else
			SetPoseAsset(v->PoseFace);
#endif
		}
	}
#endif

	Super::Initialize_AnyThread(Context);
}

void FAnimNode_VrmPoseBlendNode::Evaluate_AnyThread(FPoseContext& Output) {
	Super::Evaluate_AnyThread(Output);

	if (bRemovePoseCurve) {
		auto* sk = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetSkinnedAsset();

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

		for (auto a : removeList) {
			Output.Curve.InvalidateCurveWeight(a);
		}
	}
}
