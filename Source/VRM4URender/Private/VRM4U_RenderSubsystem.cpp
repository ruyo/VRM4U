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

	//GEngine->GetPreRenderDelegateEx().AddUObject(this, &UVRM4U_RenderSubsystem::RenderPre);
	//GEngine->GetPostRenderDelegateEx().AddUObject(this, &UVRM4U_RenderSubsystem::RenderPost);

    //GetRendererModule().RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateUObject(this, &UVRM4U_RenderSubsystem::PostOpaque));
    //GetRendererModule().RegisterOverlayRenderDelegate(FPostOpaqueRenderDelegate::CreateUObject(this, &UVRM4U_RenderSubsystem::PostOpaque));
    
    //GetRendererModule().GetResolvedSceneColorCallbacks().AddUObject(this, &UVRM4U_RenderSubsystem::OnResolvedSceneColor_RenderThread);
}

void UVRM4U_RenderSubsystem::OnResolvedSceneColor_RenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures) {
    if (RenderTarget == nullptr) return;

    FRDGTextureRef DstTexture = RegisterExternalTexture(GraphBuilder, RenderTarget->GetRenderTargetResource()->GetTexture2DRHI(), TEXT("CopyDstPost"));
    //FRDGTextureRef SourceTexture = RegisterExternalTexture(GraphBuilder, RenderTargets[1].Texture->GetRHI(), TEXT("CopySrcPost"));;
    //FRDGTextureRef SourceTexture = RenderTargets[0].Texture;
    //FRDGTextureRef SourceTexture = SceneTextures.Velocity;
    //FRDGTextureRef SourceTexture = Parameters.DepthTexture;

    FRHICopyTextureInfo CopyInfo;
    //AddCopyTexturePass(*(Parameters.GraphBuilder), SourceTexture, DstTexture, CopyInfo);

    //FRDGDrawTextureInfo Info;
    //AddDrawTexturePass(
    //    GraphBuilder,
    //    GetGlobalShaderMap(GMaxRHIFeatureLevel),
    //    SourceTexture,
     //   DstTexture,
    //    Info
    //);

    /*
    FRDGTextureRef DstTexture2 = RegisterExternalTexture(GraphBuilder, RenderTarget->GetRenderTargetResource()->GetTexture2DRHI(), TEXT("CopyDstPost"));
    FRDGTextureRef SrcTexture2 = SceneTextures.Color;

    FScreenPassRenderTarget DstTexture(DstTexture2, ERenderTargetLoadAction::EClear);
    FScreenPassTexture SrcTexture(SrcTexture2);

    AddDrawTexturePass(
        GraphBuilder,
        *Parameters.View,
        SrcTexture,
        DstTexture
    );
    */
}


void UVRM4U_RenderSubsystem::PostOpaque(FPostOpaqueRenderParameters& Parameters) {

    if (RenderTarget == nullptr) return;

    {
        FRDGTextureRef DstTexture2 = RegisterExternalTexture(*(Parameters.GraphBuilder), RenderTarget->GetRenderTargetResource()->GetTexture2DRHI(), TEXT("CopyDstPost"));
        //FRDGTextureRef SrcTexture2 = Parameters.ColorTexture;
        FRDGTextureRef SrcTexture2 = Parameters.SceneTexturesUniformParams->GetParameters()->GBufferBTexture;

        FScreenPassRenderTarget DstTexture(DstTexture2, ERenderTargetLoadAction::EClear);
        FScreenPassTexture SrcTexture(SrcTexture2);

        AddDrawTexturePass(
            *(Parameters.GraphBuilder),
            *Parameters.View,
            SrcTexture,
            DstTexture
        );
    }
}

void UVRM4U_RenderSubsystem::RenderPre(FRDGBuilder& GraphBuilder) {
}
void UVRM4U_RenderSubsystem::RenderPost(FRDGBuilder& GraphBuilder) {
}
