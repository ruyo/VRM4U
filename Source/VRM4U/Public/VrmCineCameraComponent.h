// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraComponent.h"

#include "VrmCineCameraComponent.generated.h"


UCLASS(Blueprintable, ClassGroup = Camera, meta=(BlueprintSpawnableComponent))
class VRM4U_API UVrmCineCameraComponent : public UCineCameraComponent
{
	GENERATED_BODY()
	
public:

	virtual void OnRegister() override;
#if WITH_EDITORONLY_DATA
	// Refreshes the visual components to match the component state
	virtual void RefreshVisualRepresentation() override;
#endif


	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Vrm Camera Settings")
	FVector2D OffCenterProjectionOffset;

};
