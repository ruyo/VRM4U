
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


#define LOCTEXT_NAMESPACE "VRM4URender"

DEFINE_LOG_CATEGORY(LogVRM4URender);

namespace {
	bool isCaptureTarget(FPostOpaqueRenderParameters& Parameters) {

		bool bCapture = false;

		bool bPlay = false;
		bool bSIE = false;
		bool bEditor = false;
		UVrmBPFunctionLibrary::VRMGetPlayMode(bPlay, bSIE, bEditor);

		UWorld* World = Parameters.View->Family->Scene->GetWorld();
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
		if (Parameters.View->bIsGameView) {
			bCapture = true;
		}
		if (Parameters.View->bIsOfflineRender) {
			bCapture = true;
		}

		return bCapture;
	}
}

void FVRM4URenderModule::AddCopyPass(FRDGBuilder &GraphBuilder, FIntPoint ViewRectSize, FRDGTextureRef SrcRDGTex, TObjectPtr<UTextureRenderTarget2D> RenderTarget) {

	//FPostOpaqueRenderParameters& Parameters
	//const FIntPoint ViewRectSize = FIntPoint(Parameters.ViewportRect.Width(), Parameters.ViewportRect.Height());

	AddPass(GraphBuilder, RDG_EVENT_NAME("VRM4UAddCopyPass"), [ViewRectSize, SrcRDGTex, RenderTarget](FRHICommandListImmediate& RHICmdList)
		{
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
					0, 0,									// Source U, V
					ViewRectSize.X, ViewRectSize.Y,			// Source USize, VSize
					TargetSize,								// Target buffer size
					FIntPoint(SceneTexture->GetSizeX(), SceneTexture->GetSizeY()),	// Source texture size
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

	if (isCaptureTarget(Parameters) == false) {
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
			FVRM4URenderModule::AddCopyPass(*Parameters.GraphBuilder, FIntPoint(Parameters.ViewportRect.Width(), Parameters.ViewportRect.Height()), SrcRDGTex, c.Key);
		}
	}
}
void FVRM4URenderModule::OnOverlay(FPostOpaqueRenderParameters& Parameters) {
	if (CaptureList.Num() == 0) return;

	if (isCaptureTarget(Parameters) == false) {
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

			FVRM4URenderModule::AddCopyPass(*Parameters.GraphBuilder, FIntPoint(Parameters.ViewportRect.Width(), Parameters.ViewportRect.Height()), SrcRDGTex, c.Key);
		}
	}
}


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FVRM4URenderModule, VRM4URender);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
