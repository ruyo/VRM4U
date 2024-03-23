// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_ModifyBone.h"

#include "AnimNode_VrmModifyBoneDynamic.generated.h"

class USkeletalMeshComponent;

/**
*	Simple controller that replaces or adds to the translation/rotation of a single bone.
*/
USTRUCT(BlueprintInternalUseOnly)
struct VRM4U_API FAnimNode_VrmModifyBoneDynamic : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

		/** Name of bone to control. This is the main bone chain to modify from. **/
	//UPROPERTY(EditAnywhere, Category=SkeletalControl) 
		//FBoneReference BoneToModify;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkeletalControl, meta=(PinShownByDefault))
	FName BoneNameToModify;

	/** New translation of bone to apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Translation, meta=(PinShownByDefault))
		FVector Translation;

	/** New rotation of bone to apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rotation, meta=(PinShownByDefault))
		FRotator Rotation;

	/** New Scale of bone to apply. This is only worldspace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Scale, meta=(PinShownByDefault))
		FVector Scale;

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, Category=Translation)
		TEnumAsByte<EBoneModificationMode> TranslationMode;

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, Category=Rotation)
		TEnumAsByte<EBoneModificationMode> RotationMode;

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, Category=Scale)
		TEnumAsByte<EBoneModificationMode> ScaleMode;

	/** Reference frame to apply Translation in. */
	UPROPERTY(EditAnywhere, Category=Translation)
		TEnumAsByte<enum EBoneControlSpace> TranslationSpace;

	/** Reference frame to apply Rotation in. */
	UPROPERTY(EditAnywhere, Category=Rotation)
		TEnumAsByte<enum EBoneControlSpace> RotationSpace;

	/** Reference frame to apply Scale in. */
	UPROPERTY(EditAnywhere, Category=Scale)
		TEnumAsByte<enum EBoneControlSpace> ScaleSpace;

	FAnimNode_VrmModifyBoneDynamic();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
};
