// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VrmHMDTrackingComponent.generated.h"

/**
 * 
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class VRM4U_API UVrmHMDTrackingComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		float rootScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		TArray<FTransform> rightHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		TArray<FTransform> leftHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		TArray<FTransform> rightHandReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		TArray<FTransform> leftHandReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		TArray<float> rightPinch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		TArray<float> leftPinch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		FVector HeadPos;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		FRotator HeadRot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Character)
		FString dummyForLog;

	UPROPERTY(BlueprintReadOnly, Category = Character)
		bool bEnableLeft;
	UPROPERTY(BlueprintReadOnly, Category = Character)
		bool bEnableRight;
	UPROPERTY(BlueprintReadOnly, Category = Character)
		bool bEnableHead;


	UFUNCTION(BlueprintPure, Category = "VRM4U", meta = (DynamicOutputParam = "OutVrmAsset"))
	bool IsEnable() const {
		return bEnableTracking;
	};

public:
	void OnRegister() override;
	void OnUnregister() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	bool bEnableTracking = false;;

};
