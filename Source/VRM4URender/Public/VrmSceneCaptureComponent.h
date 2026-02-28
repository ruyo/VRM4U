// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "Components/SceneCaptureComponent2D.h"
#include "VrmSceneCaptureComponent.generated.h"

//DECLARE_LOG_CATEGORY_EXTERN(LogCineCapture, Log, All);

class FVrmSceneCaptureSceneViewExtension;
class UCineCameraComponent;

/**
* Cine Capture Component extends Scene Capture to allow users to render Cine Camera Component into a render target. 
* Cine Capture has a few modifiable properties, but most of the properties are controlled by Cine Camera Component.
* Cine Capture Component is required to be parented to Cine Camera Component or a class that extends it.
* 
*/
UCLASS(meta = (BlueprintSpawnableComponent))
//UCLASS(hidecategories = (Transform, Collision, Object, Physics, SceneComponent, PostProcessVolume, Projection, Rendering, PlanarReflection), ClassGroup = Rendering, editinlinenew, meta = (BlueprintSpawnableComponent))
class VRM4URENDER_API UVrmSceneCaptureComponent2D : public USceneCaptureComponent2D
{
	GENERATED_UCLASS_BODY()
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture Settings")
	TObjectPtr<UTextureRenderTarget2D> RenderTargetA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture Settings")
	TObjectPtr<UTextureRenderTarget2D> RenderTargetB;

public:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	/** Used for validation of this componet's attachment to Cine Camera Component. */
	virtual void OnAttachmentChanged() override; 
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

private:
	/** This scene view extension is used to get ahold of views during the setup process. */
	TSharedPtr<FVrmSceneCaptureSceneViewExtension, ESPMode::ThreadSafe> SceneViewExtension;
	//TSharedPtr<class FVrmSceneViewExtension, ESPMode::ThreadSafe> SceneViewExtension;

};
