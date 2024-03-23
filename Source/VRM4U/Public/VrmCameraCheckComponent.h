// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "Misc/CoreDelegates.h"

#if WITH_EDITOR
#include "Editor/UnrealEdTypes.h"
#endif


#include "VrmCameraCheckComponent.generated.h"

/**
 * 
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class VRM4U_API UVrmCameraCheckComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
	
public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVrmCameraCheckDelegate);

	UPROPERTY(BlueprintAssignable)
	FVrmCameraCheckDelegate OnCameraMove;

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (DynamicOutputParam = "OutVrmAsset"))
	void SetCameraCheck(bool bCheckOn);

public:
	void OnRegister() override;
	void OnUnregister() override;

private:
#if WITH_EDITOR
	void OnCameraTransformChanged(const FVector&, const FRotator&, ELevelViewportType, int32);
#endif

	FDelegateHandle handle;
};
