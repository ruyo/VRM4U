// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "AnimGraphNode_VrmModifyHumanoidBone.h"
#include "Misc/EngineVersionComparison.h"
#include "UnrealWidget.h"
#include "AnimNodeEditModes.h"
#include "Kismet2/CompilerResultsLog.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ModifyBone

#define LOCTEXT_NAMESPACE "A3Nodesaaaa"

UAnimGraphNode_VrmModifyHumanoidBone::UAnimGraphNode_VrmModifyHumanoidBone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if	UE_VERSION_OLDER_THAN(5,0,0)
	CurWidgetMode = (int32)FWidget::WM_Rotate;
#else
	CurWidgetMode = UE::Widget::EWidgetMode::WM_Rotate;
#endif
}

void UAnimGraphNode_VrmModifyHumanoidBone::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	// Temporary fix where skeleton is not fully loaded during AnimBP compilation and thus virtual bone name check is invalid UE-39499 (NEED FIX) 
	if (ForSkeleton && !ForSkeleton->HasAnyFlags(RF_NeedPostLoad))
	{
	}

	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

FText UAnimGraphNode_VrmModifyHumanoidBone::GetControllerDescription() const
{
	return LOCTEXT("VrmModifyHumanoidBone", "VrmTransform (Modify) Humanoid Bone");
}

FText UAnimGraphNode_VrmModifyHumanoidBone::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_ModifyBone_Tooltip", "The Transform Bone node alters the transform - i.e. Translation, Rotation, or Scale - of the bone");
}

FText UAnimGraphNode_VrmModifyHumanoidBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

void UAnimGraphNode_VrmModifyHumanoidBone::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_VrmModifyHumanoidBone* ModifyBone = static_cast<FAnimNode_VrmModifyHumanoidBone*>(InPreviewNode);

	// copies Pin values from the internal node to get data which are not compiled yet
	//ModifyBone->Transform = Node.Transform;
	//ModifyBone->VrmAssetList = Node.VrmAssetList;
	//ModifyBone->bEnableRotation = Node.bEnableRotation;
}

//FEditorModeID UAnimGraphNode_VrmModifyHumanoidBone::GetEditorMode() const
//{
//	return AnimNodeEditModes::ModifyBone;
//}

void UAnimGraphNode_VrmModifyHumanoidBone::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{
}

#undef LOCTEXT_NAMESPACE
