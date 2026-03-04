// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VrmBoneCheckComponent.generated.h"


UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class VRM4U_API UVrmBoneCheckComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()
	
public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVrmBoneCheckDelegate);

	UPROPERTY(BlueprintAssignable)
	FVrmBoneCheckDelegate OnBoneTransform;

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	void OnTargetTransformUpdate();


	//virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

};
