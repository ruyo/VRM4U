// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "Misc/EngineVersionComparison.h"

#include "VrmAnimInstanceCopy.generated.h"

class UVrmAssetListObject;
struct FAnimNode_VrmSpringBone;
struct FAnimNode_VrmConstraint;

USTRUCT()
struct VRM4U_API FVrmAnimInstanceCopyProxy : public FAnimInstanceProxy {

public:
	GENERATED_BODY()

	float CurrentDeltaTime = 0.f;
	bool bIgnoreVRMSwingBone = false;
	bool bIgnoreVRMConstraint = false;
	bool bIgnoreWindDirectionalSource = false;
	int CalcCount = 0;
	TSharedPtr<FAnimNode_VrmSpringBone> Node_SpringBone;
	TSharedPtr<FAnimNode_VrmConstraint> Node_Constraint;

	bool bUseAnimStop = false;
	bool bAnimStop = false;
	bool bIgnoreCenterLocation = false;
	FVector CenterLocationScaleByHeightScale = { 1.f, 1.f, 1.f };
	FVector CenterLocationOffset = {0.f, 0.f, 0.f};
	bool bCopyStop = false;
	FCompactHeapPose CachedPose;

	FVrmAnimInstanceCopyProxy();

	FVrmAnimInstanceCopyProxy(UAnimInstance* InAnimInstance);

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
#if	UE_VERSION_OLDER_THAN(4,24,0)
	virtual void UpdateAnimationNode(float DeltaSeconds) override;
#else
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext);
#endif
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class VRM4U_API UVrmAnimInstanceCopy : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= Rendering, meta = (NeverAsPin))
	bool bUseAttachedParent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	USkeletalMeshComponent *SrcSkeletalMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	USkinnedMeshComponent* SrcAsSkinnedMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	UVrmAssetListObject *SrcVrmAssetList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	UVrmMetaObject* SrcVrmMetaOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	UVrmAssetListObject *DstVrmAssetList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	UVrmMetaObject *DstVrmMetaForCustomSpring;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	TArray<FName> InitialMorphName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	TArray<float> InitialMorphValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	bool bUseAnimStop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	bool bAnimStop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	bool bCopyStop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	bool bIgnoreCenterLocation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	FVector CenterLocationScaleByHeightScale = {1.f, 1.f, 1.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	FVector CenterLocationOffset = { 0.f, 0.f, 0.f };

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	FVrmAnimInstanceCopyProxy *myProxy = nullptr;
	bool bIgnoreVRMSwingBone = false;
	bool bIgnoreWindDirectionalSource = false;
public:
	virtual void NativeInitializeAnimation()override;
	// Native update override point. It is usually a good idea to simply gather data in this step and 
	// for the bulk of the work to be done in NativeUpdateAnimation.
	virtual void NativeUpdateAnimation(float DeltaSeconds)override;
	// Native Post Evaluate override point
	virtual void NativePostEvaluateAnimation()override;
	// Native Uninitialize override point
	virtual void NativeUninitializeAnimation()override;

	// Executed when begin play is called on the owning component
	virtual void NativeBeginPlay()override;

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetSkeletalMeshCopyData(UVrmAssetListObject *dstAssetList, USkeletalMeshComponent *srcSkeletalMesh, USkinnedMeshComponent* srcSkinnedMesh, UVrmAssetListObject *srcAssetList, UVrmMetaObject* srcVrmMetaOption);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetSkeletalMeshCopyDataForCustomSpring(UVrmMetaObject *dstMetaForCustomSpring);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetVrmSpringBoneParam(float gravityScale = 1.f, FVector gravityAdd = FVector::ZeroVector, float stiffnessScale = 1.f, float stiffnessAdd = 0.f, float randomWindRange = 0.2f);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetVrmSpringBoneBool(bool bIgnoreVrmSpringBone = false, bool bIgnorePhysicsCollision = false, bool bIgnoreVRMCollision = false, bool bIgnoreWind = false);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetVrmSpringBoneIgnoreWingBone(const TArray<FName> &boneNameList);

	FVrmAnimInstanceCopyProxy *GetProxy() { return myProxy; }
};
