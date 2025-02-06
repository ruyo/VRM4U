// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmPoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "VrmAnimInstance.h"
#include "VrmUtil.h"




UVrmPoseableMeshComponent::UVrmPoseableMeshComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;

	// for morph curve copy
	bTickInEditor = true;
}

void UVrmPoseableMeshComponent::OnRegister() {
	Super::OnRegister();

	if (bUseDefaultMaterial) {
		this->OverrideMaterials.Empty();
	}
	Init();
	UpdateLeader();
}

#if WITH_EDITOR
void UVrmPoseableMeshComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateLeader();
}
void UVrmPoseableMeshComponent::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) {
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}
#endif

void UVrmPoseableMeshComponent::OnAttachmentChanged() {
	Super::OnAttachmentChanged();
}

void UVrmPoseableMeshComponent::InitializeComponent() {
	Super::InitializeComponent();
}

void UVrmPoseableMeshComponent::UpdateLeader() {

	if (bUseParentAsLeader) {
		USkinnedMeshComponent* skin = Cast<USkinnedMeshComponent>(this->GetAttachParent());
		USkeletalMeshComponent* skel = Cast<USkeletalMeshComponent>(skin);
#if	UE_VERSION_OLDER_THAN(5,1,0)
		SetMasterPoseComponent(skin);
#else
		SetLeaderPoseComponent(skin);
#endif  
		VRMCopyPoseAndMorphFromSkeletalComponent(skel);
	}
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
	}
#endif
}

void UVrmPoseableMeshComponent::VRMCopyPoseAndMorphFromSkeletalComponent(USkeletalMeshComponent* InComponentToCopy) {
	if (InComponentToCopy && VRMGetSkinnedAsset(InComponentToCopy)) {
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
