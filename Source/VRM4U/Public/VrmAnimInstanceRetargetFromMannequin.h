// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once


#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "Misc/EngineVersionComparison.h"

#if	UE_VERSION_OLDER_THAN(5,2,0)
static_assert(0, "Available for UE5.2+. delete h/cpp files");
#endif

#include "Retargeter/IKRetargeter.h"
#include "VrmAnimInstanceRetargetFromMannequin.generated.h"

class UVrmAssetListObject;
struct FAnimNode_VrmSpringBone;
struct FAnimNode_VrmConstraint;
struct FAnimNode_RetargetPoseFromMesh;

USTRUCT()
struct VRM4U_API FVrmAnimInstanceRetargetFromMannequinProxy : public FAnimInstanceProxy {

public:
	GENERATED_BODY()

	float CurrentDeltaTime = 0.f;
	bool bUseRetargeter = false;
	bool bIgnoreVRMSwingBone = false;
	bool bIgnoreVRMConstraint = false;
	bool bIgnoreWindDirectionalSource = false;
	int CalcCount = 0;
	TSharedPtr<FAnimNode_VrmSpringBone> Node_SpringBone;
	TSharedPtr<FAnimNode_VrmConstraint> Node_Constraint;
#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
	//TWeakObjectPtr<USkeletalMeshComponent> RetargetSourceMeshComponent = nullptr;
	TSoftObjectPtr<UIKRetargeter> Retargeter;
	TSharedPtr<FAnimNode_RetargetPoseFromMesh> Node_Retarget;

	void CustomInitialize();
#endif

	FCompactHeapPose CachedPose;

	FVrmAnimInstanceRetargetFromMannequinProxy();

	FVrmAnimInstanceRetargetFromMannequinProxy(UAnimInstance* InAnimInstance);

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext);

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds);
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class VRM4U_API UVrmAnimInstanceRetargetFromMannequin : public UAnimInstance
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

	virtual bool CanRunParallelWork() const { return false; }

	FVrmAnimInstanceRetargetFromMannequinProxy *myProxy = nullptr;
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
	virtual void PreUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetRetargetData(bool bUseRetargeter, UIKRetargeter *IKRetargeter);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetVrmAssetList(UVrmAssetListObject *dstAssetList);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetSkeletalMeshCopyDataForCustomSpring(UVrmMetaObject *dstMetaForCustomSpring);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetVrmSpringBoneParam(float gravityScale = 1.f, FVector gravityAdd = FVector::ZeroVector, float stiffnessScale = 1.f, float stiffnessAdd = 0.f, float randomWindRange = 0.2f);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetVrmSpringBoneBool(bool bIgnoreVrmSpringBone = false, bool bIgnorePhysicsCollision = false, bool bIgnoreVRMCollision = false, bool bIgnoreWind = false);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetVrmSpringBoneIgnoreWingBone(const TArray<FName> &boneNameList);

	FVrmAnimInstanceRetargetFromMannequinProxy*GetProxy() { return myProxy; }
};
