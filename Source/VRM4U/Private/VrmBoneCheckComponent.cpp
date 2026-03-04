// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmBoneCheckComponent.h"
#include "Components/SkinnedMeshComponent.h"

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

	USkinnedMeshComponent* skin = Cast<USkinnedMeshComponent>(this->GetAttachParent());
	if (skin == nullptr) return;

	//USkeletalMeshComponent* c;
	//c->RegisterOnBoneTransformsFinalizedDelegate
	skin->RegisterOnBoneTransformsFinalizedDelegate(
	FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &UVrmBoneCheckComponent::OnTargetTransformUpdate));
}
void UVrmBoneCheckComponent::OnUnregister() {
	Super::OnUnregister();
}

void UVrmBoneCheckComponent::OnTargetTransformUpdate() {
	OnBoneTransform.Broadcast();
}

