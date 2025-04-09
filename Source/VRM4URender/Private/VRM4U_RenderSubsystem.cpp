// Fill out your copyright notice in the Description page of Project Settings.


#include "VRM4U_RenderSubsystem.h"
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
#if UE_VERSION_OLDER_THAN(5,4,0)
#include "Runtime/Renderer/Private/SceneTextures.h"
#endif


#if WITH_EDITOR
#include "LevelEditor.h"
#endif

namespace{
	void VRM4U_AddCopyPass(FPostOpaqueRenderParameters& Parameters, FRDGTextureRef SrcRDGTex, TObjectPtr<UTextureRenderTarget2D> RenderTarget) {

		const FIntPoint ViewRectSize = FIntPoint(Parameters.ViewportRect.Width(), Parameters.ViewportRect.Height());

		AddPass(*Parameters.GraphBuilder, RDG_EVENT_NAME("VRM4UAddCopyPass"), [ViewRectSize, SrcRDGTex, RenderTarget](FRHICommandListImmediate& RHICmdList)
			{
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

}

void UVRM4U_RenderSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	Super::Initialize(Collection);

	SceneViewExtension = FSceneViewExtensions::NewExtension<FVrmSceneViewExtension>();
	GetRendererModule().RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateUObject(this, &UVRM4U_RenderSubsystem::OnPostOpaque));
	GetRendererModule().RegisterOverlayRenderDelegate(FPostOpaqueRenderDelegate::CreateUObject(this, &UVRM4U_RenderSubsystem::OnOverlay));
	
	//GetRendererModule().GetResolvedSceneColorCallbacks().AddUObject(this, &UVRM4U_RenderSubsystem::OnResolvedSceneColor_RenderThread);
}

void UVRM4U_RenderSubsystem::OnResolvedSceneColor_RenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures) {
}

void UVRM4U_RenderSubsystem::OnPostOpaque(FPostOpaqueRenderParameters& Parameters) {

	if (CaptureList.Num() == 0) return;

	if (bIsPlay) {
		if (Parameters.View->PlayerIndex == INDEX_NONE) return;
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
			VRM4U_AddCopyPass(Parameters, SrcRDGTex, c.Key);
		}
	}
}
void UVRM4U_RenderSubsystem::OnOverlay(FPostOpaqueRenderParameters& Parameters) {
	if (CaptureList.Num() == 0) return;

	if (bIsPlay) {
		if (Parameters.View->PlayerIndex == INDEX_NONE) return;
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

			VRM4U_AddCopyPass(Parameters, SrcRDGTex, c.Key);
		}
	}
}

#if WITH_EDITOR
void UVRM4U_RenderSubsystem::OnMapChange(UWorld* World, EMapChangeType ChangeType) {
	if (ChangeType == EMapChangeType::TearDownWorld)
	{
		CaptureList.Empty();
	}
}

void UVRM4U_RenderSubsystem::OnPIEEvent(bool bPIEBegin, bool bPIEEnd) {
	bIsPlay = bPIEBegin;
}
#endif


void UVRM4U_RenderSubsystem::RenderPre(FRDGBuilder& GraphBuilder) {
}
void UVRM4U_RenderSubsystem::RenderPost(FRDGBuilder& GraphBuilder) {
}

void UVRM4U_RenderSubsystem::AddCaptureTexture(UTextureRenderTarget2D* Texture, EVRM4U_CaptureSource CaptureSource) {
	if (Texture == nullptr) return;

	CaptureList.FindOrAdd(Texture) = CaptureSource;

#if WITH_EDITOR
	if (HandleTearDown.IsValid() == false){
		if (CaptureList.Num() == 1) {
			if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
			{
				FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

				
				HandleTearDown = LevelEditor.OnMapChanged().AddUObject(this, &UVRM4U_RenderSubsystem::OnMapChange);
						/*
					HandleTearDown = LevelEditor.OnMapChanged().AddLambda([&](UWorld* World, EMapChangeType ChangeType)
					{
						if (ChangeType == EMapChangeType::TearDownWorld)
						{
							CaptureList.Empty();
						}
					});
					*/
			}
		}
	}

	if (bInitPIE == false) {
		bInitPIE = true;
		FEditorDelegates::BeginPIE.AddLambda([&](const bool bIsSimulating) {
			this->OnPIEEvent(true, false);
			});
		FEditorDelegates::EndPIE.AddLambda([&](const bool bIsSimulating) {
			this->OnPIEEvent(false, true);
			});
	}
#else
	bIsPlay = true;
#endif
}

void UVRM4U_RenderSubsystem::RemoveCaptureTexture(UTextureRenderTarget2D* Texture) {
	CaptureList.Remove(Texture);
}

void UVRM4U_RenderSubsystem::RemoveAllCaptureTexture() {
	CaptureList.Empty();
}


void UVRM4U_RenderSubsystem::ResetSceneTextureExtentHistory() {
	::ResetSceneTextureExtentHistory();
}

void UVRM4U_RenderSubsystem::SetViewExtension(bool bEnable) {

	if (SceneViewExtension == nullptr) {
		return;
	}

//	SceneViewExtension->
}
