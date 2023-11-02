// VRM4U Copyright (c) 2021-2023 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmSkeletalMeshComponent.h"
#include "VrmAnimInstance.h"




UVrmSkeletalMeshComponent::UVrmSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

	AnimClass = UVrmAnimInstance::StaticClass();
}

void UVrmSkeletalMeshComponent::RefreshBoneTransforms(FActorComponentTickFunction* TickFunction) 
{
	Super::RefreshBoneTransforms(TickFunction);
}