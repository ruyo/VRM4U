// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_ModifyBone.h"
#include "Misc/EngineVersionComparison.h"

#include "AnimNode_VrmRetargetFromMannequin.generated.h"

class USkeletalMeshComponent;
class UVrmMetaObject;

/**
*	Simple controller that replaces or adds to the translation/rotation of a single bone.
*/
USTRUCT(BlueprintInternalUseOnly)
struct VRM4U_API FAnimNode_VrmRetargetFromMannequin : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/* If SourceMeshComponent is not valid, and if this is true, it will look for attached parent as a source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= Skeleton, meta = (NeverAsPin))
	bool bUseAttachedParent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta = (PinShownByDefault))
	TWeakObjectPtr<USkeletalMeshComponent> srcMannequinMesh= nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta=(PinHiddenByDefault))
	UVrmMetaObject *VrmMetaObject = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta = (PinHiddenByDefault))
	bool bIgnoreCenterLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta = (PinHiddenByDefault))
	FVector CenterLocationScaleByHeightScale = { 1.0f, 1.0f, 1.0f }; // FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta = (PinHiddenByDefault))
	FVector CenterLocationOffset = { 0.0f, 0.0f, 0.0f }; // FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Skeleton, meta = (PinHiddenByDefault))
	UVrmMetaObject *MannequinVrmMetaObject = nullptr;

	FAnimNode_VrmRetargetFromMannequin();

	UPROPERTY()
	USkeletalMesh *srcSkeletalMesh = nullptr;
	UPROPERTY()
	USkeletalMesh *dstSkeletalMesh = nullptr;

	TArray<FTransform> srcRefSkeletonCompTransform;
	TArray<FTransform> dstRefSkeletonCompTransform;
	TArray<FTransform> dstCurrentSkeletonCompTransform;

	void UpdateCache(FComponentSpacePoseContext& Output);

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
//	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

	virtual bool NeedsDynamicReset() const override { return true; }
#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
	virtual void ResetDynamics(ETeleportType InTeleportType) override;
#endif

	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context)override;
	// End of FAnimNode_SkeletalControlBase interface

	virtual void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp, bool bPreviewForeground = false) const;

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
};
