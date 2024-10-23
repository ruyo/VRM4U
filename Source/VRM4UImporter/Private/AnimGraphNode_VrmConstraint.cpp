// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "AnimGraphNode_VrmConstraint.h"
#include "Misc/EngineVersionComparison.h"
#include "UnrealWidget.h"
#include "AnimNodeEditModes.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstance.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Components/SkeletalMeshComponent.h"
#include "VrmMetaObject.h"
#include "VrmUtil.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ModifyBone

#define LOCTEXT_NAMESPACE "A3Nodesaaaa"
////////////////////

UAnimGraphNode_VrmConstraint::UAnimGraphNode_VrmConstraint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if	UE_VERSION_OLDER_THAN(5,0,0)
	CurWidgetMode = (int32)FWidget::WM_Rotate;
#else
	CurWidgetMode = UE::Widget::EWidgetMode::WM_Rotate;
#endif
}

void UAnimGraphNode_VrmConstraint::ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex) {

	if (Node.VrmMetaObject_Internal == nullptr) {
		//MessageLog.Warning(*LOCTEXT("VrmNoMetaObject", "@@ - You must set VrmMetaObject").ToString(), this);
	} else {
		if (Node.VrmMetaObject_Internal->SkeletalMesh) {
			if (VRMGetSkeleton(Node.VrmMetaObject_Internal->SkeletalMesh) != CompiledClass->GetTargetSkeleton()) {
				MessageLog.Warning(*LOCTEXT("VrmDifferentSkeleton", "@@ - You must set VrmMetaObject has same skeleton").ToString(), this);
			}
		}
		if (CompiledClass->GetTargetSkeleton()->GetReferenceSkeleton().GetRawBoneNum() <= 0) {
			MessageLog.Warning(*LOCTEXT("VrmNoBone", "@@ - Skeleton bad data").ToString(), this);
		}
	}

	Super::ValidateAnimNodePostCompile(MessageLog, CompiledClass, CompiledNodeIndex);
}

void UAnimGraphNode_VrmConstraint::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

FText UAnimGraphNode_VrmConstraint::GetControllerDescription() const
{
	if (Node.EnableAutoSearchMetaData) {
		return LOCTEXT("VrmConstraint", "VrmConstraint(auto)");
	} else {
		return LOCTEXT("VrmConstraint", "VrmConstraint");
	}
}

FText UAnimGraphNode_VrmConstraint::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_VrmConstraint_Tooltip", "VrmCopyHandBone");
}

FText UAnimGraphNode_VrmConstraint::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

void UAnimGraphNode_VrmConstraint::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_VrmConstraint* node = static_cast<FAnimNode_VrmConstraint*>(InPreviewNode);

	// no copy
}

FEditorModeID UAnimGraphNode_VrmConstraint::GetEditorMode() const
{
	return Super::GetEditorMode();
}

void UAnimGraphNode_VrmConstraint::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent * PreviewSkelMeshComp) const
{
	if (PreviewSkelMeshComp)
	{
		/*
		if (FAnimNode_VrmSpringBone* ActiveNode = GetActiveInstanceNode<FAnimNode_VrmSpringBone>(PreviewSkelMeshComp->GetAnimInstance()))
		{
			if (bPreviewLive) {
				ActiveNode->ConditionalDebugDraw(PDI, PreviewSkelMeshComp, bPreviewForeground);
			}
		}
		*/
		if (bPreviewLive) {
			Node.ConditionalDebugDraw(PDI, PreviewSkelMeshComp, bPreviewForeground);
		}
	}
}


void UAnimGraphNode_VrmConstraint::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{
}



#undef LOCTEXT_NAMESPACE
