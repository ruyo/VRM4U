// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "VrmSkeletalMeshComponent.generated.h"

/**
 * 
 */
//UCLASS(meta=(BlueprintSpawnableComponent))
UCLASS()
class VRM4ULOADER_API UVrmSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_UCLASS_BODY()
	
	
public:
	virtual void RefreshBoneTransforms( FActorComponentTickFunction* TickFunction = NULL ) override;


};
