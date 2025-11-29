// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "AnimNodes/AnimNode_PoseBlendNode.h"
#include "Misc/EngineVersionComparison.h"

#include "AnimNode_VrmPoseBlendNode.generated.h"

class UVrmMetaObject;
class UVrmAssetListObject;

/**
*	Simple controller that replaces or adds to the translation/rotation of a single bone.
*/
USTRUCT(BlueprintInternalUseOnly)
struct VRM4U_API FAnimNode_VrmPoseBlendNode : public FAnimNode_PoseBlendNode
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta = (PinHiddenByDefault))
	bool EnableAutoSearchMetaData = true;

	//UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadWrite, Category = Links)
	//FPoseLink SourcePose;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta=(PinHiddenByDefault))
	//const UVrmMetaObject *VrmMetaObject = nullptr;

#if	UE_VERSION_OLDER_THAN(5,0,0)
	TAssetPtr<UVrmMetaObject> VrmMetaObject_Internal = nullptr;
	TAssetPtr<UVrmAssetListObject> VrmAssetListObject_Internal = nullptr;
	//TAssetPtr<FPoseLink> SourcePose_Internal = nullptr;
#else
	TSoftObjectPtr<UVrmMetaObject> VrmMetaObject_Internal = nullptr;
	TSoftObjectPtr<UVrmAssetListObject> VrmAssetListObject_Internal = nullptr;
	//TSoftObjectPtr<FPoseLink> SourcePose_Internal = nullptr;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta = (PinHiddenByDefault))
	bool bRemovePoseCurve = false;


	bool bCallInitialized = false;

	bool bCallByAnimInstance = false;

	FAnimNode_VrmPoseBlendNode();

	// FAnimNode_SkeletalControlBase interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;

	virtual void Evaluate_AnyThread(FPoseContext& Output) override;


private:
};
