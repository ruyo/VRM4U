

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RendererInterface.h"

//#include "VRMImporterModule.h"

//DECLARE_LOG_CATEGORY_EXTERN(LogVRM4URender, Verbose, All);

//#define SPRITER_IMPORT_ERROR(FormatString, ...) \
//	if (!bSilent) { UE_LOG(LogVRM4URender, Warning, FormatString, __VA_ARGS__); }
//#define SPRITER_IMPORT_WARNING(FormatString, ...) \
//	if (!bSilent) { UE_LOG(LogVRM4URender, Warning, FormatString, __VA_ARGS__); }

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


class FVRM4URenderModule : public FDefaultModuleImpl
{

	FDelegateHandle HandleTearDown;
	bool bIsPlay = false;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRM4U")
	TMap<TObjectPtr<UTextureRenderTarget2D>, TEnumAsByte<EVRM4U_CaptureSource> > CaptureList;


	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OnPostOpaque(FPostOpaqueRenderParameters& Parameters);
	void OnOverlay(FPostOpaqueRenderParameters& Parameters);

#if WITH_EDITOR
	void OnMapChange(UWorld* World, EMapChangeType ChangeType);

	void OnPIEEvent(bool bPIEBegin, bool bPIEEnd);
#endif
};
