// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmSceneViewExtension.h"
#include "Misc/EngineVersionComparison.h"
#include "Runtime/Renderer/Private/SceneRendering.h"
#include "VRM4U_RenderSubsystem.h"

#if	UE_VERSION_OLDER_THAN(5,3,0)
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessMaterial.h"
#else
#include "PostProcess/PostProcessMaterialInputs.h"
#endif

#if UE_VERSION_OLDER_THAN(5,4,0)
#include "Runtime/Renderer/Private/SceneTextures.h"
#endif

FVrmSceneViewExtension::FVrmSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {
}

void FVrmSceneViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) {

	//decltype(auto) View = InView;
	//check(View.bIsViewInfo);
	//const FSceneTextures& st = static_cast<const FViewInfo&>(View).GetSceneTextures();
}
void FVrmSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) {

	//const FSceneView& View = Views[0];
	//check(View.bIsViewInfo);
	//const FMinimalSceneTextures& SceneTextures = static_cast<const FViewInfo&>(View).GetSceneTextures();
}
void FVrmSceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) {
	if (Pass == EPostProcessingPass::FXAA)
	{
		//InOutPassCallbacks.Add(
		//	FAfterPassCallbackDelegate::CreateRaw(this, &FVrmSceneViewExtension::AfterTonemap_RenderThread));
	}
}

FScreenPassTexture FVrmSceneViewExtension::AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& InOutInputs) {

	UVRM4U_RenderSubsystem* s = GEngine->GetEngineSubsystem<UVRM4U_RenderSubsystem>();
	if (s && s->CaptureList.Num()){
	
#if	UE_VERSION_OLDER_THAN(5,3,0)
		decltype(auto) View = static_cast<const FViewInfo&>(InView);
#else
		decltype(auto) View = InView;
#endif

		FRDGTextureRef DstRDGTex = nullptr;
		FRDGTextureRef SrcRDGTex = nullptr;

		for (auto c : s->CaptureList) {
			//switch (c.Value) {
			//case EVRM4U_CaptureSource::FinalColor:
			//	DstRDGTex = RegisterExternalTexture(GraphBuilder, c.Key->GetRenderTargetResource()->GetTexture2DRHI(), TEXT("VRM4U_CopyDst"));
			//	break;
			//default:
			//	break;
			//}
		}

#if	UE_VERSION_OLDER_THAN(5,4,0)

		if (DstRDGTex) {
			FScreenPassRenderTarget DstTex(DstRDGTex, ERenderTargetLoadAction::EClear);
			FScreenPassTexture SrcTex = const_cast<FScreenPassTexture&>(InOutInputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor]);

			AddDrawTexturePass(
				GraphBuilder,
				View,
				SrcTex,
				DstTex
			);
		}
#else
		if (DstRDGTex) {
			FScreenPassRenderTarget DstTex(DstRDGTex, ERenderTargetLoadAction::EClear);
			FScreenPassTexture SrcTex((InOutInputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor]));

			AddDrawTexturePass(
				GraphBuilder,
				View,
				SrcTex,
				DstTex
			);
		}
#endif
	}

	if (InOutInputs.OverrideOutput.IsValid())
	{
		return InOutInputs.OverrideOutput;
	}
	else
	{
#if	UE_VERSION_OLDER_THAN(5,4,0)
		/** We don't want to modify scene texture in any way. We just want it to be passed back onto the next stage. */
		FScreenPassTexture SceneTexture = const_cast<FScreenPassTexture&>(InOutInputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor]);
		return SceneTexture;
#else
		FScreenPassTexture SceneTexture((InOutInputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor]));
		return SceneTexture;
#endif
	}
}

