
#include "VRM4URender.h"
#include "CoreMinimal.h"
#include "VRM4URenderLog.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Internationalization/Internationalization.h"


#include "VrmBPFunctionLibrary.h"
#include "VRM4U_RenderSubsystem.h"
#include "VrmExtensionRimFilterData.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"
#include "EngineModule.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "Engine/SkeletalMesh.h"
#include "RendererInterface.h"
#include "ScreenPass.h"
#include "Runtime/Renderer/Private/SceneRendering.h"
#include "SceneRenderTargetParameters.h"
#include "Slate/SceneViewport.h"
#include "ScreenRendering.h"
#include "SceneView.h"
#include "SceneRendering.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#if UE_VERSION_OLDER_THAN(5,4,0)
#else
#include "Substrate/Substrate.h"
#endif

#if	UE_VERSION_OLDER_THAN(5,5,0)
#include "DataDrivenShaderPlatformInfo.h"
#endif

#define LOCTEXT_NAMESPACE "VRM4URender"

DEFINE_LOG_CATEGORY(LogVRM4URender);

class FCustomStencilCopyPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCustomStencilCopyPS);
	SHADER_USE_PARAMETER_STRUCT(FCustomStencilCopyPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<uint2>, CustomStencilTexture)
		SHADER_PARAMETER(FVector2f, SourceTextureSize)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

void FVRM4URenderModule::AddCustomStencilCopyPass(
	FRDGBuilder& GraphBuilder,
	FIntRect ViewRect,
	FRDGTextureSRVRef SrcStencilSRV,
	TObjectPtr<UTextureRenderTarget2D> RenderTarget)
{
	if (SrcStencilSRV == nullptr || RenderTarget == nullptr ||
		RenderTarget->GetRenderTargetResource() == nullptr)
	{
		return;
	}

	FRHITexture* DestTexture = RenderTarget->GetRenderTargetResource()->GetTextureRHI();
	if (DestTexture == nullptr || SrcStencilSRV->GetParent() == nullptr)
	{
		return;
	}

	FCustomStencilCopyPS::FParameters* Parameters =
		GraphBuilder.AllocParameters<FCustomStencilCopyPS::FParameters>();
	Parameters->CustomStencilTexture = SrcStencilSRV;
	Parameters->SourceTextureSize = FVector2f(SrcStencilSRV->GetParent()->Desc.Extent);

	const FIntPoint TargetSize(
		RenderTarget->GetRenderTargetResource()->GetSizeX(),
		RenderTarget->GetRenderTargetResource()->GetSizeY());
	const FIntPoint SourceTextureSize = SrcStencilSRV->GetParent()->Desc.Extent;

	TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FCustomStencilCopyPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	// destはRDGに登録せず、AddCopyPassと同じく生のRHIテクスチャへ手動でBeginRenderPassする。
	// (RDGのRENDER_TARGET_BINDING_SLOTS経由でミッドフレームに外部RTを登録すると
	//  環境によってPSO生成に失敗してFatalになるケースがあったため)
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("VRM4U_CustomStencilCopy"),
		Parameters,
		ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
		[Parameters, VertexShader, PixelShader, ViewRect, TargetSize, SourceTextureSize, DestTexture](FRHICommandListImmediate& RHICmdList)
		{
			FRHIRenderPassInfo RPInfo(DestTexture, ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(RPInfo, TEXT("VRM4U_CustomStencilCopy"));
			{
				RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);

				FGraphicsPipelineStateInitializer GraphicsPSOInit{};
				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
				GraphicsPSOInit.PrimitiveType = PT_TriangleList;
				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *Parameters);

				IRendererModule* RendererModule =
					&FModuleManager::GetModuleChecked<IRendererModule>(TEXT("Renderer"));
				RendererModule->DrawRectangle(
					RHICmdList,
					0, 0, TargetSize.X, TargetSize.Y,
					ViewRect.Min.X, ViewRect.Min.Y, ViewRect.Width(), ViewRect.Height(),
					TargetSize, SourceTextureSize, VertexShader, EDRF_Default);
			}
			RHICmdList.EndRenderPass();
	});
}

IMPLEMENT_GLOBAL_SHADER(FCustomStencilCopyPS, "/VRM4UShaders/Private/CustomStencilCopy.usf", "MainPS", SF_Pixel);

class FCustomDepthCopyPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCustomDepthCopyPS);
	SHADER_USE_PARAMETER_STRUCT(FCustomDepthCopyPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, CustomDepthTexture)
		SHADER_PARAMETER(FVector2f, SourceTextureSize)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

void FVRM4URenderModule::AddCustomDepthCopyPass(
	FRDGBuilder& GraphBuilder,
	FIntRect ViewRect,
	FRDGTextureSRVRef SrcDepthSRV,
	TObjectPtr<UTextureRenderTarget2D> RenderTarget)
{
	if (SrcDepthSRV == nullptr || RenderTarget == nullptr ||
		RenderTarget->GetRenderTargetResource() == nullptr)
	{
		return;
	}

	FRHITexture* DestTexture = RenderTarget->GetRenderTargetResource()->GetTextureRHI();
	if (DestTexture == nullptr || SrcDepthSRV->GetParent() == nullptr)
	{
		return;
	}

	FCustomDepthCopyPS::FParameters* Parameters =
		GraphBuilder.AllocParameters<FCustomDepthCopyPS::FParameters>();
	Parameters->CustomDepthTexture = SrcDepthSRV;
	Parameters->SourceTextureSize = FVector2f(SrcDepthSRV->GetParent()->Desc.Extent);

	const FIntPoint TargetSize(
		RenderTarget->GetRenderTargetResource()->GetSizeX(),
		RenderTarget->GetRenderTargetResource()->GetSizeY());
	const FIntPoint SourceTextureSize = SrcDepthSRV->GetParent()->Desc.Extent;

	TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FCustomDepthCopyPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("VRM4U_CustomDepthCopy"),
		Parameters,
		ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
		[Parameters, VertexShader, PixelShader, ViewRect, TargetSize, SourceTextureSize, DestTexture](FRHICommandListImmediate& RHICmdList)
		{
			FRHIRenderPassInfo RPInfo(DestTexture, ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(RPInfo, TEXT("VRM4U_CustomDepthCopy"));
			{
				RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);

				FGraphicsPipelineStateInitializer GraphicsPSOInit{};
				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
				GraphicsPSOInit.PrimitiveType = PT_TriangleList;
				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *Parameters);

				IRendererModule* RendererModule =
					&FModuleManager::GetModuleChecked<IRendererModule>(TEXT("Renderer"));
				RendererModule->DrawRectangle(
					RHICmdList,
					0, 0, TargetSize.X, TargetSize.Y,
					ViewRect.Min.X, ViewRect.Min.Y, ViewRect.Width(), ViewRect.Height(),
					TargetSize, SourceTextureSize, VertexShader, EDRF_Default);
			}
			RHICmdList.EndRenderPass();
	});
}

IMPLEMENT_GLOBAL_SHADER(FCustomDepthCopyPS, "/VRM4UShaders/Private/CustomDepthCopy.usf", "MainPS", SF_Pixel);

#if UE_VERSION_OLDER_THAN(5,4,0)

void FVRM4URenderModule::AddSubstrateBaseColorCopyPass(FRDGBuilder&, const FViewInfo&, TObjectPtr<UTextureRenderTarget2D>)
{
}

void FVRM4URenderModule::AddSubstrateNormalCopyPass(FRDGBuilder&, const FViewInfo&, TObjectPtr<UTextureRenderTarget2D>)
{
}

void FVRM4URenderModule::AddSubstrateMRSCopyPass(FRDGBuilder&, const FViewInfo&, TObjectPtr<UTextureRenderTarget2D>)
{
}

#else

class FSubstrateBaseColorCopyPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSubstrateBaseColorCopyPS);
	SHADER_USE_PARAMETER_STRUCT(FSubstrateBaseColorCopyPS, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSubstrateGlobalUniformParameters, Substrate)
		SHADER_PARAMETER(FVector2f, SourceTextureSize)
	END_SHADER_PARAMETER_STRUCT()
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) && Substrate::IsSubstrateEnabled();
	}
};

class FSubstrateNormalCopyPS : public FSubstrateBaseColorCopyPS
{
	DECLARE_GLOBAL_SHADER(FSubstrateNormalCopyPS);
	SHADER_USE_PARAMETER_STRUCT(FSubstrateNormalCopyPS, FSubstrateBaseColorCopyPS);
};

class FSubstrateMRSCopyPS : public FSubstrateBaseColorCopyPS
{
	DECLARE_GLOBAL_SHADER(FSubstrateMRSCopyPS);
	SHADER_USE_PARAMETER_STRUCT(FSubstrateMRSCopyPS, FSubstrateBaseColorCopyPS);
};

IMPLEMENT_GLOBAL_SHADER(FSubstrateBaseColorCopyPS, "/VRM4UShaders/Private/SubstrateBaseColorCopy.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FSubstrateNormalCopyPS, "/VRM4UShaders/Private/SubstrateNormalCopy.usf", "MainPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FSubstrateMRSCopyPS, "/VRM4UShaders/Private/SubstrateMRSCopy.usf", "MainPS", SF_Pixel);

template<typename TPixelShader>
static void AddSubstrateCopyPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, TObjectPtr<UTextureRenderTarget2D> RenderTarget, const TCHAR* PassName)
{
	const FSubstrateSceneData* SceneData = View.SubstrateViewData.SceneData;
	if (!SceneData || !SceneData->MaterialTextureArray || !SceneData->TopLayerTexture || !RenderTarget || !RenderTarget->GetRenderTargetResource()) return;
	FRHITexture* DestTexture = RenderTarget->GetRenderTargetResource()->GetTextureRHI();
	if (!DestTexture) return;

	typename TPixelShader::FParameters* Parameters = GraphBuilder.AllocParameters<typename TPixelShader::FParameters>();
	Parameters->ViewUniformBuffer = View.ViewUniformBuffer;
	// Substrate.ush declares the global Substrate uniform buffer. Binding the complete
	// buffer is required by D3D12 even though this pass only reads three of its fields.
	Parameters->Substrate = View.SubstrateViewData.SubstrateGlobalUniformParameters;
	Parameters->SourceTextureSize = FVector2f(SceneData->TopLayerTexture->Desc.Extent);
	const FIntPoint TargetSize(RenderTarget->GetRenderTargetResource()->GetSizeX(), RenderTarget->GetRenderTargetResource()->GetSizeY());
	const FIntPoint SourceSize = SceneData->TopLayerTexture->Desc.Extent;
	const FIntRect ViewRect = View.UnconstrainedViewRect;
	TShaderMapRef<FScreenVS> VertexShader(View.ShaderMap);
	TShaderMapRef<TPixelShader> PixelShader(View.ShaderMap);

	GraphBuilder.AddPass(RDG_EVENT_NAME("VRM4U_SubstrateCopy"), Parameters,
		ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
		[Parameters, VertexShader, PixelShader, ViewRect, TargetSize, SourceSize, DestTexture, PassName](FRHICommandListImmediate& RHICmdList)
		{
			FRHIRenderPassInfo RPInfo(DestTexture, ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(RPInfo, PassName);
			RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);
			FGraphicsPipelineStateInitializer PSO{};
			RHICmdList.ApplyCachedRenderTargets(PSO);
			PSO.BlendState = TStaticBlendState<>::GetRHI();
			PSO.RasterizerState = TStaticRasterizerState<>::GetRHI();
			PSO.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			PSO.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
			PSO.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			PSO.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			PSO.PrimitiveType = PT_TriangleList;
			SetGraphicsPipelineState(RHICmdList, PSO, 0);
			SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *Parameters);
			FModuleManager::GetModuleChecked<IRendererModule>(TEXT("Renderer")).DrawRectangle(
				RHICmdList, 0, 0, TargetSize.X, TargetSize.Y,
				ViewRect.Min.X, ViewRect.Min.Y, ViewRect.Width(), ViewRect.Height(),
				TargetSize, SourceSize, VertexShader, EDRF_Default);
			RHICmdList.EndRenderPass();
		});
}

void FVRM4URenderModule::AddSubstrateBaseColorCopyPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, TObjectPtr<UTextureRenderTarget2D> RenderTarget)
{
	AddSubstrateCopyPass<FSubstrateBaseColorCopyPS>(GraphBuilder, View, RenderTarget, TEXT("VRM4U_SubstrateBaseColorCopy"));
}

void FVRM4URenderModule::AddSubstrateNormalCopyPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, TObjectPtr<UTextureRenderTarget2D> RenderTarget)
{
	AddSubstrateCopyPass<FSubstrateNormalCopyPS>(GraphBuilder, View, RenderTarget, TEXT("VRM4U_SubstrateNormalCopy"));
}

void FVRM4URenderModule::AddSubstrateMRSCopyPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, TObjectPtr<UTextureRenderTarget2D> RenderTarget)
{
	AddSubstrateCopyPass<FSubstrateMRSCopyPS>(GraphBuilder, View, RenderTarget, TEXT("VRM4U_SubstrateMRSCopy"));
}

#endif

bool FVRM4URenderModule::isCaptureTarget(const FSceneView* View) {

	bool bCapture = false;

	bool bPlay = false;
	bool bSIE = false;
	bool bEditor = false;
	UVrmBPFunctionLibrary::VRMGetPlayMode(bPlay, bSIE, bEditor);

	UWorld* World = View->Family->Scene->GetWorld();
	if (World) {
		EWorldType::Type WorldType = World->WorldType;

		if (bPlay) {
			switch (WorldType) {
			case EWorldType::Game:
			case EWorldType::PIE:
				bCapture = true;
				break;
			}
		} else {
			switch (WorldType) {
			case EWorldType::Editor:
				bCapture = true;
				break;
			}
		}
	}
	if (View->bIsGameView) {
		bCapture = true;
	}
	if (View->bIsOfflineRender) {
		bCapture = true;
	}
	if (View->bIsSceneCapture) {
		bCapture = false;
	}

	return bCapture;
}

void FVRM4URenderModule::AddCopyPass(FRDGBuilder &GraphBuilder, FIntRect ViewRect, FRDGTextureRef SrcRDGTex, TObjectPtr<UTextureRenderTarget2D> RenderTarget) {

	if (SrcRDGTex == nullptr) {
		return;
	}
	if (RenderTarget == nullptr) {
		return;
	}

	//FPostOpaqueRenderParameters& Parameters
	//const FIntPoint ViewRectSize = FIntPoint(Parameters.ViewportRect.Width(), Parameters.ViewportRect.Height());

	AddPass(GraphBuilder, RDG_EVENT_NAME("VRM4UAddCopyPass"), [ViewRect, SrcRDGTex, RenderTarget](FRHICommandListImmediate& RHICmdList)
		{
			if (SrcRDGTex == nullptr) return;
			if (SrcRDGTex->GetRHI() == nullptr) return;

			const FIntPoint TargetSize(RenderTarget->GetRenderTargetResource()->GetSizeX(), RenderTarget->GetRenderTargetResource()->GetSizeY());

			FRHITexture* DestRenderTarget = RenderTarget->GetRenderTargetResource()->GetTextureRHI();

			FRHIRenderPassInfo RPInfo(DestRenderTarget, ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(RPInfo, TEXT("VRM4U_Copy"));
			{
				RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);

				const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel;
				FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(FeatureLevel);
				TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
				TShaderMapRef<FScreenPS> PixelShader(ShaderMap);

				FGraphicsPipelineStateInitializer GraphicsPSOInit;
				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = static_cast<FRHIVertexShader*>(VertexShader.GetRHIShaderBase(SF_Vertex));
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = static_cast<FRHIPixelShader*>(PixelShader.GetRHIShaderBase(SF_Pixel));
				GraphicsPSOInit.PrimitiveType = PT_TriangleList;
				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

				FRHITexture* SceneTexture = SrcRDGTex->GetRHI()->GetTexture2D();
				const FIntPoint SourceTextureSize(SceneTexture->GetSizeX(), SceneTexture->GetSizeY());

#if	UE_VERSION_OLDER_THAN(5,3,0)
				PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), SceneTexture);
#else
				FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
				PixelShader->SetParameters(BatchedParameters, TStaticSamplerState<SF_Bilinear>::GetRHI(), SceneTexture);
				RHICmdList.SetBatchedShaderParameters(RHICmdList.GetBoundPixelShader(), BatchedParameters);
#endif
				IRendererModule* RendererModule = &FModuleManager::GetModuleChecked<IRendererModule>(TEXT("Renderer"));
				RendererModule->DrawRectangle(
					RHICmdList,
					0, 0,									// Dest X, Y
					TargetSize.X, TargetSize.Y,				// Dest Width, Height
					ViewRect.Min.X, ViewRect.Min.Y,			// Source U, V (�R�s�[�J�n�ʒu)
					ViewRect.Width(), ViewRect.Height(),	// Source USize, VSize (�R�s�[�T�C�Y)
					TargetSize,								// Target buffer size
					SourceTextureSize,						// Source texture size
					VertexShader,
					EDRF_Default);
			}
			RHICmdList.EndRenderPass();
		});
}


//////////////////////////////////////////////////////////////////////////
// FSpriterImporterModule

#if WITH_EDITOR
void FVRM4URenderModule::OnMapChange(UWorld* World, EMapChangeType ChangeType) {
	if (ChangeType == EMapChangeType::TearDownWorld)
	{
		CaptureList.Empty();
	}
}

void FVRM4URenderModule::OnPIEEvent(bool bPIEBegin, bool bPIEEnd) {
	bIsPlay = bPIEBegin;
}
#endif


void FVRM4URenderModule::StartupModule(){
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("VRM4U"))->GetBaseDir(), TEXT("Shaders"));

	AddShaderSourceDirectoryMapping(TEXT("/VRM4UShaders"), PluginShaderDir);

	GetRendererModule().RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateRaw(this, &FVRM4URenderModule::OnPostOpaque));
	GetRendererModule().RegisterOverlayRenderDelegate(FPostOpaqueRenderDelegate::CreateRaw(this, &FVRM4URenderModule::OnOverlay));

#if WITH_EDITOR
#else
	bIsPlay = true;
#endif
}

void FVRM4URenderModule::ShutdownModule(){
}

void FVRM4URenderModule::OnPostOpaque(FPostOpaqueRenderParameters& Parameters) {

	if (CaptureList.Num() == 0) return;

	if (isCaptureTarget(Parameters.View) == false) {
		return;
	}

	{
		TArray<TObjectPtr<UTextureRenderTarget2D>> keys;
		CaptureList.GetKeys(keys);
		for (auto k : keys) {
			if (IsValid(k) == false) {
				CaptureList.Remove(k);
				continue;
			}
			if (k->GetRenderTargetResource() == nullptr) {
				CaptureList.Remove(k);
				continue;
			}
			if (k->GetRenderTargetResource()->GetTexture2DRHI() == nullptr) {
				CaptureList.Remove(k);
				continue;
			}
		}
	}

	FRDGTextureRef DstRDGTex = nullptr;
	FRDGTextureRef SrcRDGTex = nullptr;

	for (auto c : CaptureList) {
		if (c.Key == nullptr) {
			continue;
		}
		if (c.Key->GetRenderTargetResource() == nullptr) {
			continue;
		}

		DstRDGTex = RegisterExternalTexture(*(Parameters.GraphBuilder), c.Key->GetRenderTargetResource()->GetTexture2DRHI(), TEXT("VRM4U_CopyDst"));
		switch (c.Value) {
		case EVRM4U_CaptureSource::ColorTexturePostOpaque:
			SrcRDGTex = Parameters.ColorTexture;
			break;
		case EVRM4U_CaptureSource::DepthTexture:
			SrcRDGTex = Parameters.DepthTexture;
			break;
		case EVRM4U_CaptureSource::NormalTexture:
			SrcRDGTex = Parameters.NormalTexture;
			break;
		case EVRM4U_CaptureSource::VelocityTexture:
			SrcRDGTex = Parameters.VelocityTexture;
			break;
			//case EVRM4U_CaptureSource::SmallDepthTexture:
			//	SrcRDGTex = Parameters.SmallDepthTexture;
			//	break;
		default:
			break;
		}

		if (SrcRDGTex == nullptr && Parameters.SceneTexturesUniformParams == nullptr) continue;
		switch (c.Value) {
		case EVRM4U_CaptureSource::SceneColorTexturePostOpaque:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->SceneColorTexture;
			break;
		case EVRM4U_CaptureSource::SceneDepthTexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->SceneDepthTexture;
			break;
			//case EVRM4U_CaptureSource::ScenePartialDepthTexture:
			//	SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->ScenePartialDepthTexture;
			//	break;

				// GBuffer
		case EVRM4U_CaptureSource::GBufferATexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->GBufferATexture;
			break;
		case EVRM4U_CaptureSource::GBufferBTexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->GBufferBTexture;
			break;
		case EVRM4U_CaptureSource::GBufferCTexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->GBufferCTexture;
			break;
		case EVRM4U_CaptureSource::GBufferDTexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->GBufferDTexture;
			break;
		case EVRM4U_CaptureSource::GBufferETexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->GBufferETexture;
			break;
		case EVRM4U_CaptureSource::GBufferFTexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->GBufferFTexture;
			break;
		case EVRM4U_CaptureSource::GBufferVelocityTexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->GBufferVelocityTexture;
			break;
		case EVRM4U_CaptureSource::ScreenSpaceAOTexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->ScreenSpaceAOTexture;
			break;
		case EVRM4U_CaptureSource::CustomDepthTexture:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->CustomDepthTexture;
			break;
		default:
			break;
		}

		if (DstRDGTex && SrcRDGTex) {
			/*
			FScreenPassRenderTarget DstTex(DstRDGTex, ERenderTargetLoadAction::EClear);
			FScreenPassTexture SrcTex(SrcRDGTex);

			AddDrawTexturePass(
				*(Parameters.GraphBuilder),
				*Parameters.View,
				SrcTex,
				DstTex
			);
			*/
			FVRM4URenderModule::AddCopyPass(*Parameters.GraphBuilder, Parameters.ViewportRect, SrcRDGTex, c.Key);
		}
	}
}
void FVRM4URenderModule::OnOverlay(FPostOpaqueRenderParameters& Parameters) {
	if (CaptureList.Num() == 0) return;

	if (isCaptureTarget(Parameters.View) == false) {
		return;
	}

	for (auto c : CaptureList) {
		if (c.Key == nullptr) {
			continue;
		}
		if (c.Key->GetRenderTargetResource() == nullptr) {
			continue;
		}

		FRDGTextureRef DstRDGTex = nullptr;
		FRDGTextureRef SrcRDGTex = nullptr;

		DstRDGTex = RegisterExternalTexture(*(Parameters.GraphBuilder), c.Key->GetRenderTargetResource()->GetTexture2DRHI(), TEXT("VRM4U_CopyDst"));
		switch (c.Value) {
		case EVRM4U_CaptureSource::ColorTextureOverlay:
			SrcRDGTex = Parameters.ColorTexture;
			break;
		default:
			break;
		}

		if (SrcRDGTex == nullptr && Parameters.SceneTexturesUniformParams == nullptr) continue;
		switch (c.Value) {
		case EVRM4U_CaptureSource::SceneColorTextureOverlay:
			SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->SceneColorTexture;
			break;
		default:
			break;
		}

		if (DstRDGTex && SrcRDGTex) {
			/*
			FScreenPassRenderTarget DstTex(DstRDGTex, ERenderTargetLoadAction::EClear);
			FScreenPassTexture SrcTex(SrcRDGTex);
			AddDrawTexturePass(
				*(Parameters.GraphBuilder),
				*Parameters.View,
				SrcTex,
				DstTex);
			*/

			FVRM4URenderModule::AddCopyPass(*Parameters.GraphBuilder, Parameters.ViewportRect, SrcRDGTex, c.Key);
		}
	}
}


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FVRM4URenderModule, VRM4URender);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
