// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "AnimGraphNode_VrmModifyBoneListRetarget.h"
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


UAnimGraphNode_VrmModifyBoneListRetarget::UAnimGraphNode_VrmModifyBoneListRetarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if	UE_VERSION_OLDER_THAN(5,0,0)
	CurWidgetMode = (int32)FWidget::WM_Rotate;
#else
	CurWidgetMode = UE::Widget::EWidgetMode::WM_Rotate;
#endif
}

void UAnimGraphNode_VrmModifyBoneListRetarget::ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex) {

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

void UAnimGraphNode_VrmModifyBoneListRetarget::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	// Temporary fix where skeleton is not fully loaded during AnimBP compilation and thus virtual bone name check is invalid UE-39499 (NEED FIX) 
	if (ForSkeleton && !ForSkeleton->HasAnyFlags(RF_NeedPostLoad))
	{
		if (Node.VrmMetaObject == nullptr) {
			//MessageLog.Warning(*LOCTEXT("VrmNoMetaObject", "@@ - You must set VrmMetaObject").ToString(), this);
		} else {
			if (Node.VrmMetaObject->SkeletalMesh){
				if (VRMGetSkeleton(Node.VrmMetaObject->SkeletalMesh) != ForSkeleton) {
			//		MessageLog.Warning(*LOCTEXT("VrmDifferentSkeleton", "@@ - You must set VrmMetaObject has same skeleton").ToString(), this);
				}
			}
			if (ForSkeleton->GetReferenceSkeleton().GetRawBoneNum() <= 0) {
			//	MessageLog.Warning(*LOCTEXT("VrmNoBone", "@@ - Skeleton bad data").ToString(), this);
			}
		}


		/*
		//if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.BoneToModify.BoneName) == INDEX_NONE)
		if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.BoneNameToModify) == INDEX_NONE)
		{
			//if (Node.BoneToModify.BoneName == NAME_None)
			if (Node.BoneNameToModify == NAME_None)
			{
				MessageLog.Warning(*LOCTEXT("NoBoneSelectedToModify", "@@ - You must pick a bone to modify").ToString(), this);
			}
			else
			{
				FFormatNamedArguments Args;
				//Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneToModify.BoneName));
				Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneNameToModify));

				FText Msg = FText::Format(LOCTEXT("NoBoneFoundToModify", "@@ - Bone {BoneName} not found in Skeleton"), Args);

				MessageLog.Warning(*Msg.ToString(), this);
			}
		}
		*/
	}

	//if ((Node.TranslationMode == BMM_Ignore) && (Node.RotationMode == BMM_Ignore) && (Node.ScaleMode == BMM_Ignore))
	{
	//	MessageLog.Warning(*LOCTEXT("NothingToModify", "@@ - No components to modify selected.  Either Rotation, Translation, or Scale should be set to something other than Ignore").ToString(), this);
	}

	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

FText UAnimGraphNode_VrmModifyBoneListRetarget::GetControllerDescription() const
{
	return LOCTEXT("VrmModifyBoneListRetarget", "VrmModifyBoneListRetarget");
}

FText UAnimGraphNode_VrmModifyBoneListRetarget::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_VrmModifyBoneListRetarget_Tooltip", "VrmModifyBoneListRetarget");
}

FText UAnimGraphNode_VrmModifyBoneListRetarget::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	//if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.BoneToModify.BoneName == NAME_None))
	//if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.BoneNameToModify == NAME_None))
	//{
		return GetControllerDescription();
	//}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
		/*
	else //if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		//Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneToModify.BoneName));
		Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneNameToModify));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_ModifyBone_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_ModifyBone_Title", "{ControllerDescription}\nBone: {BoneName}"), Args), this);
		}
	}
	return CachedNodeTitles[TitleType];
	*/
}

//void UAnimGraphNode_VrmModifyBoneListRetarget::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
//{
//}

FEditorModeID UAnimGraphNode_VrmModifyBoneListRetarget::GetEditorMode() const
{
	return Super::GetEditorMode();
}

void UAnimGraphNode_VrmModifyBoneListRetarget::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent * PreviewSkelMeshComp) const
{
	if (PreviewSkelMeshComp)
	{
		if (FAnimNode_VrmModifyBoneListRetarget* ActiveNode = GetActiveInstanceNode<FAnimNode_VrmModifyBoneListRetarget>(PreviewSkelMeshComp->GetAnimInstance()))
		{
			if (bPreviewLive) {
				//ActiveNode->ConditionalDebugDraw(PDI, PreviewSkelMeshComp, bPreviewForeground);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
