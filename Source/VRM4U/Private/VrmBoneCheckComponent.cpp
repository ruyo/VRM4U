// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmBoneCheckComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/EngineVersionComparison.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#endif




UVrmBoneCheckComponent::UVrmBoneCheckComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

void UVrmBoneCheckComponent::OnRegister() {
	Super::OnRegister();

#if	UE_VERSION_OLDER_THAN(4,27,0)
#else

	USkeletalMeshComponent* skc = Cast<USkeletalMeshComponent>(this->GetAttachParent());
	if (skc == nullptr) return;

	//USkeletalMeshComponent* c;
	//c->RegisterOnBoneTransformsFinalizedDelegate
	skc->RegisterOnBoneTransformsFinalizedDelegate(
	FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &UVrmBoneCheckComponent::OnTargetTransformUpdate));
#endif
}
void UVrmBoneCheckComponent::OnUnregister() {
	Super::OnUnregister();
}

void UVrmBoneCheckComponent::OnTargetTransformUpdate() {
	OnBoneTransform.Broadcast();
}

