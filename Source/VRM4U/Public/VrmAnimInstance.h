// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Misc/EngineVersionComparison.h"

#include "VrmAnimInstance.generated.h"

class UVrmMetaObject;
class USkeleton;

UENUM(BlueprintType)
enum class EVRMBlendShapeGroup : uint8
{
	BSG_Neutoral		UMETA(DisplayName="00_Neutoral"),
	BSG_A			UMETA(DisplayName="01_A"),
	BSG_I			UMETA(DisplayName="02_I"),
	BSG_U			UMETA(DisplayName="03_U"),
	BSG_E			UMETA(DisplayName="04_E"),
	BSG_O			UMETA(DisplayName="05_O"),
	BSG_Blink		UMETA(DisplayName="06_Blink"),
	BSG_Joy			UMETA(DisplayName="07_Joy"),
	BSG_Angry		UMETA(DisplayName="08_Angry"),
	BSG_Sorrow		UMETA(DisplayName="09_Sorrow"),
	BSG_Fun			UMETA(DisplayName="10_Fun"),
	BSG_LookUp		UMETA(DisplayName="11_LookUp"),
	BSG_LookDown	UMETA(DisplayName="12_LookDown"),
	BSG_LookLeft	UMETA(DisplayName="13_LookLeft"),
	BSG_LookRight	UMETA(DisplayName="14_LookRight"),
	BSG_Blink_L		UMETA(DisplayName="15_Blink_L"),
	BSG_Blink_R		UMETA(DisplayName="16_Blink_R"),
	Num				UMETA(Hidden)
};


USTRUCT()
struct VRM4U_API FVrmAnimInstanceProxy : public FAnimInstanceProxy {

public:
	GENERATED_BODY()

		FVrmAnimInstanceProxy()
	{
	}

	FVrmAnimInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

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
class VRM4U_API UVrmAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	UVrmMetaObject *MetaObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	USkeleton *BaseSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering)
	USkeletalMeshComponent *BaseSkeletalMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
	FTransform TransHandLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
	FTransform TransHandRight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
	FTransform TransHead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
	USceneComponent *ComponentHandLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
	USceneComponent *ComponentHandRight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
	USceneComponent *ComponentHead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
		USceneComponent *ComponentHandJointTargetLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tracking)
		USceneComponent *ComponentHandJointTargetRight;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
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

	//virtual USkeleton* GetTargetSkeleton() const override;

	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetMorphTargetVRM(EVRMBlendShapeGroup type, float Value);

	
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void SetVrmData(USkeletalMeshComponent *baseSkeletalMesh, UVrmMetaObject *meta);

};
