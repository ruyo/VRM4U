// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"


class FVrmSceneViewExtension : public FSceneViewExtensionBase
{

public:

	FVrmSceneViewExtension(const FAutoRegister& AutoRegister);

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& RHICmdList, FSceneViewFamily& InViewFamily) override {}

	virtual void PreRenderView_RenderThread(FRDGBuilder& RHICmdList, FSceneView& InView) override {}
	virtual void PreInitViews_RenderThread(FRDGBuilder& GraphBuilder) override {}

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
	//virtual void PostRenderBasePassMobile_RenderThread(FRHICommandList& RHICmdList, FSceneView& InView) override {};

	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

	//virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}
	//virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;


	virtual int32 GetPriority() const override { return 100; }
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

	FScreenPassTexture AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs);

};
