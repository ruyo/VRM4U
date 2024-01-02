// Fill out your copyright notice in the Description page of Project Settings.


#include "VRM4U_RenderSubsystem.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"
#include "EngineModule.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RendererInterface.h"
#include "ScreenPass.h"
#include "Runtime/Renderer/Private/SceneTextures.h"
#include "Runtime/Renderer/Private/SceneRendering.h"


void UVRM4U_RenderSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	Super::Initialize(Collection);

    //SceneViewExtension = FSceneViewExtensions::NewExtension<FVrmSceneViewExtension>();
    //GetRendererModule().RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateUObject(this, &UVRM4U_RenderSubsystem::OnPostOpaque));
    //GetRendererModule().RegisterOverlayRenderDelegate(FPostOpaqueRenderDelegate::CreateUObject(this, &UVRM4U_RenderSubsystem::OnOverlay));
    
    //GetRendererModule().GetResolvedSceneColorCallbacks().AddUObject(this, &UVRM4U_RenderSubsystem::OnResolvedSceneColor_RenderThread);
}

void UVRM4U_RenderSubsystem::OnResolvedSceneColor_RenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures) {
    //SceneTextures.Color.Resolve
}

void UVRM4U_RenderSubsystem::OnPostOpaque(FPostOpaqueRenderParameters& Parameters) {

    if (CaptureList.Num() == 0) return;

    {
        TArray<TObjectPtr<UTextureRenderTarget2D>> keys;
        CaptureList.GetKeys(keys);
        for (auto k : keys) {
            if (IsValid(k) == false) {
                CaptureList.Remove(k);
            }
        }
    }

    FRDGTextureRef DstRDGTex = nullptr;
    FRDGTextureRef SrcRDGTex = nullptr;

    for (auto c : CaptureList) {
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
        case EVRM4U_CaptureSource::SmallDepthTexture:
            SrcRDGTex = Parameters.SmallDepthTexture;
            break;
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
        case EVRM4U_CaptureSource::ScenePartialDepthTexture:
            SrcRDGTex = Parameters.SceneTexturesUniformParams->GetParameters()->ScenePartialDepthTexture;
            break;

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
            FScreenPassRenderTarget DstTex(DstRDGTex, ERenderTargetLoadAction::EClear);
            FScreenPassTexture SrcTex(SrcRDGTex);

            AddDrawTexturePass(
                *(Parameters.GraphBuilder),
                *Parameters.View,
                SrcTex,
                DstTex
            );
        }
    }
}
void UVRM4U_RenderSubsystem::OnOverlay(FPostOpaqueRenderParameters& Parameters) {
    if (CaptureList.Num() == 0) return;


    for (auto c : CaptureList) {
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
            FScreenPassRenderTarget DstTex(DstRDGTex, ERenderTargetLoadAction::EClear);
            FScreenPassTexture SrcTex(SrcRDGTex);

            AddDrawTexturePass(
                *(Parameters.GraphBuilder),
                *Parameters.View,
                SrcTex,
                DstTex
            );
        }
    }
}

void UVRM4U_RenderSubsystem::RenderPre(FRDGBuilder& GraphBuilder) {
}
void UVRM4U_RenderSubsystem::RenderPost(FRDGBuilder& GraphBuilder) {
}


void UVRM4U_RenderSubsystem::AddCaptureTexture(UTextureRenderTarget2D* Texture, EVRM4U_CaptureSource CaptureSource) {

    if (Texture == nullptr) return;

    CaptureList.Add(Texture, CaptureSource);
}

void UVRM4U_RenderSubsystem::RemoveCaptureTexture(UTextureRenderTarget2D* Texture) {
    CaptureList.Remove(Texture);
}

void UVRM4U_RenderSubsystem::RemoveAllCaptureTexture() {
    CaptureList.Empty();
}

