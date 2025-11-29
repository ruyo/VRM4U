// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmSceneViewExtension.h"
#include "VrmExtensionRimFilterData.h"
#include "Misc/EngineVersionComparison.h"
#include "Runtime/Renderer/Private/SceneRendering.h"

#include "VRM4U_RenderSubsystem.h"

#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
#include "DataDrivenShaderPlatformInfo.h"
#include "SystemTextures.h"
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

#include "RenderGraphEvent.h"
#include "RenderGraphBuilder.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Stats/Stats.h"
#include "DynamicRHI.h"

DECLARE_GPU_STAT(VRM4U);


class FMyComputeShader : public FGlobalShader
{

	DECLARE_GLOBAL_SHADER(FMyComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FMyComputeShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, BaseColorTexture)

		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, NormalTexture)

		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, SceneDepthTexture)
		//SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CustomDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, CustomDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<uint2>, CustomStencilTexture)
		//SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)

		//SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture) // 深度
		//SHADER_PARAMETER_SAMPLER(SamplerState, DepthSampler)

		//SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture) // カラー
		//SHADER_PARAMETER_SAMPLER(SamplerState, ColorSampler)

		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)


		SHADER_PARAMETER(FVector3f, CameraPosition)
		SHADER_PARAMETER(FVector3f, CameraForwardVector)

		SHADER_PARAMETER(float, UseCustomLightPosition)
		SHADER_PARAMETER(float, UseCustomLightColor)
		SHADER_PARAMETER(FVector3f, LightPosition)
		SHADER_PARAMETER(FVector3f, LightDirection)
		SHADER_PARAMETER(FVector3f, LightColor)
		SHADER_PARAMETER(float, LightScale)
		SHADER_PARAMETER(float, RimEdgeFade)

		SHADER_PARAMETER(float, UseBinaryEdge)
		SHADER_PARAMETER(float, RimEdgeBinaryRange)

		SHADER_PARAMETER(float, SampleScreenScale)
		SHADER_PARAMETER(float, SampleScale)

		SHADER_PARAMETER(int, CustomStencilMask)

		//RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

/*
BEGIN_SHADER_PARAMETER_STRUCT(FMyComputeShaderParameters, )
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneColorTexture)
	//RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()
*/

#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
IMPLEMENT_GLOBAL_SHADER(FMyComputeShader, "/VRM4UShaders/private/BaseColorCS.usf", "MainCS", SF_Compute);
#endif


static bool LocalCSEnable()
{

	static const auto CVarCustomDepth = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.CustomDepth"));
	const int32 EnabledWithStencil = 3;// CustomDepthMode::EnabledWithStencil ではダメ。
	if (CVarCustomDepth) {
		if (CVarCustomDepth->GetValueOnAnyThread() != EnabledWithStencil) {
			return false;
		}
	}

	if (GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM6) {
		return true;
	}
	return false;
}

static void LocalCopyFilter(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters>& SceneTextures) {
#if	UE_VERSION_OLDER_THAN(5,4,0)
#else
	FScreenPassViewInfo ViewInfo(InView);
#endif

	// copy枚数
	const int copyNum = 4;

	FRDGTextureDesc CopyDesc[copyNum] = {};
	FRDGTextureRef CopyTexture[copyNum] = {};
	FRDGTextureRef CopyTextureDepth[copyNum] = {};

	{

#if	UE_VERSION_OLDER_THAN(5,5,0)
#else
		{
			// depth copy
			RDG_EVENT_SCOPE_STAT(GraphBuilder, VRM4U, "VRM4U::Copy2");

			FRDGTextureRef SourceTexture = SceneTextures->GetParameters()->SceneDepthTexture;
			if (!SourceTexture) return;

			for (int i = 0; i < copyNum; ++i) {
				CopyDesc[i] = SourceTexture->Desc;
				if (i == 0) {
				}
				else {
					CopyDesc[i].Extent = CopyDesc[i - 1].Extent / 2;
				}
				CopyDesc[i].Flags |= TexCreate_RenderTargetable | TexCreate_UAV;

				FString name = TEXT("CopiedGBuffer2") + FString::FromInt(i);
				CopyTextureDepth[i] = GraphBuilder.CreateTexture(
					CopyDesc[i],
					*name
				);
				// テクスチャをコピー
				if (i == 0) {
					AddCopyTexturePass(
						GraphBuilder,
						SourceTexture,
						CopyTextureDepth[i]
					);
				}
				else {
					AddDrawTexturePass(
						GraphBuilder,
						ViewInfo,
						//SourceTexture,
						CopyTextureDepth[i - 1],
						CopyTextureDepth[i],
						FIntPoint(0, 0), CopyDesc[i - 1].Extent,
						FIntPoint(0, 0), CopyDesc[i].Extent
					);
				}
			}
		}
#endif


#if	UE_VERSION_OLDER_THAN(5,5,0)
#else
		RDG_EVENT_SCOPE_STAT(GraphBuilder, VRM4U, "VRM4U::Copy");
#endif
		RDG_GPU_STAT_SCOPE(GraphBuilder, VRM4U);
		SCOPED_NAMED_EVENT(VRM4U, FColor::Emerald);

		// RenderTargets の0番目を取得
		const FRenderTargetBinding& FirstTarget = RenderTargets[0];

		FRDGTextureRef SourceTexture = FirstTarget.GetTexture();
		if (!SourceTexture) return;


		for (int i = 0; i < copyNum; ++i) {
			CopyDesc[i] = SourceTexture->Desc;
			if (i == 0) {
			}
			else {
				CopyDesc[i].Extent = CopyDesc[i - 1].Extent / 2;
			}
			CopyDesc[i].Flags |= TexCreate_RenderTargetable | TexCreate_UAV;

			FString name = TEXT("CopiedGBuffer") + FString::FromInt(i);
			CopyTexture[i] = GraphBuilder.CreateTexture(
				CopyDesc[i],
				*name
			);
			// テクスチャをコピー
			if (i == 0) {
				AddCopyTexturePass(
					GraphBuilder,
					SourceTexture,
					CopyTexture[i]
				);
			}
			else {
#if	UE_VERSION_OLDER_THAN(5,4,0)
#elif UE_VERSION_OLDER_THAN(5,5,0)
				AddDrawTexturePass(
					GraphBuilder,
					InView,
					//SourceTexture,
					CopyTexture[i - 1],
					CopyTexture[i],
					FIntPoint(0, 0), CopyDesc[0].Extent,
					FIntPoint(0, 0), CopyDesc[i - 1].Extent
				);
#else
				AddDrawTexturePass(
					GraphBuilder,
					ViewInfo,
					//SourceTexture,
					CopyTexture[i - 1],
					CopyTexture[i],
					FIntPoint(0, 0), CopyDesc[i - 1].Extent,
					FIntPoint(0, 0), CopyDesc[i].Extent
				);
#endif
				//				AddCopyTexturePass(
				//					GraphBuilder,
				//					CopyTexture[i-1],
				//					CopyTexture[i]
				//				);
			}
		}

		//RenderTargets.DepthStencil.GetTexture();


		// SRV（読み込み用）を作成
		//FRDGTextureSRVRef InputSRV = GraphBuilder.CreateSRV(CopyTexture[0]);
	}

}


static void LocalRimFilter(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> &SceneTextures) {

#if	UE_VERSION_OLDER_THAN(5,2,0)
#else

	UVRM4U_RenderSubsystem* s = GEngine->GetEngineSubsystem<UVRM4U_RenderSubsystem>();
	if (s == nullptr) return;

	auto data = s->GenerateFilterData();

	for (int dataIndex = 0; dataIndex < data.Num(); ++dataIndex){
		const auto& d = data[dataIndex];

		if (RenderTargets[0].GetTexture() == nullptr) continue;
		if (RenderTargets[1].GetTexture() == nullptr) continue;
		if (RenderTargets[2].GetTexture() == nullptr) continue;
		if (RenderTargets[3].GetTexture() == nullptr) continue;

		// RenderTargets
		// 0 SceneColor
		// 1 A normal
		// 2 B MRS
		// 3 C basecolor
		// 4 D subsurface


		const FRDGSystemTextures& SystemTextures = FRDGSystemTextures::Get(GraphBuilder);
		//const FCustomDepthTextures& CustomDepthTextures = SceneTextures->CustomDepth;
		//bool bCustomDepthProduced = HasBeenProduced(CustomDepthTextures.Depth);

		FMyComputeShader::FParameters* Parameters = GraphBuilder.AllocParameters<FMyComputeShader::FParameters>();

		{
			// SceneColorTexture
			const FRenderTargetBinding& FirstTarget = RenderTargets[0];
			FRDGTextureRef TargetTexture = FirstTarget.GetTexture();
			FRDGTextureUAVRef GBufferUAV = GraphBuilder.CreateUAV(TargetTexture);
			Parameters->SceneColorTexture = GBufferUAV;
		}

		{
			// BaseColorTexture
			//const FRenderTargetBinding& FirstTarget = RenderTargets[3];
			//FRDGTextureRef TargetTexture = FirstTarget.GetTexture();
			//FRDGTextureUAVRef GBufferUAV = GraphBuilder.CreateUAV(TargetTexture);
			//Parameters->BaseColorTexture = GBufferUAV;
		}

		//Parameters->NormalTexture = GraphBuilder.CreateSRV(SceneTextures->GetParameters()->GBufferATexture);
		Parameters->NormalTexture = GraphBuilder.CreateSRV(RenderTargets[1].GetTexture());

		Parameters->SceneDepthTexture = GraphBuilder.CreateSRV(SceneTextures->GetParameters()->SceneDepthTexture);

		Parameters->CustomDepthTexture = GraphBuilder.CreateSRV(SceneTextures->GetParameters()->CustomDepthTexture);
		//Parameters->CustomDepthTexture = SceneTextures->GetParameters()->CustomDepthTexture;
		Parameters->CustomStencilTexture = (SceneTextures->GetParameters()->CustomStencilTexture);
		//Parameters->CustomStencilTexture = bCustomDepthProduced ? CustomDepthTextures.Stencil : SystemTextures.StencilDummySRV;
		//Parameters->InputTexture = GraphBuilder.CreateSRV(CopyTexture[1]);
		//Parameters->SceneDepthTexture = SceneTextures->GetParameters()->SceneDepthTexture;
		//Parameters->SceneDepthTexture = SceneTextures->GetParameters()->GBufferBTexture;
		//check(DepthTexture);
		//Parameters->DepthSampler = TStaticSamplerState<SF_Point>::GetRHI();

		Parameters->View = InView.ViewUniformBuffer;
		//Parameters->SceneTextures = SceneTextures;
		//Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();
		//PassParameters->SceneColorTexture = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(SceneColorTexture));

		Parameters->CameraPosition = FVector3f(InView.ViewMatrices.GetViewOrigin());
		Parameters->CameraForwardVector = FVector3f(InView.ViewRotation.RotateVector(FVector(1, 0, 0)));

		Parameters->UseCustomLightPosition = d.bUseCustomLighPosition;
		Parameters->LightPosition = FVector3f(d.LightPosition);
		Parameters->LightDirection = FVector3f(d.LightDirection);

		Parameters->UseCustomLightColor = d.bUseCustomLighColor;
		Parameters->LightColor = FVector3f(d.LightColor);
		Parameters->LightScale = d.LightScale;
		Parameters->RimEdgeFade = d.RimEdgeFade;

		Parameters->UseBinaryEdge = d.bUseBinaryEdge;
		Parameters->RimEdgeBinaryRange = d.RimEdgeBinaryRange;

		Parameters->SampleScreenScale = d.SampleScreenScale;
		Parameters->SampleScale = d.SampleScale;

		Parameters->CustomStencilMask = d.CustomStencilMask;

		int Width = RenderTargets[0].GetTexture()->Desc.Extent.X;
		int Height = RenderTargets[0].GetTexture()->Desc.Extent.Y;

		TShaderMapRef<FMyComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("VRM4U_CustomGBufferWrite %d", dataIndex),
			ComputeShader,
			Parameters,
			FIntVector(Width / 8, Height / 8, 1)
		);
		/*
		//FRDGBuilder GraphBuilder(FRHICommandListExecutor::GetImmediateCommandList());

		//RenderTargets.

		// Render TargetをRDGリソースに変換
		FRDGTextureRef SceneColorTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(RenderTarget->GameThread_GetRenderTargetResource(), TEXT("SceneColorTexture")));
		FRDGTextureUAVRef OutputUAV = GraphBuilder.CreateUAV(SceneColorTexture);

		// パラメータを設定
		FMyComputeShader::FParameters* Parameters = GraphBuilder.AllocParameters<FMyComputeShader::FParameters>();
		Parameters->SceneColorTexture = OutputUAV;

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

#endif


}

///////////////////////

FVrmSceneViewExtension::FVrmSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {
}

void FVrmSceneViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) {

	if (LocalCSEnable() == false) return;

	const auto FeatureLevel = InView.GetFeatureLevel();
	if (FeatureLevel <= ERHIFeatureLevel::SM5) return;

	{
		bool bActive = false;

		//bool bCapture = InView.bIsSceneCapture;

		auto t = InView.Family->Scene->GetWorld()->WorldType;
		switch (t) {
		case EWorldType::Editor:
		case EWorldType::Game:
		case EWorldType::PIE:
			bActive = true;
			break;
		}

		if (bActive == false) {
			return;
		}
	}

	//check(InView.bIsViewInfo);
	//auto ViewInfo = static_cast<const FViewInfo*>(&InView);


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


	///// filter
#if	UE_VERSION_OLDER_THAN(5,2,0)
#else


	bool bUseRimFilter = false;
	{
		LocalRimFilter(GraphBuilder, InView, RenderTargets, SceneTextures);
	}

#endif

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
