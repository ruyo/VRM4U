// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_VrmPoseBlendNode.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Modules/ModuleManager.h"
#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
#include "ToolMenus.h"
#endif

#include "AnimGraphCommands.h"
#include "BlueprintActionFilter.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"

#if	UE_VERSION_OLDER_THAN(4,26,0)
#else
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#define LOCTEXT_NAMESPACE "PoseBlendNode"

/////////////////////////////////////////////////////
// UAnimGraphNode_VrmPoseBlendNode

UAnimGraphNode_VrmPoseBlendNode::UAnimGraphNode_VrmPoseBlendNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAnimGraphNode_VrmPoseBlendNode::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets) const
{
	if (Node.PoseAsset)
	{
		HandleAnimReferenceCollection(Node.PoseAsset, AnimationAssets);
	}
}

void UAnimGraphNode_VrmPoseBlendNode::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& AnimAssetReplacementMap)
{
	HandleAnimReferenceReplacement(Node.PoseAsset, AnimAssetReplacementMap);
}

FText UAnimGraphNode_VrmPoseBlendNode::GetTooltipText() const
{
	// FText::Format() is slow, so we utilize the cached list title
	return GetNodeTitle(ENodeTitleType::ListView);
}

FText UAnimGraphNode_VrmPoseBlendNode::GetNodeTitleForPoseAsset(ENodeTitleType::Type TitleType, UPoseAsset* InPoseAsset) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("PoseAssetName"), FText::FromString(InPoseAsset->GetName()));

	return FText::Format(LOCTEXT("PoseByName_Title", "VRM4U {PoseAssetName}"), Args);
}

FText UAnimGraphNode_VrmPoseBlendNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (Node.EnableAutoSearchMetaData)
	{
		// we may have a valid variable connected or default pin value
		UEdGraphPin* PosePin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_VrmPoseBlendNode, PoseAsset));
		if (PosePin && PosePin->LinkedTo.Num() > 0)
		{
			return LOCTEXT("PoseByName_TitleVariable", "VRM4U Pose");
		}
		else if (PosePin && PosePin->DefaultObject != nullptr)
		{
			return GetNodeTitleForPoseAsset(TitleType, CastChecked<UPoseAsset>(PosePin->DefaultObject));
		}
		else
		{
			return LOCTEXT("PoseByName_TitleNONE", "VRM4U Pose (Auto Search)");
		}
	}
	else
	{
		return LOCTEXT("PoseByName_TitleVariable", "VRM4U Pose");
		//return GetNodeTitleForPoseAsset(TitleType, Node.PoseAsset);
	}
}

void UAnimGraphNode_VrmPoseBlendNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
	GetMenuActionsHelper(
		InActionRegistrar,
		GetClass(),
		{ UPoseAsset::StaticClass() },
		{ },
		[](const FAssetData& InAssetData, UClass* InClass)
		{
			if (InAssetData.IsValid())
			{
				return GetTitleGivenAssetInfo(FText::FromName(InAssetData.AssetName));
			}
			else
			{
				return LOCTEXT("PlayerDesc", "VRM4U Evaluate Pose");
			}
		},
		[](const FAssetData& InAssetData, UClass* InClass)
		{
			if (InAssetData.IsValid())
			{
				return GetTitleGivenAssetInfo(FText::FromName(InAssetData.AssetName));
			}
			else
			{
				return LOCTEXT("PlayerDescTooltip", "VRM4U Evaluate Pose Asset");
			}
		},
		[](UEdGraphNode* InNewNode, bool bInIsTemplateNode, const FAssetData InAssetData)
		{
			UAnimGraphNode_AssetPlayerBase::SetupNewNode(InNewNode, bInIsTemplateNode, InAssetData);
		});
#endif
}

FText UAnimGraphNode_VrmPoseBlendNode::GetTitleGivenAssetInfo(const FText& AssetName)
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("AssetName"), AssetName);

	return FText::Format(LOCTEXT("PoseAssetNodeTitle", "Evaluate Pose {AssetName}"), Args);
}

FText UAnimGraphNode_VrmPoseBlendNode::GetMenuCategory() const
{
	return LOCTEXT("PoseAssetCategory_Label", "Animation|Poses");
}

bool UAnimGraphNode_VrmPoseBlendNode::DoesSupportTimeForTransitionGetter() const
{
	return false;
}

#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
void UAnimGraphNode_VrmPoseBlendNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
	if (!Context->bIsDebugging)
	{
		// add an option to convert to single frame
		{
			FToolMenuSection& Section = Menu->AddSection("AnimGraphNodePoseBlender", LOCTEXT("PoseBlenderHeading", "Pose Blender"));
			Section.AddMenuEntry(FAnimGraphCommands::Get().ConvertToPoseByName);
		}
	}
#endif
}
#endif

EAnimAssetHandlerType UAnimGraphNode_VrmPoseBlendNode::SupportsAssetClass(const UClass* AssetClass) const
{
	if (AssetClass->IsChildOf(UPoseAsset::StaticClass()))
	{
		return EAnimAssetHandlerType::PrimaryHandler;
	}
	else
	{
		return EAnimAssetHandlerType::NotSupported;
	}
}


////
void UAnimGraphNode_VrmPoseBlendNode::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog) {
}
void UAnimGraphNode_VrmPoseBlendNode::PreloadRequiredAssets() {
}
UAnimationAsset* UAnimGraphNode_VrmPoseBlendNode::GetAnimationAsset() const {
	UPoseAsset* PoseAsset = GetPoseHandlerNode()->PoseAsset;
	UEdGraphPin* PoseAssetPin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_PoseHandler, PoseAsset));
	if (PoseAssetPin != nullptr && PoseAsset == nullptr)
	{
		PoseAsset = Cast<UPoseAsset>(PoseAssetPin->DefaultObject);
	}

	return PoseAsset;
}
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
TSubclassOf<UAnimationAsset> UAnimGraphNode_VrmPoseBlendNode::GetAnimationAssetClass() const {
	return UPoseAsset::StaticClass();
}
void UAnimGraphNode_VrmPoseBlendNode::OnOverrideAssets(IAnimBlueprintNodeOverrideAssetsContext& InContext) const {
	/*
		if (InContext.GetAssets().Num() > 0)
		{
			if (UPoseAsset* PoseAsset = Cast<UPoseAsset>(InContext.GetAssets()[0]))
			{
				FAnimNode_PoseHandler& AnimNode = InContext.GetAnimNode<FAnimNode_PoseHandler>();
				AnimNode.SetPoseAsset(PoseAsset);
			}
		}
		*/
}
#endif

void UAnimGraphNode_VrmPoseBlendNode::SetAnimationAsset(UAnimationAsset* Asset) {
	if (UPoseAsset* PoseAsset = Cast<UPoseAsset>(Asset))
	{
		GetPoseHandlerNode()->PoseAsset = PoseAsset;
	}
}

////
#undef LOCTEXT_NAMESPACE
