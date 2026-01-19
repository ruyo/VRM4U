// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/EngineVersionComparison.h"
#include "VrmSceneViewExtension.h"
#include "VrmExtensionRimFilterData.h"
#include "VRM4URender.h"

#if WITH_EDITOR
#include "UnrealEdMisc.h"
#endif

#include "VRM4U_RenderSubsystem.generated.h"



#if	UE_VERSION_OLDER_THAN(4,22,0)

//Couldn't find parent type for 'VRM4U_AnimSubsystem' named 'UEngineSubsystem'
#error "please remove VRM4U_AnimSubsystem.h/cpp  for <=UE4.21"

#endif


UCLASS()
class VRM4URENDER_API UVRM4U_RenderSubsystem : public UEngineSubsystem
{

	GENERATED_BODY()

	FDelegateHandle HandleTearDown;
	bool bInitPIE = false;

public:

	//// rim filter
	FCriticalSection cs_rim;

	TArray< TWeakObjectPtr<class UVrmExtensionRimFilterData> > RimFilterData;

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void AddRimFilterData(class UVrmExtensionRimFilterData *FilterData);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void RemoveRimFilterData(class UVrmExtensionRimFilterData* FilterData);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void RemoveRimFilterDataByPriority(int Priotiry = -1);

	TArray<struct UVrmExtensionRimFilterData::FFilterData> GenerateFilterData();

	////

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RenderPre(FRDGBuilder& GraphBuilder);
	void RenderPost(FRDGBuilder& GraphBuilder);

	void OnPostOpaque(FPostOpaqueRenderParameters& Parameters);
	void OnOverlay(FPostOpaqueRenderParameters& Parameters);
	void OnResolvedSceneColor_RenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures);

#if WITH_EDITOR
	void OnMapChange(UWorld* World, EMapChangeType ChangeType);

	void OnPIEEvent(bool bPIEBegin, bool bPIEEnd);
#endif

	// keep class
	TSharedPtr<class FVrmSceneViewExtension, ESPMode::ThreadSafe> SceneViewExtension;

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void AddCaptureTexture(UTextureRenderTarget2D *Texture, EVRM4U_CaptureSource CaptureSource = EVRM4U_CaptureSource::ColorTextureOverlay);

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
