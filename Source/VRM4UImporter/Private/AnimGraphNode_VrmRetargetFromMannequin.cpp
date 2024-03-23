// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "AnimGraphNode_VrmRetargetFromMannequin.h"
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

UAnimGraphNode_VrmRetargetFromMannequin::UAnimGraphNode_VrmRetargetFromMannequin(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if	UE_VERSION_OLDER_THAN(5,0,0)
	CurWidgetMode = (int32)FWidget::WM_Rotate;
#else
	CurWidgetMode = UE::Widget::EWidgetMode::WM_Rotate;
#endif
}

void UAnimGraphNode_VrmRetargetFromMannequin::ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex) {

	if (Node.VrmMetaObject == nullptr) {
		//MessageLog.Warning(*LOCTEXT("VrmNoMetaObject", "@@ - You must set VrmMetaObject").ToString(), this);
	} else {
		if (Node.VrmMetaObject->SkeletalMesh) {
			if (VRMGetSkeleton(Node.VrmMetaObject->SkeletalMesh) != CompiledClass->GetTargetSkeleton()) {
				MessageLog.Warning(*LOCTEXT("VrmDifferentSkeleton", "@@ - You must set VrmMetaObject has same skeleton").ToString(), this);
			}
		}
		if (CompiledClass->GetTargetSkeleton()->GetReferenceSkeleton().GetRawBoneNum() <= 0) {
			MessageLog.Warning(*LOCTEXT("VrmNoBone", "@@ - Skeleton bad data").ToString(), this);
		}
	}

	Super::ValidateAnimNodePostCompile(MessageLog, CompiledClass, CompiledNodeIndex);
}

void UAnimGraphNode_VrmRetargetFromMannequin::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

FText UAnimGraphNode_VrmRetargetFromMannequin::GetControllerDescription() const
{
	return LOCTEXT("VrmRetargetFromMannequin", "VrmRetargetFromMannequin");
}

FText UAnimGraphNode_VrmRetargetFromMannequin::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_VrmRetargetFromMannequin_Tooltip", "VrmCopyHandBone");
}

FText UAnimGraphNode_VrmRetargetFromMannequin::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

void UAnimGraphNode_VrmRetargetFromMannequin::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_VrmRetargetFromMannequin* node = static_cast<FAnimNode_VrmRetargetFromMannequin*>(InPreviewNode);

	// no copy
}

FEditorModeID UAnimGraphNode_VrmRetargetFromMannequin::GetEditorMode() const
{
	return Super::GetEditorMode();
}

void UAnimGraphNode_VrmRetargetFromMannequin::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent * PreviewSkelMeshComp) const
{
}


void UAnimGraphNode_VrmRetargetFromMannequin::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{
}



#undef LOCTEXT_NAMESPACE
