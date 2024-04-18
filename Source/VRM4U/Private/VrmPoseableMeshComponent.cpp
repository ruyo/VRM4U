// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmPoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "VrmAnimInstance.h"




UVrmPoseableMeshComponent::UVrmPoseableMeshComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// for morph curve copy
	bTickInEditor = true;
}

void UVrmPoseableMeshComponent::OnRegister() {
	Super::OnRegister();
	if (bUseDefaultMaterial) {
		this->OverrideMaterials.Empty();
	}
	Init();
}


void UVrmPoseableMeshComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if	UE_VERSION_OLDER_THAN(4,20,0)
#else

#if	UE_VERSION_OLDER_THAN(5,1,0)
	USkinnedMeshComponent* MPCPtr = MasterPoseComponent.Get();
#else
	USkinnedMeshComponent* MPCPtr = LeaderPoseComponent.Get();
#endif

	if (MPCPtr) {
		MorphTargetWeights = MPCPtr->MorphTargetWeights;
		ActiveMorphTargets = MPCPtr->ActiveMorphTargets;

		/*
		if (ActiveMorphTargets.Num() == 0){
			auto* p = Cast<USkeletalMeshComponent>(MPCPtr);
			if (p) {
				auto* a = p->GetAnimInstance();
				if (a) {
					TArray<FName> names;
					a->GetActiveCurveNames(EAnimCurveType::MorphTargetCurve, names);
					for (const auto& n : names) {
						a->GetCurveValue(n);
					}
				}
			}
		}
		*/
	}
#endif
}

void UVrmPoseableMeshComponent::VRMCopyPoseAndMorphFromSkeletalComponent(USkeletalMeshComponent* InComponentToCopy) {
	if (InComponentToCopy) {
		Super::CopyPoseFromSkeletalComponent(InComponentToCopy);

		MorphTargetWeights = InComponentToCopy->MorphTargetWeights;
		ActiveMorphTargets = InComponentToCopy->ActiveMorphTargets;
	}
}

void UVrmPoseableMeshComponent::RefreshBoneTransforms(FActorComponentTickFunction* TickFunction)
{
	Super::RefreshBoneTransforms(TickFunction);

#if	UE_VERSION_OLDER_THAN(5,1,0)
	USkinnedMeshComponent* MPCPtr = MasterPoseComponent.Get();
#else
	USkinnedMeshComponent* MPCPtr = LeaderPoseComponent.Get();
#endif
	if (MPCPtr) {
		MorphTargetWeights = MPCPtr->MorphTargetWeights;
		ActiveMorphTargets = MPCPtr->ActiveMorphTargets;
	}
}
