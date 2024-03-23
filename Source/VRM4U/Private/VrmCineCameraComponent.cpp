// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmCineCameraComponent.h"
#include "VrmDrawFrustumComponent.h"
#include "Misc/EngineVersionComparison.h"

void UVrmCineCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) {
	Super::GetCameraView(DeltaTime, DesiredView);
	DesiredView.OffCenterProjectionOffset = OffCenterProjectionOffset;
}

void UVrmCineCameraComponent::OnRegister()
{
#if WITH_EDITORONLY_DATA
	AActor* MyOwner = GetOwner();
	if ((MyOwner != nullptr) && !IsRunningCommandlet())
	{
		if (DrawFrustum == nullptr)
		{
			DrawFrustum = NewObject<UVrmDrawFrustumComponent>(MyOwner, NAME_None, RF_Transactional | RF_TextExportTransient);
			DrawFrustum->SetupAttachment(this);
			DrawFrustum->CreationMethod = CreationMethod;
			DrawFrustum->RegisterComponentWithWorld(GetWorld());

#if	UE_VERSION_OLDER_THAN(4,21,0)
#else
			DrawFrustum->SetIsVisualizationComponent(true);
#endif

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
			DrawFrustum->bFrustumEnabled = bDrawFrustumAllowed;
#endif
		}
	}

	RefreshVisualRepresentation();
#endif	// WITH_EDITORONLY_DATA

	Super::OnRegister();
}

#if WITH_EDITORONLY_DATA
void UVrmCineCameraComponent::RefreshVisualRepresentation()
{
	if (DrawFrustum != nullptr)
	{
		UVrmDrawFrustumComponent* v = Cast< UVrmDrawFrustumComponent>(DrawFrustum);
		v->OffCenterProjectionOffset = OffCenterProjectionOffset;
	}
	Super::RefreshVisualRepresentation();
}

#endif
