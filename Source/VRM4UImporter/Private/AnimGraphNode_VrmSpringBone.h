// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
//#include "BoneControllers/AnimNode_ModifyBone.h"
#include "AnimNode_VrmSpringBone.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "AnimGraphNode_SkeletalControlBase.h"

#include "UnrealWidget.h"
#include "AnimNodeEditMode.h"

#include "AnimGraphNode_VrmSpringBone.generated.h" 

class FCompilerResultsLog;
class FPrimitiveDrawInterface;
class USkeletalMeshComponent;

class FVrmSpringBoneEditMode {
public:
	static const FEditorModeID VrmSpringBone;
};

/*
class FVrmSpringBoneEditMode : public FAnimNodeEditMode
{
public:
	//IAnimNodeEditMode interface
	virtual void EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode) override;
	virtual void ExitMode() override;
	virtual FVector GetWidgetLocation() const override;
	virtual FWidget::EWidgetMode GetWidgetMode() const override;
	virtual void DoTranslation(FVector& InTranslation) override;

	static const FEditorModeID VrmSpringBone;

private:
	struct FAnimNode_VrmSpringBone* RuntimeNode;
	class UAnimGraphNode_VrmSpringBone* GraphNode;
};
*/



UCLASS(meta=(Keywords = "VRM4U"))
class UAnimGraphNode_VrmSpringBone : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_VrmSpringBone Node;

public:
	UPROPERTY(EditAnywhere, Category = Preview)
	bool bPreviewLive = true;

	UPROPERTY(EditAnywhere, Category = Preview)
	bool bPreviewForeground = false;

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	// End of UEdGraphNode interface

protected:
	// UAnimGraphNode_Base interface
	virtual void ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog) override;
	virtual void ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex) override;
	//	virtual FEditorModeID GetEditorMode() const override;
	virtual void CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode) override;
	virtual void CopyPinDefaultsToNodeData(UEdGraphPin* InPin) override;

	virtual FEditorModeID GetEditorMode() const override;
	virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent * PreviewSkelMeshComp) const override;
	// End of UAnimGraphNode_Base interface

	// UAnimGraphNode_SkeletalControlBase interface
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
	// End of UAnimGraphNode_SkeletalControlBase interface

private:
	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTitleTextTable CachedNodeTitles;

	// storing current widget mode 
	int32 CurWidgetMode;
};

