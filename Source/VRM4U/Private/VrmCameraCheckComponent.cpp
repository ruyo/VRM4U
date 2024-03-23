// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmCameraCheckComponent.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#endif




UVrmCameraCheckComponent::UVrmCameraCheckComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UVrmCameraCheckComponent::OnRegister() {
	Super::OnRegister();
}
void UVrmCameraCheckComponent::OnUnregister() {
	Super::OnUnregister();
}

#if WITH_EDITOR
void UVrmCameraCheckComponent::OnCameraTransformChanged(const FVector&, const FRotator&, ELevelViewportType, int32) {
	OnCameraMove.Broadcast();
}
#endif

void UVrmCameraCheckComponent::SetCameraCheck(bool bCheckOn) {
#if WITH_EDITOR
	if (bCheckOn) {
		handle = FEditorDelegates::OnEditorCameraMoved.AddUObject(this, &UVrmCameraCheckComponent::OnCameraTransformChanged);
	} else {
		if (handle.IsValid()) {
			FEditorDelegates::OnEditorCameraMoved.Remove(handle);
		}
	}
#endif
}

