// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/EngineVersionComparison.h"
#include "VRM4U_RenderSubsystem.generated.h"


#if	UE_VERSION_OLDER_THAN(4,22,0)

//Couldn't find parent type for 'VRM4U_AnimSubsystem' named 'UEngineSubsystem'
#error "please remove VRM4U_AnimSubsystem.h/cpp  for <=UE4.21"

#endif

UCLASS()
class VRM4URENDER_API UVRM4U_RenderSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void RenderPre(FRDGBuilder& GraphBuilder);
	void RenderPost(FRDGBuilder& GraphBuilder);

	void PostOpaque(FPostOpaqueRenderParameters& Parameters);

	void OnResolvedSceneColor_RenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures);


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* RenderTarget = nullptr;
};
