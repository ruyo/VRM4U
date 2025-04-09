// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/EngineVersionComparison.h"
#include "VrmSceneViewExtension.h"

#if WITH_EDITOR
#include "UnrealEdMisc.h"
#endif

#include "VRM4U_RenderSubsystem.generated.h"



#if	UE_VERSION_OLDER_THAN(4,22,0)

//Couldn't find parent type for 'VRM4U_AnimSubsystem' named 'UEngineSubsystem'
#error "please remove VRM4U_AnimSubsystem.h/cpp  for <=UE4.21"

#endif

UENUM()
enum EVRM4U_CaptureSource : int
{
	ColorTexturePostOpaque,
	ColorTextureOverlay,
	DepthTexture,
	NormalTexture,
	VelocityTexture,
	//SmallDepthTexture,

	SceneColorTexturePostOpaque,
	SceneColorTextureOverlay,
	SceneDepthTexture,
	//ScenePartialDepthTexture,

	// GBuffer
	GBufferATexture,
	GBufferBTexture,
	GBufferCTexture,
	GBufferDTexture,
	GBufferETexture,
	GBufferFTexture,
	GBufferVelocityTexture,

	// SSAO
	ScreenSpaceAOTexture,

	// Custom Depth / Stencil
	CustomDepthTexture,

	//CaptureSource_MAX,
};

UCLASS()
class VRM4URENDER_API UVRM4U_RenderSubsystem : public UEngineSubsystem
{

	GENERATED_BODY()

	FDelegateHandle HandleTearDown;
	bool bInitPIE = false;
	bool bIsPlay = false;

public:

	bool bUsePostRenderBasePass = false;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void RenderPre(FRDGBuilder& GraphBuilder);
	void RenderPost(FRDGBuilder& GraphBuilder);

	void OnPostOpaque(FPostOpaqueRenderParameters& Parameters);
	void OnOverlay(FPostOpaqueRenderParameters& Parameters);
	void OnResolvedSceneColor_RenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures);

#if WITH_EDITOR
	void OnMapChange(UWorld* World, EMapChangeType ChangeType);

	void OnPIEEvent(bool bPIEBegin, bool bPIEEnd);
#endif

	TSharedPtr<class FVrmSceneViewExtension, ESPMode::ThreadSafe> SceneViewExtension;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRM4U")
	TMap<TObjectPtr<UTextureRenderTarget2D>, TEnumAsByte<EVRM4U_CaptureSource> > CaptureList;

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void AddCaptureTexture(UTextureRenderTarget2D *Texture, EVRM4U_CaptureSource CaptureSource);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void RemoveCaptureTexture(UTextureRenderTarget2D* Texture);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void RemoveAllCaptureTexture();

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void ResetSceneTextureExtentHistory();

	//
	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetViewExtension(bool bEnable);
};
