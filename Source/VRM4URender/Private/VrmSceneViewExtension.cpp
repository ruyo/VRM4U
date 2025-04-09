// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmSceneViewExtension.h"
#include "Misc/EngineVersionComparison.h"
#include "Runtime/Renderer/Private/SceneRendering.h"

#include "VRM4U_RenderSubsystem.h"

#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
#include "DataDrivenShaderPlatformInfo.h"
#endif

#if	UE_VERSION_OLDER_THAN(5,3,0)
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessMaterial.h"
#else
#include "PostProcess/PostProcessMaterialInputs.h"
#endif

#if UE_VERSION_OLDER_THAN(5,4,0)
#include "Runtime/Renderer/Private/SceneTextures.h"
#endif


#include "ScreenPass.h"
#include "ShaderParameterUtils.h"


#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class FMyComputeShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMyComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FMyComputeShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		//RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};


BEGIN_SHADER_PARAMETER_STRUCT(FMyComputeShaderParameters, )
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	//RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

IMPLEMENT_GLOBAL_SHADER(FMyComputeShader, "/VRM4UShaders/private/BaseColorCS.usf", "MainCS", SF_Compute);




FVrmSceneViewExtension::FVrmSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {
}

void FVrmSceneViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) {

	UVRM4U_RenderSubsystem* s = GEngine->GetEngineSubsystem<UVRM4U_RenderSubsystem>();

	if (s == nullptr) return;

	if (s->bUsePostRenderBasePass == false) return;

	//decltype(auto) View = InView;
	//check(View.bIsViewInfo);
	//const FSceneTextures& st = static_cast<const FViewInfo&>(View).GetSceneTextures();


	// GBuffer の BaseColor を取得（例: GBufferA に格納されていると仮定）
	//FRDGTextureRef GBufferBaseColorTexture = SceneTextures.GBufferA;
	//FRDGTextureRef GBufferBaseColorTexture = GraphBuilder.RegisterExternalTexture(
	//	InView.Scene->SceneTexturesUniformBuffer->GetRDGTexture("GBufferATexture"), // 仮の取得方法
	//	TEXT("GBufferBaseColor")
	//);
	//PassTextures.Depth = SceneTextures.Depth;
	//PassTextures.Color = SceneTextures.Color.Target;
	//PassTextures.GBufferA = (*SceneTextures.UniformBuffer)->GBufferATexture;
	//PassTextures.GBufferB = (*SceneTextures.UniformBuffer)->GBufferBTexture;
	//PassTextures.GBufferC = (*SceneTextures.UniformBuffer)->GBufferCTexture;
	//PassTextures.GBufferE = (*SceneTextures.UniformBuffer)->GBufferETexture;
	//PassTextures.DBufferTextures = DBufferTextures;

	//FRDGTextureRef GBufferBaseColorTexture = (*SceneTextures.UniformBuffer)->GBufferATexture;

	//FScreenPassTexture ScreenPassTex(SceneTextures->GetParameters()->GBufferATexture);
	//RDG_EVENT_SCOPE(GraphBuilder, "Naga_PostRenderBasePassDeferred %dx%d", ScreenPassTex.ViewRect.Width(), ScreenPassTex.ViewRect.Height());

//	FRDGTextureUAVRef GBufferUAV = GraphBuilder.CreateUAV(ScreenPassTex.Texture);


	{
		RenderTargets.DepthStencil.GetTexture();

		// RenderTargets の0番目を取得
		const FRenderTargetBinding& FirstTarget = RenderTargets[0];
		//if (!FirstTarget.IsValid()) return;

		FRDGTextureRef SourceTexture = FirstTarget.GetTexture();
		if (!SourceTexture) return;

		// コピー先のテクスチャを作成
		FRDGTextureDesc CopyDesc = SourceTexture->Desc;
		FRDGTextureRef CopiedTexture = GraphBuilder.CreateTexture(
			CopyDesc,
			TEXT("CopiedGBuffer")
		);

		// テクスチャをコピー
		AddCopyTexturePass(
			GraphBuilder,
			SourceTexture,
			CopiedTexture,
			FIntPoint(0, 0),  // コピー元オフセット
			FIntPoint(0, 0)   // コピー先オフセット
		);

		// SRV（読み込み用）を作成
		FRDGTextureSRVRef InputSRV = GraphBuilder.CreateSRV(CopiedTexture);
	}

		// RenderTargets の0番目を取得
	const FRenderTargetBinding& FirstTarget = RenderTargets[0];
	//if (!FirstTarget.IsValid())
	//{
	//	return; // ターゲットが無効な場合スキップ
	//}

	// FRDGTexture を取得
	FRDGTextureRef TargetTexture = FirstTarget.GetTexture();
	if (!TargetTexture)
	{
		return; // テクスチャが取得できない場合スキップ
	}

	// UAV を作成
	FRDGTextureUAVRef GBufferUAV = GraphBuilder.CreateUAV(TargetTexture);


	FMyComputeShader::FParameters* Parameters = GraphBuilder.AllocParameters<FMyComputeShader::FParameters>();
	Parameters->OutputTexture = GBufferUAV;
	//Parameters->RenderTargets = RenderTargets;

	TShaderMapRef<FMyComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("CustomGBufferWrite"),
		ComputeShader,
		Parameters,
		FIntVector(TargetTexture->Desc.Extent.X / 8, TargetTexture->Desc.Extent.Y / 8, 1)
	);
	/*
	//FRDGBuilder GraphBuilder(FRHICommandListExecutor::GetImmediateCommandList());

	//RenderTargets.

	// Render TargetをRDGリソースに変換
	FRDGTextureRef OutputTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(RenderTarget->GameThread_GetRenderTargetResource(), TEXT("OutputTexture")));
	FRDGTextureUAVRef OutputUAV = GraphBuilder.CreateUAV(OutputTexture);

	// パラメータを設定
	FMyComputeShader::FParameters* Parameters = GraphBuilder.AllocParameters<FMyComputeShader::FParameters>();
	Parameters->OutputTexture = OutputUAV;

	// Shaderを追加してディスパッチ
	TShaderMapRef<FMyComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("MyComputeShader"),
		ComputeShader,
		Parameters,
		FIntVector(RenderTarget->SizeX / 8, RenderTarget->SizeY / 8, 1) // スレッドグループ数
	);

	GraphBuilder.Execute();
	*/


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


bool FVrmSceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const  {
	return FSceneViewExtensionBase::IsActiveThisFrame_Internal(Context);
}
